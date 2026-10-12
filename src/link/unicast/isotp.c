//
// Copyright (c) 2026 ZettaScale Technology
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

#include "zenoh-pico/link/config/isotp.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/link/transport/isotp.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_LINK_ISOTP == 1

#define _Z_ISOTP_DEV_MAX 32

#define _Z_CAN_SFF_MASK 0x000007FFu
#define _Z_CAN_EFF_MASK 0x1FFFFFFFu

z_result_t _z_endpoint_isotp_valid(_z_endpoint_t *endpoint) {
    z_result_t ret = _Z_RES_OK;

    _z_string_t isotp_str = _z_string_alias_str(ISOTP_SCHEMA);
    if (!_z_string_equals(&endpoint->_locator._protocol, &isotp_str)) {
        _Z_ERROR_LOG(_Z_ERR_CONFIG_LOCATOR_INVALID);
        ret = _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    if (ret == _Z_RES_OK) {
        size_t addr_len = _z_string_len(&endpoint->_locator._address);
        // The address is the interface name. Empty is meaningless and an
        // over-long one would truncate into a different interface, so both are
        // rejected here rather than at open, where the error is less obvious.
        if ((addr_len == 0) || (addr_len >= _Z_ISOTP_DEV_MAX)) {
            _Z_ERROR_LOG(_Z_ERR_CONFIG_LOCATOR_INVALID);
            ret = _Z_ERR_CONFIG_LOCATOR_INVALID;
        }
    }

    return ret;
}

// Read one unsigned integer config value, accepting decimal and 0x-prefixed
// hex. Identifiers are conventionally written in hex.
static uint32_t __z_isotp_cfg_u32(const _z_str_intmap_t *cfg, uint8_t key, uint32_t dflt, _Bool *found) {
    char *s = _z_str_intmap_get(cfg, key);
    if (found != NULL) {
        *found = (s != NULL);
    }
    if (s == NULL) {
        return dflt;
    }

    uint32_t base = 10;
    const char *p = s;
    if ((p[0] == '0') && ((p[1] == 'x') || (p[1] == 'X'))) {
        base = 16;
        p += 2;
    }

    uint32_t acc = 0;
    _Bool any = false;
    for (; *p != '\0'; p++) {
        uint32_t digit;
        if ((*p >= '0') && (*p <= '9')) {
            digit = (uint32_t)(*p - '0');
        } else if ((base == 16) && (*p >= 'a') && (*p <= 'f')) {
            digit = (uint32_t)(*p - 'a') + 10u;
        } else if ((base == 16) && (*p >= 'A') && (*p <= 'F')) {
            digit = (uint32_t)(*p - 'A') + 10u;
        } else {
            if (found != NULL) {
                *found = false;  // malformed reads as absent rather than half-parsed
            }
            return dflt;
        }
        if (acc > ((UINT32_MAX - digit) / base)) {
            if (found != NULL) {
                *found = false;
            }
            return dflt;
        }
        acc = (acc * base) + digit;
        any = true;
    }

    if ((found != NULL) && !any) {
        *found = false;
    }
    return any ? acc : dflt;
}

static z_result_t __z_isotp_open(_z_link_t *self) {
    char dev[_Z_ISOTP_DEV_MAX];
    size_t addr_len = _z_string_len(&self->_endpoint._locator._address);
    if (addr_len >= sizeof(dev)) {
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }
    memcpy(dev, _z_string_data(&self->_endpoint._locator._address), addr_len);
    dev[addr_len] = '\0';

    const _z_str_intmap_t *cfg = &self->_endpoint._config;
    _Bool has_tx = false;
    _Bool has_rx = false;
    uint32_t tx_id = __z_isotp_cfg_u32(cfg, ISOTP_CONFIG_TX_ID_KEY, 0, &has_tx);
    uint32_t rx_id = __z_isotp_cfg_u32(cfg, ISOTP_CONFIG_RX_ID_KEY, 0, &has_rx);
    _Bool eff = (__z_isotp_cfg_u32(cfg, ISOTP_CONFIG_EFF_KEY, 0, NULL) != 0u);
    // Both are a single octet of the flow control frame, so a wider value is a
    // typo rather than an intent. Clamping would hide it; refusing names it.
    uint32_t stmin_cfg = __z_isotp_cfg_u32(cfg, ISOTP_CONFIG_STMIN_KEY, 0, NULL);
    uint32_t bs_cfg = __z_isotp_cfg_u32(cfg, ISOTP_CONFIG_BS_KEY, 0, NULL);
    if ((stmin_cfg > 0xFFu) || (bs_cfg > 0xFFu)) {
        _Z_ERROR("ISO-TP: %s and %s are one byte each of the flow control frame, so 0..255", ISOTP_CONFIG_STMIN_STR,
                 ISOTP_CONFIG_BS_STR);
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }
    uint8_t stmin = (uint8_t)stmin_cfg;
    uint8_t bs = (uint8_t)bs_cfg;

    // Both are required. Defaulting either one would produce a link that opens
    // and then silently never communicates.
    if (!has_tx || !has_rx) {
        _Z_ERROR("ISO-TP: both %s and %s are required", ISOTP_CONFIG_TX_ID_STR, ISOTP_CONFIG_RX_ID_STR);
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    // ISO-TP addresses a peer by a DIRECTED pair; one identifier for both
    // directions would have this peer answering its own flow control.
    if (tx_id == rx_id) {
        _Z_ERROR("ISO-TP: %s and %s are both 0x%x, but they must be a directed pair", ISOTP_CONFIG_TX_ID_STR,
                 ISOTP_CONFIG_RX_ID_STR, (unsigned)tx_id);
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    uint32_t max = eff ? _Z_CAN_EFF_MASK : _Z_CAN_SFF_MASK;
    if ((tx_id > max) || (rx_id > max)) {
        _Z_ERROR("ISO-TP: identifier above the %s maximum 0x%x", eff ? "29-bit" : "11-bit", (unsigned)max);
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    z_result_t ret = _z_open_isotp(&self->_socket._isotp, dev, tx_id, rx_id, eff, stmin, bs);
    if (ret != _Z_RES_OK) {
        return ret;
    }

    self->_mtu = _Z_ISOTP_MTU_SIZE;
    return _Z_RES_OK;
}

z_result_t _z_f_link_open_isotp(_z_link_t *self) { return __z_isotp_open(self); }

// Registered on the CONNECT side only, so this is never reached. See
// _z_new_link_isotp for why.
z_result_t _z_f_link_listen_isotp(_z_link_t *self) {
    _ZP_UNUSED(self);
    _Z_ERROR("ISO-TP: this link connects out and does not listen");
    return _Z_ERR_GENERIC;
}

void _z_f_link_close_isotp(_z_link_t *self) { _z_close_isotp(&self->_socket._isotp); }

void _z_f_link_free_isotp(_z_link_t *self) { _ZP_UNUSED(self); }

size_t _z_f_link_write_isotp(const _z_link_t *self, const uint8_t *ptr, size_t len, _z_sys_net_socket_t *socket) {
    _ZP_UNUSED(socket);
    return _z_send_isotp(&self->_socket._isotp, ptr, len);
}

size_t _z_f_link_write_all_isotp(const _z_link_t *self, const uint8_t *ptr, size_t len) {
    // One write is one PDU; the platform segments it and observes the peer's
    // flow control, so there is no partial write to loop over.
    return _z_send_isotp(&self->_socket._isotp, ptr, len);
}

size_t _z_f_link_read_isotp(const _z_link_t *self, uint8_t *ptr, size_t len, _z_slice_t *addr) {
    // An ISO-TP channel has exactly one peer, so there is no sender to report.
    _ZP_UNUSED(addr);
    return _z_read_isotp(&self->_socket._isotp, ptr, len);
}

size_t _z_f_link_read_exact_isotp(const _z_link_t *self, uint8_t *ptr, size_t len, _z_slice_t *addr,
                                  _z_sys_net_socket_t *socket) {
    _ZP_UNUSED(addr);
    _ZP_UNUSED(socket);
    // "Exact" and "best effort" collapse on a message-preserving link: one call
    // returns one whole PDU or it fails.
    return _z_read_isotp(&self->_socket._isotp, ptr, len);
}

// Reached on every inbound batch. This was originally a stub that logged and
// returned SIZE_MAX, copied from the CAN link, where reads must filter on the
// receive identifier and so cannot go through a bare descriptor. ISO-TP has no
// such need -- the identifier pair is bound into the socket -- and being a
// UNICAST link it goes down the transport's _read_socket_f path, which the
// multicast CAN link never touches.
size_t _z_f_link_read_socket_isotp(const _z_sys_net_socket_t socket, uint8_t *ptr, size_t len) {
    return _z_read_isotp_socket(socket, ptr, len);
}

uint16_t _z_get_link_mtu_isotp(void) { return _Z_ISOTP_MTU_SIZE; }

z_result_t _z_new_link_isotp(_z_link_t *zl, _z_endpoint_t endpoint) {
    zl->_type = _Z_LINK_TYPE_ISOTP;
    // UNICAST, and that is the entire point. zenoh routes queries and liveliness
    // only to unicast faces, so a multicast CAN link cannot carry ROS services,
    // actions, parameters or graph introspection and this one can. ISO-TP earns
    // the classification honestly: a directed identifier pair with flow control
    // is point-to-point by construction.
    zl->_cap._transport = Z_LINK_CAP_TRANSPORT_UNICAST;
    // ISO-TP preserves message boundaries: one write is one PDU.
    zl->_cap._flow = Z_LINK_CAP_FLOW_DATAGRAM;
    // Flow control paces the sender, but a lost consecutive frame aborts the
    // whole PDU and nothing below zenoh recovers it. Same call the serial link
    // makes.
    zl->_cap._is_reliable = false;

    zl->_mtu = _z_get_link_mtu_isotp();
    zl->_endpoint = endpoint;

    zl->_open_f = _z_f_link_open_isotp;
    zl->_listen_f = _z_f_link_listen_isotp;
    zl->_close_f = _z_f_link_close_isotp;
    zl->_free_f = _z_f_link_free_isotp;

    zl->_write_f = _z_f_link_write_isotp;
    zl->_write_all_f = _z_f_link_write_all_isotp;
    zl->_read_f = _z_f_link_read_isotp;
    zl->_read_exact_f = _z_f_link_read_exact_isotp;
    zl->_read_socket_f = _z_f_link_read_socket_isotp;

    return _Z_RES_OK;
}

#endif
