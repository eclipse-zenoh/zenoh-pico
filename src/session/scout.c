//
// Copyright (c) 2022 ZettaScale Technology
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

#include <stddef.h>
#include <string.h>

#include "zenoh-pico/link/config/udp.h"
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/link/transport/socket.h"
#include "zenoh-pico/link/transport/udp_unicast.h"
#include "zenoh-pico/protocol/codec/transport.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_SCOUTING == 1

#define SCOUT_BUFFER_SIZE 32

#define _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE _z_link_t
#define _ZP_STATIC_VECTOR_TEMPLATE_NAME _z_scout_links
#define _ZP_STATIC_VECTOR_TEMPLATE_SIZE Z_MAX_NUM_SCOUT_INTERFACES
#define _ZP_STATIC_VECTOR_TEMPLATE_ELEM_DESTROY_FN _z_link_clear
#include "zenoh-pico/collections/static_vector_template.h"

#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME _z_scout_ready_mask
#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_SIZE Z_MAX_NUM_SCOUT_INTERFACES
#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_IS_SET 1
#include "zenoh-pico/collections/static_bit_vector_template.h"

typedef struct {
    _z_scout_links_t _links;
    _z_wbuf_t _tx_buffer;
    _z_zbuf_t _rx_buffer;
} _z_scout_t;

static z_result_t _z_scout_open_link(_z_link_t *link, const _z_string_t *locator, const char *iface) {
    memset(link, 0, sizeof(*link));
    if (iface == NULL) {
        return _z_open_link(link, locator, NULL);
    }

    _z_endpoint_t endpoint;
    _Z_RETURN_IF_ERR(_z_endpoint_from_string(&endpoint, locator));

    char *iface_name = _z_str_clone(iface);
    if ((iface_name == NULL) || (_z_str_intmap_insert(&endpoint._config, UDP_CONFIG_IFACE_KEY, iface_name) == NULL)) {
        z_free(iface_name);
        _z_endpoint_clear(&endpoint);
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }

    z_result_t ret = _z_new_link_udp_unicast(link, endpoint);
    if (ret != _Z_RES_OK) {
        _z_endpoint_clear(&endpoint);
        return ret;
    }

    ret = link->_open_f(link);
    if (ret != _Z_RES_OK) {
        _z_link_clear(link);
    }
    return ret;
}

static z_result_t _z_scout_add_link(_z_scout_t *scout, const _z_string_t *locator, const char *iface) {
    if (_z_scout_links_size(&scout->_links) == _z_scout_links_capacity(&scout->_links)) {
        return _Z_ERR_TRANSPORT_NO_SPACE;
    }

    _z_link_t link;
    _Z_RETURN_IF_ERR(_z_scout_open_link(&link, locator, iface));
    if (!_z_scout_links_push_back(&scout->_links, &link)) {
        _z_link_clear(&link);
        return _Z_ERR_TRANSPORT_NO_SPACE;
    }
    return _Z_RES_OK;
}

static z_result_t _z_scout_init_links(_z_scout_t *scout, const _z_string_t *locator) {
    _z_scout_links_init(&scout->_links);

    _z_endpoint_t endpoint;
    _Z_RETURN_IF_ERR(_z_endpoint_from_string(&endpoint, locator));
    _z_string_t udp = _z_string_alias_str(UDP_SCHEMA);
    if (!_z_string_equals(&endpoint._locator._protocol, &udp)) {
        _z_endpoint_clear(&endpoint);
        return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
    }

#if defined(ZP_PLATFORM_HAS_UDP_INTERFACE_ITERATOR)
    const char *iface = _z_str_intmap_get(&endpoint._config, UDP_CONFIG_IFACE_KEY);
    if (iface != NULL) {
#endif
        z_result_t ret = _z_scout_add_link(scout, locator, NULL);
        _z_endpoint_clear(&endpoint);
        return ret;
#if defined(ZP_PLATFORM_HAS_UDP_INTERFACE_ITERATOR)
    }

    _z_udp_unicast_interface_iterator_t iter;
    z_result_t ret = _z_udp_unicast_interface_iterator_init(&iter, &endpoint._locator._address);
    if (ret != _Z_RES_OK) {
        _z_endpoint_clear(&endpoint);
        return ret;
    }

    while (_z_udp_unicast_interface_iterator_next(&iter)) {
        const char *iface_name = _z_udp_unicast_interface_iterator_deref(&iter);
        if (_z_scout_links_size(&scout->_links) == _z_scout_links_capacity(&scout->_links)) {
            _Z_WARN("Scouting link limit reached, ignoring interface: %s", iface_name);
            continue;
        }

        ret = _z_scout_add_link(scout, locator, iface_name);
        if (ret == _Z_RES_OK) {
            _Z_DEBUG("Opened scout link on interface: %s", iface_name);
        } else {
            _Z_ERROR("Failed to open scout link on interface: %s (error: %d)", iface_name, ret);
        }
    }
    _z_udp_unicast_interface_iterator_clear(&iter);
    _z_endpoint_clear(&endpoint);

    return _z_scout_links_size(&scout->_links) > 0 ? _Z_RES_OK : _Z_ERR_TRANSPORT_OPEN_FAILED;
#endif
}

static z_result_t _z_scout_init(_z_scout_t *scout, const _z_string_t *locator) {
    memset(scout, 0, sizeof(*scout));

    scout->_tx_buffer = _z_wbuf_make(SCOUT_BUFFER_SIZE, false);
    if (_z_wbuf_capacity(&scout->_tx_buffer) < SCOUT_BUFFER_SIZE) {
        _z_wbuf_clear(&scout->_tx_buffer);
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }

    scout->_rx_buffer = _z_zbuf_make(Z_BATCH_UNICAST_SIZE);
    if (_z_zbuf_capacity(&scout->_rx_buffer) < Z_BATCH_UNICAST_SIZE) {
        _z_wbuf_clear(&scout->_tx_buffer);
        _z_zbuf_clear(&scout->_rx_buffer);
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }

    z_result_t ret = _z_scout_init_links(scout, locator);
    if (ret != _Z_RES_OK) {
        _z_wbuf_clear(&scout->_tx_buffer);
        _z_zbuf_clear(&scout->_rx_buffer);
        _z_scout_links_destroy(&scout->_links);
    }
    return ret;
}

static z_result_t _z_scout_send(_z_scout_t *scout, z_what_t what, const _z_id_t *zid) {
    _z_scouting_message_t message = _z_s_msg_make_scout(what, *zid);
    _z_wbuf_reset(&scout->_tx_buffer);
    _Z_RETURN_IF_ERR(_z_scouting_message_encode(&scout->_tx_buffer, &message));

    bool sent = false;
    for (size_t i = 0; i < _z_scout_links_size(&scout->_links); i++) {
        if (_z_link_send_wbuf(_z_scout_links_at(&scout->_links, i), &scout->_tx_buffer, NULL) != _Z_RES_OK) {
            _Z_ERROR("Failed to send scouting message on link %zu", i);
        } else {
            sent = true;
        }
    }
    return sent ? _Z_RES_OK : _Z_ERR_TRANSPORT_TX_FAILED;
}

static void _z_scout_clear(_z_scout_t *scout) {
    _z_wbuf_clear(&scout->_tx_buffer);
    _z_zbuf_clear(&scout->_rx_buffer);
    _z_scout_links_destroy(&scout->_links);
}

typedef struct {
    _z_scout_links_t *links;
    _z_scout_ready_mask_t *ready_mask;
    size_t pos;
} _z_scout_wait_readable_context_t;

static void _z_scout_wait_readable_reset(_z_socket_wait_iter_t *iter) {
    _z_scout_wait_readable_context_t *ctx = (_z_scout_wait_readable_context_t *)iter->_ctx;
    ctx->pos = 0;
    iter->_current_entry = NULL;
    _z_scout_ready_mask_set_all(ctx->ready_mask, false);
}

static bool _z_scout_wait_readable_next(_z_socket_wait_iter_t *iter) {
    _z_scout_wait_readable_context_t *ctx = (_z_scout_wait_readable_context_t *)iter->_ctx;
    if (ctx->pos >= _z_scout_links_size(ctx->links)) {
        iter->_current_entry = NULL;
        return false;
    }
    iter->_current_entry = _z_scout_links_at(ctx->links, ctx->pos);
    ctx->pos++;
    return true;
}

static const _z_sys_net_socket_t *_z_scout_wait_readable_get_socket(const _z_socket_wait_iter_t *iter) {
    return _z_link_get_socket((const _z_link_t *)iter->_current_entry);
}

static void _z_scout_wait_readable_set_ready(_z_socket_wait_iter_t *iter, bool ready) {
    _z_scout_wait_readable_context_t *ctx = (_z_scout_wait_readable_context_t *)iter->_ctx;
    _z_scout_ready_mask_set_at(ctx->ready_mask, ctx->pos - 1, ready);
}

static _z_scout_ready_mask_t _z_scout_wait_readable(_z_scout_t *scout) {
    _z_scout_ready_mask_t ready_mask = _z_scout_ready_mask_new();
    _z_scout_wait_readable_context_t ctx = {
        .links = &scout->_links,
        .ready_mask = &ready_mask,
        .pos = 0,
    };
    _z_socket_wait_iter_t iter = {
        ._ctx = &ctx,
        ._current_entry = NULL,
        ._reset = _z_scout_wait_readable_reset,
        ._next = _z_scout_wait_readable_next,
        ._get_socket = _z_scout_wait_readable_get_socket,
        ._set_ready = _z_scout_wait_readable_set_ready,
    };
    if (_z_socket_wait_readable(&iter, Z_CONFIG_SOCKET_TIMEOUT) < _Z_RES_OK) {
        _z_scout_ready_mask_set_all(&ready_mask, true);
    }
    return ready_mask;
}

z_result_t _z_scout_process_hello(const _z_s_msg_hello_t *message, _z_hello_slist_t **hellos, bool exit_on_first) {
    size_t locator_count = _z_locator_array_len(&message->_locators);
    if ((locator_count == 0) && exit_on_first) {
        return _Z_NO_DATA_PROCESSED;
    }

    _z_hello_slist_t *old_head = *hellos;
    *hellos = _z_hello_slist_push_empty(*hellos);
    if (old_head == *hellos) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }

    _z_hello_t *hello = _z_hello_slist_value(*hellos);
    hello->_version = message->_version;
    hello->_whatami = message->_whatami;
    // Flawfinder: ignore [CWE-120]
    memcpy(hello->_zid.id, message->_zid.id, Z_ZID_LENGTH);
    hello->_locators = _z_string_svec_make(locator_count);
    if (hello->_locators._capacity != locator_count) {
        *hellos = _z_hello_slist_pop(*hellos);
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }

    z_result_t ret = _Z_RES_OK;
    for (size_t i = 0; i < locator_count; i++) {
        _z_string_t locator = _z_locator_to_string(&message->_locators._val[i]);
        if (_z_string_is_empty(&locator) || (_z_string_svec_append(&hello->_locators, &locator, true) != _Z_RES_OK)) {
            _z_string_clear(&locator);
            ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;
            break;
        }
    }
    _Z_CLEAN_RETURN_IF_ERR(ret, *hellos = _z_hello_slist_pop(*hellos));
    return _Z_RES_OK;
}

static z_result_t _z_scout_recv(_z_scout_t *scout, size_t link_id, _z_hello_slist_t **hellos, bool exit_on_first) {
    _z_zbuf_reset(&scout->_rx_buffer);
    size_t len = _z_link_recv_zbuf(_z_scout_links_at(&scout->_links, link_id), &scout->_rx_buffer, NULL);
    if (len == SIZE_MAX) {
        return _Z_NO_DATA_PROCESSED;
    }

    _z_scouting_message_t message;
    _Z_RETURN_IF_ERR(_z_scouting_message_decode(&message, &scout->_rx_buffer));

    z_result_t ret = _Z_RES_OK;
    if (_Z_MID(message._header) == _Z_MID_HELLO) {
        ret = _z_scout_process_hello(&message._body._hello, hellos, exit_on_first);
    } else {
        ret = _Z_ERR_MESSAGE_UNEXPECTED;
    }
    _z_s_msg_clear(&message);
    return ret;
}

static z_result_t _z_scout_read(_z_scout_t *scout, _z_hello_slist_t **hellos, bool exit_on_first) {
    _z_scout_ready_mask_t ready = _z_scout_wait_readable(scout);
    for (size_t i = 0; i < _z_scout_links_size(&scout->_links); i++) {
        if (!*_z_scout_ready_mask_const_at(&ready, i)) {
            continue;
        }

        z_result_t ret = _z_scout_recv(scout, i, hellos, exit_on_first);
        if (ret < _Z_RES_OK) {
            _Z_ERROR("Failed to receive scouting message from link %zu: %d", i, ret);
        } else if (exit_on_first && (*hellos != NULL)) {
            break;
        }
    }
    return _Z_RES_OK;
}

_z_hello_slist_t *_z_scout_inner(const z_what_t what, _z_id_t zid, const _z_string_t *locator, const uint32_t timeout,
                                 const bool exit_on_first) {
    _z_hello_slist_t *hellos = NULL;

    _z_scout_t scout;
    if (_z_scout_init(&scout, locator) != _Z_RES_OK) {
        return NULL;
    }
    if (_z_scout_send(&scout, what, &zid) != _Z_RES_OK) {
        _z_scout_clear(&scout);
        return NULL;
    }

    z_clock_t start = z_clock_now();
    while (z_clock_elapsed_ms(&start) < timeout) {
        (void)_z_scout_read(&scout, &hellos, exit_on_first);
        if (exit_on_first && (hellos != NULL)) {
            break;
        }
    }

    _z_scout_clear(&scout);
    return hellos;
}

#else

_z_hello_slist_t *_z_scout_inner(const z_what_t what, _z_id_t zid, const _z_string_t *locator, const uint32_t timeout,
                                 const bool exit_on_first) {
    _ZP_UNUSED(what);
    _ZP_UNUSED(zid);
    _ZP_UNUSED(locator);
    _ZP_UNUSED(timeout);
    _ZP_UNUSED(exit_on_first);
    return NULL;
}

#endif  // Z_FEATURE_SCOUTING == 1
