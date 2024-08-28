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

#include "zenoh-pico/transport/common/tx.h"

#include <assert.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/protocol/codec/network.h"
#include "zenoh-pico/protocol/codec/transport.h"
#include "zenoh-pico/protocol/iobuf.h"
#include "zenoh-pico/transport/unicast/tx.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_UNICAST_TRANSPORT == 1

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - ztu->_mutex_inner
 */
_z_zint_t __unsafe_z_unicast_get_sn(_z_transport_unicast_t *ztu, z_reliability_t reliability) {
    _z_zint_t sn;
    if (reliability == Z_RELIABILITY_RELIABLE) {
        sn = ztu->_sn_tx_reliable;
        ztu->_sn_tx_reliable = _z_sn_increment(ztu->_sn_res, ztu->_sn_tx_reliable);
    } else {
        sn = ztu->_sn_tx_best_effort;
        ztu->_sn_tx_best_effort = _z_sn_increment(ztu->_sn_res, ztu->_sn_tx_best_effort);
    }
    return sn;
}

int8_t _z_unicast_send_t_msg(_z_transport_unicast_t *ztu, const _z_transport_message_t *t_msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG(">> send session message");

#if Z_FEATURE_MULTI_THREAD == 1
    // Acquire the lock
    _z_mutex_lock(&ztu->_mutex_tx);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    // Prepare the buffer eventually reserving space for the message length
    __unsafe_z_prepare_wbuf(&ztu->_wbuf, ztu->_link._cap._flow);

    // Encode the session message
    ret = _z_transport_message_encode(&ztu->_wbuf, t_msg);
    if (ret == _Z_RES_OK) {
        // Write the message length in the reserved space if needed
        __unsafe_z_finalize_wbuf(&ztu->_wbuf, ztu->_link._cap._flow);
        // Send the wbuf on the socket
        ret = _z_link_send_wbuf(&ztu->_link, &ztu->_wbuf);
        if (ret == _Z_RES_OK) {
            ztu->_transmitted = true;  // Mark the session that we have transmitted data
        }
    }

#if Z_FEATURE_MULTI_THREAD == 1
    _z_mutex_unlock(&ztu->_mutex_tx);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    return ret;
}

int8_t _z_unicast_send_n_msg(_z_session_t *zn, const _z_network_message_t *n_msg, z_reliability_t reliability,
                             z_congestion_control_t cong_ctrl) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG(">> send network message");

    _z_transport_unicast_t *ztu = &zn->_tp._transport._unicast;

    // Acquire the lock and drop the message if needed
    _Bool drop = false;
    if (cong_ctrl == Z_CONGESTION_CONTROL_BLOCK) {
#if Z_FEATURE_MULTI_THREAD == 1
        _z_mutex_lock(&ztu->_mutex_tx);
#endif  // Z_FEATURE_MULTI_THREAD == 1
    } else {
#if Z_FEATURE_MULTI_THREAD == 1
        int8_t locked = _z_mutex_try_lock(&ztu->_mutex_tx);
        if (locked != (int8_t)0) {
            _Z_INFO("Dropping zenoh message because of congestion control");
            // We failed to acquire the lock, drop the message
            drop = true;
        }
#endif  // Z_FEATURE_MULTI_THREAD == 1
    }

    if (drop == false) {
        // Prepare the buffer eventually reserving space for the message length
        __unsafe_z_prepare_wbuf(&ztu->_wbuf, ztu->_link._cap._flow);

        _z_zint_t sn = __unsafe_z_unicast_get_sn(ztu, reliability);  // Get the next sequence number

        _z_transport_message_t t_msg = _z_t_msg_make_frame_header(sn, reliability);
        ret = _z_transport_message_encode(&ztu->_wbuf, &t_msg);  // Encode the frame header
        if (ret == _Z_RES_OK) {
            ret = _z_network_message_encode(&ztu->_wbuf, n_msg);  // Encode the network message
            if (ret == _Z_RES_OK) {
                // Write the message length in the reserved space if needed
                __unsafe_z_finalize_wbuf(&ztu->_wbuf, ztu->_link._cap._flow);

                if (ztu->_wbuf._ioss._len == 1) {
                    ret = _z_link_send_wbuf(&ztu->_link, &ztu->_wbuf);  // Send the wbuf on the socket
                } else {
                    // Change the MID

                    // for ()
                }
                if (ret == _Z_RES_OK) {
                    ztu->_transmitted = true;  // Mark the session that we have transmitted data
                }
            } else {
#if Z_FEATURE_FRAGMENTATION == 1
                // The message does not fit in the current batch, let's fragment it
                // Create an expandable wbuf for fragmentation
                _z_wbuf_t fbf = _z_wbuf_make(_Z_FRAG_BUFF_BASE_SIZE, true);

                ret = _z_network_message_encode(&fbf, n_msg);  // Encode the message on the expandable wbuf
                if (ret == _Z_RES_OK) {
                    _Bool is_first = true;  // Fragment and send the message
                    while (_z_wbuf_len(&fbf) > 0) {
                        if (is_first == false) {  // Get the fragment sequence number
                            sn = __unsafe_z_unicast_get_sn(ztu, reliability);
                        }
                        is_first = false;

                        // Clear the buffer for serialization
                        __unsafe_z_prepare_wbuf(&ztu->_wbuf, ztu->_link._cap._flow);

                        // Serialize one fragment
                        ret = __unsafe_z_serialize_zenoh_fragment(&ztu->_wbuf, &fbf, reliability, sn);
                        if (ret == _Z_RES_OK) {
                            // Write the message length in the reserved space if needed
                            __unsafe_z_finalize_wbuf(&ztu->_wbuf, ztu->_link._cap._flow);

                            ret = _z_link_send_wbuf(&ztu->_link, &ztu->_wbuf);  // Send the wbuf on the socket
                            if (ret == _Z_RES_OK) {
                                ztu->_transmitted = true;  // Mark the session that we have transmitted data
                            }
                        }
                    }
                }

                // Clear the buffer as it's no longer required
                _z_wbuf_clear(&fbf);
#else
                _Z_INFO("Sending the message required fragmentation feature that is deactivated.");
#endif
            }
        }

#if Z_FEATURE_MULTI_THREAD == 1
        _z_mutex_unlock(&ztu->_mutex_tx);
#endif  // Z_FEATURE_MULTI_THREAD == 1
    }

    return ret;
}
#else
int8_t _z_unicast_send_t_msg(_z_transport_unicast_t *ztu, const _z_transport_message_t *t_msg) {
    _ZP_UNUSED(ztu);
    _ZP_UNUSED(t_msg);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

int8_t _z_unicast_send_n_msg(_z_session_t *zn, const _z_network_message_t *n_msg, z_reliability_t reliability,
                             z_congestion_control_t cong_ctrl) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(n_msg);
    _ZP_UNUSED(reliability);
    _ZP_UNUSED(cong_ctrl);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}
#endif  // Z_FEATURE_UNICAST_TRANSPORT == 1
