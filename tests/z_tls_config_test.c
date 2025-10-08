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

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico.h"

#if Z_FEATURE_LINK_TLS == 1

#include "mbedtls/base64.h"

static const char SERVER_CA_PEM[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDSzCCAjOgAwIBAgIITcwv1N10nqEwDQYJKoZIhvcNAQELBQAwIDEeMBwGA1UE\n"
    "AxMVbWluaWNhIHJvb3QgY2EgNGRjYzJmMCAXDTIzMDMwNjE2NDEwNloYDzIxMjMw\n"
    "MzA2MTY0MTA2WjAgMR4wHAYDVQQDExVtaW5pY2Egcm9vdCBjYSA0ZGNjMmYwggEi\n"
    "MA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC2WUgN7NMlXIknew1cXiTWGmS0\n"
    "1T1EjcNNDAq7DqZ7/ZVXrjD47yxTt5EOiOXK/cINKNw4Zq/MKQvq9qu+Oax4lwiV\n"
    "Ha0i8ShGLSuYI1HBlXu4MmvdG+3/SjwYoGsGaShr0y/QGzD3cD+DQZg/RaaIPHlO\n"
    "MdmiUXxkMcy4qa0hFJ1imlJdq/6Tlx46X+0vRCh8nkekvOZR+t7Z5U4jn4XE54Kl\n"
    "0PiwcyX8vfDZ3epa/FSHZvVQieM/g5Yh9OjIKCkdWRg7tD0IEGsaW11tEPJ5SiQr\n"
    "mDqdRneMzZKqY0xC+QqXSvIlzpOjiu8PYQx7xugaUFE/npKRQdvh8ojHJMdNAgMB\n"
    "AAGjgYYwgYMwDgYDVR0PAQH/BAQDAgKEMB0GA1UdJQQWMBQGCCsGAQUFBwMBBggr\n"
    "BgEFBQcDAjASBgNVHRMBAf8ECDAGAQH/AgEAMB0GA1UdDgQWBBTX46+p+Po1npE6\n"
    "QLQ7mMI+83s6qDAfBgNVHSMEGDAWgBTX46+p+Po1npE6QLQ7mMI+83s6qDANBgkq\n"
    "hkiG9w0BAQsFAAOCAQEAaN0IvEC677PL/JXzMrXcyBV88IvimlYN0zCt48GYlhmx\n"
    "vL1YUDFLJEB7J+dyERGE5N6BKKDGblC4WiTFgDMLcHFsMGRc0v7zKPF1PSBwRYJi\n"
    "ubAmkwdunGG5pDPUYtTEDPXMlgClZ0YyqSFJMOqA4IzQg6exVjXtUxPqzxNhyC7S\n"
    "vlgUwPbX46uNi581a9+Ls2V3fg0ZnhkTSctYZHGZNeh0Nsf7Am8xdUDYG/bZcVef\n"
    "jbQ9gpChosdjF0Bgblo7HSUct/2Va+YlYwW+WFjJX8k4oN6ZU5W5xhdfO8Czmgwk\n"
    "US5kJ/+1M0uR8zUhZHL61FbsdPxEj+fYKrHv4woo+A==\n"
    "-----END CERTIFICATE-----\n";

static const char SERVER_CERT_PEM[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDLjCCAhagAwIBAgIIW1mAtJWJAJYwDQYJKoZIhvcNAQELBQAwIDEeMBwGA1UE\n"
    "AxMVbWluaWNhIHJvb3QgY2EgNGRjYzJmMCAXDTIzMDMwNjE2NDEwNloYDzIxMjMw\n"
    "MzA2MTY0MTA2WjAUMRIwEAYDVQQDEwlsb2NhbGhvc3QwggEiMA0GCSqGSIb3DQEB\n"
    "AQUAA4IBDwAwggEKAoIBAQCYMLJKooc+YRlKEMfeV09pX9myH34eUcUuT0fXS8lm\n"
    "PlZ/NW7mm5lDwa8EUg61WuXQv2ouQDptmIcdeb/w4RW93Xflkyng1Xbd91OwQBJd\n"
    "+8ZVBjzL7hSRk3QPDqx/CVBU/I1GmXKzb6cWzq1fTkOn1WLNXf21I6p7+N3qHLPF\n"
    "JQeoVq1HBBFcAjTgJnpyQNvRGLDuLTK+OsWEGib2U8qrgiRdkaBLkxGXSlGABlOo\n"
    "cQyW/zOhf4pwb2Z/JAge2mRW5IcexCPBWint8ydPsoJDds8j5+AyYCD6HUhHX0Ob\n"
    "Qkz73OW7f2PQhuTK2uzKy0Yz6lNFt2nuzaWC04wIW3T7AgMBAAGjdjB0MA4GA1Ud\n"
    "DwEB/wQEAwIFoDAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwDAYDVR0T\n"
    "AQH/BAIwADAfBgNVHSMEGDAWgBTX46+p+Po1npE6QLQ7mMI+83s6qDAUBgNVHREE\n"
    "DTALgglsb2NhbGhvc3QwDQYJKoZIhvcNAQELBQADggEBAAxrmQPG54ybKgMVliN8\n"
    "Mg5povSdPIVVnlU/HOVG9yxzAOav/xQP003M4wqpatWxI8tR1PcLuZf0EPmcdJgb\n"
    "tVl9nZMVZtveQnYMlU8PpkEVu56VM4Zr3rH9liPRlr0JEAXODdKw76kWKzmdqWZ/\n"
    "rzhup3Ek7iEX6T5j/cPUvTWtMD4VEK2I7fgoKSHIX8MIVzqM7cuboGWPtS3eRNXl\n"
    "MgvahA4TwLEXPEe+V1WAq6nSb4g2qSXWIDpIsy/O1WGS/zzRnKvXu9/9NkXWqZMl\n"
    "C1LSpiiQUaRSglOvYf/Zx6r+4BOS4OaaArwHkecZQqBSCcBLEAyb/FaaXdBowI0U\n"
    "PQ4=\n"
    "-----END CERTIFICATE-----\n";

static const char SERVER_KEY_PEM[] =
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "MIIEpAIBAAKCAQEAmDCySqKHPmEZShDH3ldPaV/Zsh9+HlHFLk9H10vJZj5WfzVu\n"
    "5puZQ8GvBFIOtVrl0L9qLkA6bZiHHXm/8OEVvd135ZMp4NV23fdTsEASXfvGVQY8\n"
    "y+4UkZN0Dw6sfwlQVPyNRplys2+nFs6tX05Dp9VizV39tSOqe/jd6hyzxSUHqFat\n"
    "RwQRXAI04CZ6ckDb0Riw7i0yvjrFhBom9lPKq4IkXZGgS5MRl0pRgAZTqHEMlv8z\n"
    "oX+KcG9mfyQIHtpkVuSHHsQjwVop7fMnT7KCQ3bPI+fgMmAg+h1IR19Dm0JM+9zl\n"
    "u39j0IbkytrsystGM+pTRbdp7s2lgtOMCFt0+wIDAQABAoIBADNTSO2uvlmlOXgn\n"
    "DKDJZTiuYKaXxFrJTOx/REUxg+x9XYJtLMeM9jVJnpKgceFrlFHAHDkY5BuN8xNX\n"
    "ugmsfz6W8BZ2eQsgMoRNIuYv1YHopUyLW/mSg1FNHzjsw/Pb2kGvIp4Kpgopv3oL\n"
    "naCkrmBtsHJ+Hk/2hUpl9cE8iMwVWcVevLzyHi98jNy1IDdIPhRtl0dhMiqC5MRr\n"
    "4gLJ5gNkLYX7xf3tw5Hmfk/bVNProqZXDIQVI7rFvItX586nvQ3LNQkmW/D2ShZf\n"
    "3FEqMu6EdA2Ycc4UZgAlQNGV0VBrWWVXizOQ+9gjLnBk3kJjqfigCU6NG94bTJ+H\n"
    "0YIhsGECgYEAwdSSyuMSOXgzZQ7Vv+GsNn/7ivi/H8eb/lDzksqS/JroA2ciAmHG\n"
    "2OF30eUJKRg+STqBTpOfXgS4QUa8QLSwBSnwcw6579x9bYGUhqD2Ypaw9uCnOukA\n"
    "CwwggZ9cDmF0tb5rYjqkW3bFPqkCnTGb0ylMFaYRhRDU20iG5t8PQckCgYEAyQEM\n"
    "KK18FLQUKivGrQgP5Ib6IC3myzlHGxDzfobXGpaQntFnHY7Cxp/6BBtmASzt9Jxu\n"
    "etnrevmzrbKqsLTJSg3ivbiq0YTLAJ1FsZrCp71dx49YR/5o9QFiq0nQoKnwUVeb\n"
    "/hrDjMAokNkjFL5vouXO711GSS6YyM4WzAKZAqMCgYEAhqGxaG06jmJ4SFx6ibIl\n"
    "nSFeRhQrJNbP+mCeHrrIR98NArgS/laN+Lz7LfaJW1r0gIa7pCmTi4l5thV80vDu\n"
    "RlfwJOr4qaucD4Du+mg5WxdSSdiXL6sBlarRtVdMaMy2dTqTegJDgShJLxHTt/3q\n"
    "P0yzBWJ5TtT3FG0XDqum/EkCgYAYNHwWWe3bQGQ9P9BI/fOL/YUZYu2sA1XAuKXZ\n"
    "0rsMhJ0dwvG76XkjGhitbe82rQZqsnvLZ3qn8HHmtOFBLkQfGtT3K8nGOUuI42eF\n"
    "H7HZKUCly2lCIizZdDVBkz4AWvaJlRc/3lE2Hd3Es6E52kTvROVKhdz06xuS8t5j\n"
    "6twqKQKBgQC01AeiWL6Rzo+yZNzVgbpeeDogaZz5dtmURDgCYH8yFX5eoCKLHfnI\n"
    "2nDIoqpaHY0LuX+dinuH+jP4tlyndbc2muXnHd9r0atytxA69ay3sSA5WFtfi4ef\n"
    "ESElGO6qXEA821RpQp+2+uhL90+iC294cPqlS5LDmvTMypVDHzrxPQ==\n"
    "-----END RSA PRIVATE KEY-----\n";

static char *encode_base64_strdup(const char *input) {
    if (input == NULL) {
        return NULL;
    }
    size_t input_len = strnlen(input, 64 * 1024);
    size_t output_len = 0;
    int rc = mbedtls_base64_encode(NULL, 0, &output_len, (const unsigned char *)input, input_len);
    assert(rc == MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL);

    unsigned char *buffer = (unsigned char *)malloc(output_len + 1);
    assert(buffer != NULL);

    rc = mbedtls_base64_encode(buffer, output_len, &output_len, (const unsigned char *)input, input_len);
    assert(rc == 0);
    buffer[output_len] = '\0';
    return (char *)buffer;
}

static volatile bool g_received = false;

static void tls_sample_handler(z_loaned_sample_t *sample, void *ctx) {
    const char *expected = (const char *)ctx;

    z_owned_string_t payload;
    assert(z_bytes_to_string(z_sample_payload(sample), &payload) == Z_OK);
    assert(expected != NULL);
    assert(strcmp(expected, z_string_data(z_loan(payload))) == 0);

    g_received = true;
    z_drop(z_move(payload));
}

int main(void) {
    char locator_buf[64];
    snprintf(locator_buf, sizeof(locator_buf), "tls/127.0.0.1:7447");
    const char *locator = locator_buf;
    static const char *const keyexpr_str = "test/tls/config";
    static const char *const payload_str = "tls-config-ok";

    char *ca_base64 = encode_base64_strdup(SERVER_CA_PEM);
    char *cert_base64 = encode_base64_strdup(SERVER_CERT_PEM);
    char *key_base64 = encode_base64_strdup(SERVER_KEY_PEM);

    z_owned_config_t listen_cfg;
    z_config_default(&listen_cfg);
    zp_config_insert(z_loan_mut(listen_cfg), Z_CONFIG_MODE_KEY, Z_CONFIG_MODE_PEER);
    zp_config_insert(z_loan_mut(listen_cfg), Z_CONFIG_LISTEN_KEY, locator);
    zp_config_insert(z_loan_mut(listen_cfg), Z_CONFIG_TLS_ROOT_CA_CERTIFICATE_BASE64_KEY, ca_base64);
    zp_config_insert(z_loan_mut(listen_cfg), Z_CONFIG_TLS_LISTEN_CERTIFICATE_BASE64_KEY, cert_base64);
    zp_config_insert(z_loan_mut(listen_cfg), Z_CONFIG_TLS_LISTEN_PRIVATE_KEY_BASE64_KEY, key_base64);

    z_owned_session_t server;
    z_result_t res = z_open(&server, z_move(listen_cfg), NULL);
    if (res != Z_OK) {
        fprintf(stderr, "server z_open failed: %d\n", res);
        return 1;
    }
    assert(zp_start_read_task(z_loan_mut(server), NULL) == Z_OK);
    assert(zp_start_lease_task(z_loan_mut(server), NULL) == Z_OK);
    (void)z_sleep_ms(50);

    z_view_keyexpr_t keyexpr;
    assert(z_view_keyexpr_from_str(&keyexpr, keyexpr_str) == Z_OK);

    z_owned_closure_sample_t callback;
    z_closure(&callback, tls_sample_handler, NULL, (void *)payload_str);
    z_owned_subscriber_t subscriber;
    assert(z_declare_subscriber(z_loan(server), &subscriber, z_loan(keyexpr), z_move(callback), NULL) == Z_OK);

    z_owned_config_t connect_cfg;
    z_config_default(&connect_cfg);
    zp_config_insert(z_loan_mut(connect_cfg), Z_CONFIG_CONNECT_KEY, locator);
    zp_config_insert(z_loan_mut(connect_cfg), Z_CONFIG_TLS_ROOT_CA_CERTIFICATE_BASE64_KEY, ca_base64);
    zp_config_insert(z_loan_mut(connect_cfg), Z_CONFIG_TLS_VERIFY_NAME_ON_CONNECT_KEY, "false");

    z_owned_session_t client;
    res = z_open(&client, z_move(connect_cfg), NULL);
    if (res != Z_OK) {
        fprintf(stderr, "client z_open failed: %d\n", res);
        return 1;
    }
    assert(zp_start_read_task(z_loan_mut(client), NULL) == Z_OK);
    assert(zp_start_lease_task(z_loan_mut(client), NULL) == Z_OK);
    (void)z_sleep_ms(50);

    z_owned_publisher_t publisher;
    assert(z_declare_publisher(z_loan(client), &publisher, z_loan(keyexpr), NULL) == Z_OK);

    z_owned_bytes_t payload;
    z_bytes_copy_from_str(&payload, payload_str);
    assert(z_publisher_put(z_loan(publisher), z_move(payload), NULL) == Z_OK);

    for (int i = 0; (i < 200) && !g_received; ++i) {
        z_sleep_ms(10);
    }
    assert(g_received == true);

    z_drop(z_move(publisher));
    z_drop(z_move(subscriber));
    z_session_drop(z_session_move(&client));
    z_session_drop(z_session_move(&server));

    free(key_base64);
    free(cert_base64);
    free(ca_base64);

    return 0;
}

#else

int main(void) {
    printf("TLS feature not enabled, skipping test\n");
    return 0;
}

#endif
