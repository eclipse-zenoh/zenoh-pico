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

#include "zenoh-pico/transport/transport.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/link.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/transport/multicast/rx.h"
#include "zenoh-pico/transport/multicast/transport.h"
#include "zenoh-pico/transport/multicast/tx.h"
#include "zenoh-pico/transport/raweth/rx.h"
#include "zenoh-pico/transport/raweth/tx.h"
#include "zenoh-pico/transport/unicast/rx.h"
#include "zenoh-pico/transport/unicast/transport.h"
#include "zenoh-pico/transport/unicast/tx.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/utils/logging.h"

int8_t _z_send_close(_z_transport_t *zt, uint8_t reason, _Bool link_only) {
    int8_t ret = _Z_RES_OK;
    // Call transport function
    switch (zt->_type) {
        case _Z_TRANSPORT_UNICAST_TYPE:
            ret = _z_unicast_send_close(&zt->_transport._unicast, reason, link_only);
            break;
        case _Z_TRANSPORT_MULTICAST_TYPE:
        case _Z_TRANSPORT_RAWETH_TYPE:
            ret = _z_multicast_send_close(&zt->_transport._multicast, reason, link_only);
            break;
        default:
            ret = _Z_ERR_TRANSPORT_NOT_AVAILABLE;
            break;
    }
    return ret;
}

int8_t _z_transport_close(_z_transport_t *zt, uint8_t reason) { return _z_send_close(zt, reason, false); }

void _z_transport_clear(_z_transport_t *zt) {
    switch (zt->_type) {
        case _Z_TRANSPORT_UNICAST_TYPE:
            _z_unicast_transport_clear(zt);
            break;
        case _Z_TRANSPORT_MULTICAST_TYPE:
        case _Z_TRANSPORT_RAWETH_TYPE:
            _z_multicast_transport_clear(zt);
            break;
        default:
            break;
    }
}

void _z_transport_free(_z_transport_t **zt) {
    _z_transport_t *ptr = *zt;
    if (ptr == NULL) {
        return;
    }
    // Clear and free transport
    _z_transport_clear(ptr);
    z_free(ptr);
    *zt = NULL;
}

/**
 * @brief Inserts an entry into `root`, allocating it a `_peer_id`
 *
 * @param root the insertion root.
 * @param entry the entry to be inserted.
 * @return _z_transport_peer_entry_list_t* the new root, after inserting the entry.
 */
_z_transport_peer_entry_list_t *_z_transport_peer_entry_list_insert(_z_transport_peer_entry_list_t *root,
                                                                    _z_transport_peer_entry_t *entry) {
    if (root == NULL) {
        entry->_peer_id = 1;
        root = _z_transport_peer_entry_list_push(root, entry);
    } else {
        _z_transport_peer_entry_t *head = _z_transport_peer_entry_list_head(root);
        if (head->_peer_id + 1 < _Z_KEYEXPR_MAPPING_UNKNOWN_REMOTE) {
            entry->_peer_id = head->_peer_id + 1;
            root = _z_transport_peer_entry_list_push(root, entry);
        } else {
            _z_transport_peer_entry_list_t *parent = root;
            uint16_t target = head->_peer_id - 1;
            while (parent->_tail != NULL) {
                _z_transport_peer_entry_list_t *list = _z_transport_peer_entry_list_tail(parent);
                head = _z_transport_peer_entry_list_head(list);
                if (head->_peer_id < target) {
                    entry->_peer_id = head->_peer_id + 1;
                    parent->_tail = _z_transport_peer_entry_list_push(list, entry);
                    return root;
                }
                parent = list;
                target = head->_peer_id - 1;
            }
            assert(target > 0);
            entry->_peer_id = 1;
            parent->_tail = _z_transport_peer_entry_list_push(NULL, entry);
            parent->_tail->_val = entry;
        }
    }
    return root;
}
