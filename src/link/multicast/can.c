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

#include "zenoh-pico/link/config/can.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/link/transport/can.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_LINK_CAN == 1

#define _Z_CAN_DEV_MAX 32

z_result_t _z_endpoint_can_valid(_z_endpoint_t *endpoint) {
    z_result_t ret = _Z_RES_OK;

    _z_string_t can_str = _z_string_alias_str(CAN_SCHEMA);
    if (!_z_string_equals(&endpoint->_locator._protocol, &can_str)) {
        _Z_ERROR_LOG(_Z_ERR_CONFIG_LOCATOR_INVALID);
        ret = _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    if (ret == _Z_RES_OK) {
        size_t addr_len = _z_string_len(&endpoint->_locator._address);
        // The address is the interface name: "can0", "vcan0", or a Zephyr
        // devicetree label. Empty is meaningless and an over-long one would
        // silently truncate into a different interface, so reject both here
        // rather than at open() where the error is less obvious.
        if ((addr_len == 0) || (addr_len >= _Z_CAN_DEV_MAX)) {
            _Z_ERROR_LOG(_Z_ERR_CONFIG_LOCATOR_INVALID);
            ret = _Z_ERR_CONFIG_LOCATOR_INVALID;
        }
    }

    return ret;
}

// Read one unsigned integer config value, accepting decimal and 0x-prefixed
// hex -- identifiers are conventionally written in hex, bit rates in decimal,
// and forcing one notation on both would make every endpoint harder to read.
static uint32_t __z_can_cfg_u32(const _z_str_intmap_t *cfg, uint8_t key, uint32_t dflt) {
    char *s = _z_str_intmap_get(cfg, key);
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
            // Malformed: fall back to the default rather than half-parsing.
            return dflt;
        }
        if (acc > ((UINT32_MAX - digit) / base)) {
            return dflt;  // overflow
        }
        acc = (acc * base) + digit;
        any = true;
    }

    return any ? acc : dflt;
}

static z_result_t __z_can_open(_z_link_t *self) {
    char dev[_Z_CAN_DEV_MAX];
    size_t addr_len = _z_string_len(&self->_endpoint._locator._address);
    if (addr_len >= sizeof(dev)) {
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }
    memcpy(dev, _z_string_data(&self->_endpoint._locator._address), addr_len);
    dev[addr_len] = '\0';

    const _z_str_intmap_t *cfg = &self->_endpoint._config;
    uint32_t bitrate = __z_can_cfg_u32(cfg, CAN_CONFIG_BITRATE_KEY, CAN_CONFIG_DEFAULT_BITRATE);
    uint32_t dbitrate = __z_can_cfg_u32(cfg, CAN_CONFIG_DBITRATE_KEY, CAN_CONFIG_DEFAULT_DBITRATE);
    uint32_t id = __z_can_cfg_u32(cfg, CAN_CONFIG_ID_KEY, CAN_CONFIG_DEFAULT_ID);
    uint32_t match = __z_can_cfg_u32(cfg, CAN_CONFIG_MATCH_KEY, CAN_CONFIG_DEFAULT_MATCH);
    uint32_t mask = __z_can_cfg_u32(cfg, CAN_CONFIG_MASK_KEY, CAN_CONFIG_DEFAULT_MASK);

    // A peer that filtered out its own identifier would never be reachable by
    // anyone, which is a configuration error rather than a quiet degradation.
    if ((mask != 0u) && ((id & mask) != match)) {
        _Z_ERROR("CAN: id 0x%x is outside its own match/mask band", (unsigned)id);
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    z_result_t ret = _z_open_can(&self->_socket._can, dev, bitrate, dbitrate, id, match, mask);
    if (ret != _Z_RES_OK) {
        return ret;
    }

    // The platform reports the mode it actually obtained; a preconfigured
    // interface may not be in the mode the endpoint asked for. Track the MTU
    // from what we got, not from what we requested -- declaring 63 on a
    // classic-CAN interface would silently truncate every frame.
    self->_mtu = self->_socket._can._mtu;
    return _Z_RES_OK;
}

z_result_t _z_f_link_open_can(_z_link_t *self) { return __z_can_open(self); }

// Multicast peers all listen; there is no connect/accept pairing on a bus.
z_result_t _z_f_link_listen_can(_z_link_t *self) { return __z_can_open(self); }

void _z_f_link_close_can(_z_link_t *self) { _z_close_can(&self->_socket._can); }

void _z_f_link_free_can(_z_link_t *self) { _ZP_UNUSED(self); }

size_t _z_f_link_write_can(const _z_link_t *self, const uint8_t *ptr, size_t len, _z_sys_net_socket_t *socket) {
    _ZP_UNUSED(socket);
    return _z_send_can(&self->_socket._can, ptr, len);
}

size_t _z_f_link_write_all_can(const _z_link_t *self, const uint8_t *ptr, size_t len) {
    // A datagram link writes one frame or fails; there is no partial write to
    // loop over. zenoh's transport never hands us more than `_mtu` because
    // tx.c clamps to min(link mtu, batch size) and fragments above it.
    return _z_send_can(&self->_socket._can, ptr, len);
}

size_t _z_f_link_read_can(const _z_link_t *self, uint8_t *ptr, size_t len, _z_slice_t *addr) {
    // `addr` carries the sender's identifier back to the multicast transport,
    // which uses it to tell peers apart (`_remote_addr` in multicast/rx.c).
    return _z_read_can(&self->_socket._can, ptr, len, addr);
}

size_t _z_f_link_read_exact_can(const _z_link_t *self, uint8_t *ptr, size_t len, _z_slice_t *addr,
                                _z_sys_net_socket_t *socket) {
    _ZP_UNUSED(socket);
    // "Exact" and "best-effort" collapse on a datagram link: one call returns
    // one whole datagram or fails. Same reasoning as any datagram link.
    return _z_read_can(&self->_socket._can, ptr, len, addr);
}

size_t _z_f_link_read_socket_can(const _z_sys_net_socket_t socket, uint8_t *ptr, size_t len) {
    // Wired for symmetry with the other transports. A CAN link dispatches
    // through its own callbacks because the read must filter on the receive
    // identifier, which a bare socket read cannot do -- reaching here is a
    // logic bug at the call site.
    _ZP_UNUSED(socket);
    _ZP_UNUSED(ptr);
    _ZP_UNUSED(len);
    _Z_ERROR("CAN: _z_f_link_read_socket_can must not be called");
    return SIZE_MAX;
}

uint16_t _z_get_link_mtu_can(void) { return _Z_CAN_MTU_SIZE; }

z_result_t _z_new_link_can(_z_link_t *zl, _z_endpoint_t endpoint) {
    zl->_type = _Z_LINK_TYPE_CAN;
    // A CAN bus is a broadcast medium -- every node hears every frame and
    // filters by identifier. Declaring UNICAST instead routes the listen side
    // through _zp_unicast_accept_task, which needs a socket and an accept()
    // that no datagram medium has, and the handshake never completes.
    zl->_cap._transport = Z_LINK_CAP_TRANSPORT_MULTICAST;
    // CAN frames are bounded and self-delimiting, which is exactly what a
    // datagram link is. Declaring STREAM instead would oblige this link to
    // build segmentation and reassembly internally, and zenoh would then
    // fragment on top of it.
    zl->_cap._flow = Z_LINK_CAP_FLOW_DATAGRAM;
    // Reliable at frame level -- CRC, ACK slot, automatic retransmission -- but
    // not end to end: controller buffers overrun and bus-off drops everything.
    // Let zenoh's own reliability cover that.
    zl->_cap._is_reliable = false;

    // Provisional. open()/listen() replaces this with the MTU for the mode the
    // interface actually came up in.
    zl->_mtu = _z_get_link_mtu_can();

    zl->_endpoint = endpoint;

    zl->_open_f = _z_f_link_open_can;
    zl->_listen_f = _z_f_link_listen_can;
    zl->_close_f = _z_f_link_close_can;
    zl->_free_f = _z_f_link_free_can;

    zl->_write_f = _z_f_link_write_can;
    zl->_write_all_f = _z_f_link_write_all_can;
    zl->_read_f = _z_f_link_read_can;
    zl->_read_exact_f = _z_f_link_read_exact_can;
    zl->_read_socket_f = _z_f_link_read_socket_can;

    return _Z_RES_OK;
}

#endif
