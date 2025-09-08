//
// Copyright (c) 2025 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   ZettaScale Zenoh Team, <zenoh@zettascale.tech>
//

#include "zenoh-pico/system/link/tls.h"

#if Z_FEATURE_LINK_TLS == 1

#include <errno.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "mbedtls/debug.h"
#include "mbedtls/entropy.h"
#include "mbedtls/error.h"
#include "mbedtls/hmac_drbg.h"
#include "mbedtls/md.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/x509_crt.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/link/config/tls.h"
#include "zenoh-pico/system/link/tcp.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

// static void _z_tls_debug(void *ctx, int level, const char *file, int line, const char *str) {
//     _ZP_UNUSED(ctx);
//     _Z_DEBUG("mbed TLS [%d] %s:%04d: %s", level, file, line, str);
// }

static int _z_tls_bio_send(void *ctx, const unsigned char *buf, size_t len) {
    int fd = *(int *)ctx;
    _Z_DEBUG("10 %s ctx=%p, fd=%d, len=%zu", __func__, ctx, fd, len);
    ssize_t n = send(fd, buf, len, 0);
    if (n < 0) {
        _Z_DEBUG("20 %s send failed: errno=%d (%s)", __func__, errno, strerror(errno));
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return MBEDTLS_ERR_SSL_WANT_WRITE;
        }
        return MBEDTLS_ERR_NET_SEND_FAILED;
    }
    _Z_DEBUG("30 %s send success: %zd bytes", __func__, n);
    return (int)n;
}

static int _z_tls_bio_recv(void *ctx, unsigned char *buf, size_t len) {
    int fd = *(int *)ctx;
    ssize_t n = recv(fd, buf, len, 0);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return MBEDTLS_ERR_SSL_WANT_READ;
        }
        return MBEDTLS_ERR_NET_RECV_FAILED;
    }
    if (n == 0) {
        return MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY;
    }
    return (int)n;
}

_z_tls_context_t *_z_tls_context_new(void) {
    _z_tls_context_t *ctx = (_z_tls_context_t *)z_malloc(sizeof(_z_tls_context_t));
    if (ctx == NULL) {
        return NULL;
    }

    mbedtls_ssl_init(&ctx->_ssl);
    mbedtls_ssl_config_init(&ctx->_ssl_config);
    mbedtls_entropy_init(&ctx->_entropy);
    mbedtls_hmac_drbg_init(&ctx->_hmac_drbg);
    mbedtls_x509_crt_init(&ctx->_ca_cert);

    int ret = mbedtls_hmac_drbg_seed(&ctx->_hmac_drbg, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
                                     mbedtls_entropy_func, &ctx->_entropy, NULL, 0);
    if (ret != 0) {
        _Z_ERROR("Failed to seed HMAC_DRBG: -0x%04x", -ret);
        _z_tls_context_free(&ctx);
        return NULL;
    }

    return ctx;
}

void _z_tls_context_free(_z_tls_context_t **ctx) {
    if (ctx != NULL && *ctx != NULL) {
        mbedtls_ssl_free(&(*ctx)->_ssl);
        mbedtls_ssl_config_free(&(*ctx)->_ssl_config);
        mbedtls_entropy_free(&(*ctx)->_entropy);
        mbedtls_hmac_drbg_free(&(*ctx)->_hmac_drbg);
        mbedtls_x509_crt_free(&(*ctx)->_ca_cert);
        z_free(*ctx);
        *ctx = NULL;
    }
}

static z_result_t _z_tls_load_ca_certificate(_z_tls_context_t *ctx, const _z_str_intmap_t *config) {
    const char *ca_cert_str = _z_str_intmap_get(config, TLS_CONFIG_ROOT_CA_CERTIFICATE_KEY);
    if (ca_cert_str == NULL) {
        _Z_INFO("No CA certificate specified, using system default");
        return _Z_RES_OK;
    }

    int ret = mbedtls_x509_crt_parse_file(&ctx->_ca_cert, ca_cert_str);
    if (ret != 0) {
        _Z_ERROR("Failed to parse CA certificate file %s: -0x%04x", ca_cert_str, -ret);
        return _Z_ERR_GENERIC;
    }

    _Z_DEBUG("Loaded CA certificate from %s", ca_cert_str);
    return _Z_RES_OK;
}

z_result_t _z_open_tls(_z_tls_socket_t *sock, const _z_sys_net_endpoint_t rep, const _z_str_intmap_t *config) {
    _Z_DEBUG("01 %s entry: sock=%p", __func__, (void *)sock);

    sock->_tls_ctx = _z_tls_context_new();
    if (sock->_tls_ctx == NULL) {
        _Z_ERROR("Failed to create TLS context");
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }

    sock->_rep = rep;

    z_result_t ret = _z_tls_load_ca_certificate(sock->_tls_ctx, config);
    if (ret != _Z_RES_OK) {
        _Z_ERROR("Failed to load CA certificate");
        _z_tls_context_free(&sock->_tls_ctx);
        return ret;
    }

    ret = _z_open_tcp(&sock->_sock, rep, Z_CONFIG_SOCKET_TIMEOUT);
    if (ret != _Z_RES_OK) {
        _Z_ERROR("Failed to open TCP socket: %d", ret);
        _z_tls_context_free(&sock->_tls_ctx);
        return ret;
    }

    // Needed for _read_socket_f callback which requires TLS context
    sock->_sock._tls_ctx = (void *)sock;

    int mbedret = mbedtls_ssl_config_defaults(&sock->_tls_ctx->_ssl_config, MBEDTLS_SSL_IS_CLIENT,
                                              MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    if (mbedret != 0) {
        _Z_ERROR("Failed to set SSL config defaults: -0x%04x", -mbedret);
        _z_close_tcp(&sock->_sock);
        _z_tls_context_free(&sock->_tls_ctx);
        return _Z_ERR_GENERIC;
    }

    if (sock->_tls_ctx->_ca_cert.version != 0) {
        mbedtls_ssl_conf_ca_chain(&sock->_tls_ctx->_ssl_config, &sock->_tls_ctx->_ca_cert, NULL);
    }
    mbedtls_ssl_conf_authmode(&sock->_tls_ctx->_ssl_config, MBEDTLS_SSL_VERIFY_NONE);
    mbedtls_ssl_conf_rng(&sock->_tls_ctx->_ssl_config, mbedtls_hmac_drbg_random, &sock->_tls_ctx->_hmac_drbg);

    mbedret = mbedtls_ssl_setup(&sock->_tls_ctx->_ssl, &sock->_tls_ctx->_ssl_config);
    if (mbedret != 0) {
        _Z_ERROR("Failed to setup SSL: -0x%04x", -mbedret);
        _z_close_tcp(&sock->_sock);
        _z_tls_context_free(&sock->_tls_ctx);
        return _Z_ERR_GENERIC;
    }

    _Z_DEBUG("10 %s setting BIO: sock=%p, fd=%d", __func__, (void *)sock, sock->_sock._fd);
    mbedtls_ssl_set_bio(&sock->_tls_ctx->_ssl, &sock->_sock._fd, _z_tls_bio_send, _z_tls_bio_recv, NULL);
    _Z_DEBUG("20 %s BIO set complete: sock=%p, fd=%d", __func__, (void *)sock, sock->_sock._fd);

    while ((mbedret = mbedtls_ssl_handshake(&sock->_tls_ctx->_ssl)) != 0) {
        if (mbedret != MBEDTLS_ERR_SSL_WANT_READ && mbedret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            _Z_ERROR("TLS handshake failed: -0x%04x", -mbedret);
            _z_close_tcp(&sock->_sock);
            _z_tls_context_free(&sock->_tls_ctx);
            return _Z_ERR_GENERIC;
        }
    }

    _Z_DEBUG("99 %s TLS handshake completed successfully: sock=%p, &sock->_sock=%p, fd=%d", __func__, (void *)sock,
             (void *)&sock->_sock, sock->_sock._fd);
    return _Z_RES_OK;
}

z_result_t _z_listen_tls(_z_tls_socket_t *sock, const char *host, const char *port, const _z_str_intmap_t *config) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(host);
    _ZP_UNUSED(port);
    _ZP_UNUSED(config);
    _Z_ERROR("TLS server not implemented in current version");
    return _Z_ERR_GENERIC;
}

void _z_close_tls(_z_tls_socket_t *sock) {
    if (sock->_tls_ctx != NULL) {
        mbedtls_ssl_close_notify(&sock->_tls_ctx->_ssl);
        _z_tls_context_free(&sock->_tls_ctx);
    }
    _z_close_tcp(&sock->_sock);
}

size_t _z_read_tls(const _z_tls_socket_t *sock, uint8_t *ptr, size_t len) {
    _Z_DEBUG("10 %s TLS read attempt: %zu bytes, sock=%p", __func__, len, (void *)sock);
    if (sock->_tls_ctx == NULL) {
        _Z_ERROR("TLS context is NULL");
        return SIZE_MAX;
    }
    int ret = mbedtls_ssl_read(&sock->_tls_ctx->_ssl, ptr, len);
    if (ret > 0) {
        _Z_DEBUG("20 %s TLS read success: %d bytes", __func__, ret);
        return (size_t)ret;
    }

    if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
        _Z_DEBUG("30 %s TLS read would block, retry needed", __func__);
        return 0;
    }

    if (ret == 0 || ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
        _Z_DEBUG("40 %s TLS peer closed connection", __func__);
        return 0;
    }

    _Z_ERROR("50 %s TLS read error: -0x%04x", __func__, -ret);
    return SIZE_MAX;
}

size_t _z_write_tls(const _z_tls_socket_t *sock, const uint8_t *ptr, size_t len) {
    _Z_DEBUG("10 %s TLS write attempt: %zu bytes, sock=%p", __func__, len, (void *)sock);
    if (sock->_tls_ctx == NULL) {
        _Z_ERROR("TLS context is NULL");
        return SIZE_MAX;
    }
    int ret = mbedtls_ssl_write(&sock->_tls_ctx->_ssl, ptr, len);
    _Z_DEBUG("15 %s after mbedtls_ssl_write: sock=%p", __func__, (void *)sock);
    if (ret > 0) {
        _Z_DEBUG("20 %s TLS write success: %d bytes", __func__, ret);
        return (size_t)ret;
    }

    if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
        _Z_DEBUG("30 %s TLS write would block, retry needed", __func__);
        return 0;
    }

    _Z_ERROR("40 %s TLS write error: -0x%04x", __func__, -ret);
    return SIZE_MAX;
}

size_t _z_write_all_tls(const _z_tls_socket_t *sock, const uint8_t *ptr, size_t len) {
    size_t n = 0;
    do {
        size_t wb = _z_write_tls(sock, &ptr[n], len - n);
        if (wb == SIZE_MAX) {
            return wb;
        }
        n += wb;
    } while (n < len);
    return n;
}

#endif  // Z_FEATURE_LINK_TLS == 1
