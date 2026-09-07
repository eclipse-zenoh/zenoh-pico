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

// Zephyr has its own ISO-TP in `subsys/canbus/isotp`, and this port uses it
// rather than the vendored `third_party/isotp-c`. That is the same rule the
// unix port follows with the kernel's CAN_ISOTP socket: where the platform
// implements the protocol, the platform's implementation wins. It is tested by
// its own conformance suite, it is maintained with the CAN drivers it sits on,
// and it is what a Zephyr application would be using anyway.
//
// CONFIG_ISOTP must be set. Zephyr marks it [EXPERIMENTAL] in
// Kconfig, and its own test suite skips the STmin timing cases, which is
// worth knowing before an island depends on services over this link.

#include "zenoh-pico/config.h"

#if Z_FEATURE_LINK_ISOTP == 1

#include <string.h>
#include <zephyr/canbus/isotp.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>

#include "zenoh-pico/link/transport/isotp.h"
#include "zenoh-pico/utils/logging.h"

// Zephyr's contexts are large (each carries a buffer queue and a work item) and
// must outlive every call, so they live here rather than in the socket. The
// socket holds an index.
#ifndef Z_ISOTP_MAX_LINKS
#define Z_ISOTP_MAX_LINKS 2
#endif

typedef struct {
    bool _taken;
    struct isotp_recv_ctx _recv;
    struct isotp_send_ctx _send;
    struct isotp_msg_id _tx_addr;
    struct isotp_msg_id _rx_addr;
    const struct device *_dev;
} _z_isotp_zslot_t;

static _z_isotp_zslot_t _z_isotp_zslots[Z_ISOTP_MAX_LINKS];
K_MUTEX_DEFINE(_z_isotp_zslot_mutex);

// Flow control this receiver asks the sender for. `bs = 0` means "send the
// whole PDU without stopping", matching what the Linux kernel asks for, so a
// Zephyr node and a Linux node pace each other the same way. `stmin = 0` leaves
// the separation time to the bus rather than adding software delay.
// Defaults when the endpoint asks for nothing: `bs = 0` says "send the whole
// PDU without stopping", matching what the Linux kernel asks for, so a Zephyr
// node and a Linux node pace each other the same way. `stmin = 0` leaves the
// separation time to the bus rather than adding software delay. An endpoint
// that sets `stmin` or `bs` overrides both, which is how a node with a slow
// loop or one frame of buffer keeps a faster peer from overrunning it.
static const struct isotp_fc_opts _z_isotp_fc = {.bs = 0, .stmin = 0};

static _z_isotp_zslot_t *__z_zslot_take(void) {
    _z_isotp_zslot_t *out = NULL;
    k_mutex_lock(&_z_isotp_zslot_mutex, K_FOREVER);
    for (int i = 0; i < Z_ISOTP_MAX_LINKS; i++) {
        if (!_z_isotp_zslots[i]._taken) {
            _z_isotp_zslots[i]._taken = true;
            out = &_z_isotp_zslots[i];
            break;
        }
    }
    k_mutex_unlock(&_z_isotp_zslot_mutex);
    return out;
}

static int __z_zslot_index(const _z_isotp_zslot_t *slot) { return (int)(slot - _z_isotp_zslots); }

// A Zephyr CAN controller does NOT transmit until `can_start()` has been
// called, and a send on a stopped controller does not report an error the
// ISO-TP layer can see: `isotp_send` queues the first frame, nothing reaches
// the bus, and the only symptom is `Reception of next FC has timed out` while
// `candump` shows an idle bus. That is exactly how this was found.
//
// Refcounted per device, the same way the multicast CAN port does it, because
// several links can share one controller: starting it twice is wrong, and so
// is one link's close stopping it under the others.
static const struct device *_z_isotp_started_dev[Z_ISOTP_MAX_LINKS];
static uint8_t _z_isotp_started_ref[Z_ISOTP_MAX_LINKS];
K_MUTEX_DEFINE(_z_isotp_start_mutex);

static bool __z_isotp_dev_acquire(const struct device *dev) {
    bool ok = true;
    k_mutex_lock(&_z_isotp_start_mutex, K_FOREVER);
    for (int i = 0; i < Z_ISOTP_MAX_LINKS; i++) {
        if (_z_isotp_started_dev[i] == dev) {
            _z_isotp_started_ref[i]++;
            k_mutex_unlock(&_z_isotp_start_mutex);
            return true;  // already running; do not restart it under the others
        }
    }
    for (int i = 0; i < Z_ISOTP_MAX_LINKS; i++) {
        if (_z_isotp_started_dev[i] == NULL) {
            int rc = can_start(dev);
            // -EALREADY means something else already started it, which is fine
            // and still ours to reference.
            if ((rc < 0) && (rc != -EALREADY)) {
                _Z_ERROR("ISO-TP: can_start failed: %d", rc);
                ok = false;
            } else {
                _z_isotp_started_dev[i] = dev;
                _z_isotp_started_ref[i] = 1;
            }
            k_mutex_unlock(&_z_isotp_start_mutex);
            return ok;
        }
    }
    k_mutex_unlock(&_z_isotp_start_mutex);
    _Z_ERROR("ISO-TP: no free device slot (Z_ISOTP_MAX_LINKS=%d)", Z_ISOTP_MAX_LINKS);
    return false;
}

static void __z_isotp_dev_release(const struct device *dev) {
    k_mutex_lock(&_z_isotp_start_mutex, K_FOREVER);
    for (int i = 0; i < Z_ISOTP_MAX_LINKS; i++) {
        if (_z_isotp_started_dev[i] == dev) {
            if (_z_isotp_started_ref[i] > 0) {
                _z_isotp_started_ref[i]--;
            }
            if (_z_isotp_started_ref[i] == 0) {
                (void)can_stop(dev);
                _z_isotp_started_dev[i] = NULL;
            }
            break;
        }
    }
    k_mutex_unlock(&_z_isotp_start_mutex);
}

z_result_t _z_open_isotp(_z_isotp_socket_t *sock, const char *dev, uint32_t tx_id, uint32_t rx_id, _Bool eff,
                         uint8_t stmin, uint8_t bs) {
    const struct device *can_dev = device_get_binding(dev);
    if ((can_dev == NULL) || !device_is_ready(can_dev)) {
        _Z_ERROR("ISO-TP: CAN device '%s' is absent or not ready", dev);
        return _Z_ERR_GENERIC;
    }

    _z_isotp_zslot_t *slot = __z_zslot_take();
    if (slot == NULL) {
        _Z_ERROR("ISO-TP: no free link slot (max %d)", Z_ISOTP_MAX_LINKS);
        return _Z_ERR_GENERIC;
    }

    slot->_dev = can_dev;

    // Only ISO-TP NORMAL addressing, on purpose: extended and mixed addressing
    // are a deliberate non-goal, because no portable implementation provides
    // them and normal addressing is the interoperable common denominator. So
    // `ext_addr` stays 0 and ISOTP_MSG_EXT_ADDR is never set.
    //
    // `std_id` and `ext_id` are a UNION in Zephyr's isotp_msg_id -- 11 bits and
    // 29 bits over the same storage -- so exactly one is written, chosen by
    // ISOTP_MSG_IDE. Writing both would silently truncate the 29-bit value.
    // Addressing mode lives in `flags`; there is no `id_type` member. That was
    // the pre-3.7 API, and reaching for it fails to compile rather than
    // misbehaving at runtime -- which is how this was caught.
    memset(&slot->_tx_addr, 0, sizeof(slot->_tx_addr));
    memset(&slot->_rx_addr, 0, sizeof(slot->_rx_addr));
    slot->_tx_addr.flags = eff ? ISOTP_MSG_IDE : 0;
    slot->_rx_addr.flags = slot->_tx_addr.flags;
    if (eff) {
        slot->_tx_addr.ext_id = tx_id;
        slot->_rx_addr.ext_id = rx_id;
    } else {
        slot->_tx_addr.std_id = tx_id;
        slot->_rx_addr.std_id = rx_id;
    }
    // `dl` is left 0, which Zephyr reads as 8: classical CAN framing. That is
    // the point of this link -- ISO-TP is what makes an 8-byte frame carry a
    // 4095-byte PDU, and classical CAN is most of the hardware in the field.

    // Bind the RECEIVE side once, at open. Zephyr installs a CAN filter here,
    // and a controller has few filter slots -- binding per read would exhaust
    // them and would also drop everything that arrived between reads.
    if (!__z_isotp_dev_acquire(can_dev)) {
        slot->_taken = false;
        return _Z_ERR_GENERIC;
    }

    const struct isotp_fc_opts fc =
        ((stmin != 0u) || (bs != 0u)) ? (struct isotp_fc_opts){.bs = bs, .stmin = stmin} : _z_isotp_fc;
    int ret = isotp_bind(&slot->_recv, can_dev, &slot->_rx_addr, &slot->_tx_addr, &fc, K_FOREVER);
    if (ret != ISOTP_N_OK) {
        _Z_ERROR("ISO-TP: isotp_bind(rx=0x%x tx=0x%x) failed: %d", (unsigned)rx_id, (unsigned)tx_id, ret);
        __z_isotp_dev_release(can_dev);
        slot->_taken = false;
        return _Z_ERR_GENERIC;
    }

    sock->_sock._isotp_dev = can_dev;
    sock->_sock._isotp_slot = __z_zslot_index(slot);
    sock->_tx_id = tx_id;
    sock->_rx_id = rx_id;
    sock->_eff = eff;
    return _Z_RES_OK;
}

void _z_close_isotp(_z_isotp_socket_t *sock) {
    int idx = sock->_sock._isotp_slot;
    if ((idx < 0) || (idx >= Z_ISOTP_MAX_LINKS)) {
        return;
    }
    _z_isotp_zslot_t *slot = &_z_isotp_zslots[idx];
    if (slot->_taken) {
        isotp_unbind(&slot->_recv);  // releases the controller's filter slot
        __z_isotp_dev_release(slot->_dev);
        slot->_taken = false;
    }
    sock->_sock._isotp_slot = -1;
}

size_t _z_read_isotp_socket(const _z_sys_net_socket_t socket, uint8_t *ptr, size_t len) {
    int idx = socket._isotp_slot;
    if ((idx < 0) || (idx >= Z_ISOTP_MAX_LINKS) || !_z_isotp_zslots[idx]._taken) {
        _Z_ERROR("ISO-TP: read on a socket with no bound link");
        return SIZE_MAX;
    }
    // K_FOREVER: this is the transport's read task and it is expected to block.
    int ret = isotp_recv(&_z_isotp_zslots[idx]._recv, ptr, len, K_FOREVER);
    if (ret < 0) {
        _Z_ERROR("ISO-TP: isotp_recv failed: %d", ret);
        return SIZE_MAX;
    }
    return (size_t)ret;
}

size_t _z_read_isotp(const _z_isotp_socket_t *sock, uint8_t *ptr, size_t len) {
    return _z_read_isotp_socket(sock->_sock, ptr, len);
}

size_t _z_send_isotp(const _z_isotp_socket_t *sock, const uint8_t *ptr, size_t len) {
    if (len > _Z_ISOTP_MTU_SIZE) {
        _Z_ERROR("ISO-TP: PDU %zu exceeds the 12-bit FF_DL limit of %d", len, _Z_ISOTP_MTU_SIZE);
        return SIZE_MAX;
    }
    int idx = sock->_sock._isotp_slot;
    if ((idx < 0) || (idx >= Z_ISOTP_MAX_LINKS) || !_z_isotp_zslots[idx]._taken) {
        _Z_ERROR("ISO-TP: send on a socket with no bound link");
        return SIZE_MAX;
    }
    _z_isotp_zslot_t *slot = &_z_isotp_zslots[idx];

    // A NULL completion callback makes isotp_send BLOCK until the whole PDU is
    // out, which is the contract the link expects: a send that returns is a
    // send the peer has paced through flow control. It is also what settles
    // N_As on this platform -- the transmit confirmation is Zephyr's problem,
    // inside the CAN driver, rather than something this port has to time.
    int ret = isotp_send(&slot->_send, slot->_dev, ptr, len, &slot->_tx_addr, &slot->_rx_addr, NULL, NULL);
    if (ret != ISOTP_N_OK) {
        _Z_ERROR("ISO-TP: isotp_send of %zu bytes failed: %d", len, ret);
        return SIZE_MAX;
    }
    return len;
}

#endif  // Z_FEATURE_LINK_ISOTP == 1
