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

#include "zenoh-pico/transport/link/tx.h"

#include "zenoh-pico/config.h"
#include "zenoh-pico/protocol/msgcodec.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/utils/logging.h"

#if Z_MULTICAST_TRANSPORT == 1

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - ztm->_mutex_inner
 */
_z_zint_t __unsafe_z_multicast_get_sn(_z_transport_multicast_t *ztm, z_reliability_t reliability) {
    _z_zint_t sn;
    if (reliability == Z_RELIABILITY_RELIABLE) {
        sn = ztm->_sn_tx_reliable;
        ztm->_sn_tx_reliable = _z_sn_increment(ztm->_sn_resolution, ztm->_sn_tx_reliable);
    } else {
        sn = ztm->_sn_tx_best_effort;
        ztm->_sn_tx_best_effort = _z_sn_increment(ztm->_sn_resolution, ztm->_sn_tx_best_effort);
    }
    return sn;
}

int8_t _z_multicast_send_t_msg(_z_transport_multicast_t *ztm, const _z_transport_message_t *t_msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG(">> send session message\n");

#if Z_MULTI_THREAD == 1
    // Acquire the lock
    _z_mutex_lock(&ztm->_mutex_tx);
#endif  // Z_MULTI_THREAD == 1

    // Prepare the buffer eventually reserving space for the message length
    __unsafe_z_prepare_wbuf(&ztm->_wbuf, _Z_LINK_IS_STREAMED(ztm->_link._capabilities));

    // Encode the session message
    ret = _z_transport_message_encode(&ztm->_wbuf, t_msg);
    if (ret == _Z_RES_OK) {
        // Write the message legnth in the reserved space if needed
        __unsafe_z_finalize_wbuf(&ztm->_wbuf, _Z_LINK_IS_STREAMED(ztm->_link._capabilities));
        // Send the wbuf on the socket
        ret = _z_link_send_wbuf(&ztm->_link, &ztm->_wbuf);
        if (ret == _Z_RES_OK) {
            ztm->_transmitted = true;  // Mark the session that we have transmitted data
        }
    }

#if Z_MULTI_THREAD == 1
    _z_mutex_unlock(&ztm->_mutex_tx);
#endif  // Z_MULTI_THREAD == 1

    return ret;
}

int8_t _z_multicast_send_z_msg(_z_session_t *zn, _z_zenoh_message_t *z_msg, z_reliability_t reliability,
                               z_congestion_control_t cong_ctrl) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG(">> send zenoh message\n");

    _z_transport_multicast_t *ztm = &zn->_tp._transport._multicast;

    // Acquire the lock and drop the message if needed
    _Bool drop = false;
    if (cong_ctrl == Z_CONGESTION_CONTROL_BLOCK) {
#if Z_MULTI_THREAD == 1
        _z_mutex_lock(&ztm->_mutex_tx);
#endif  // Z_MULTI_THREAD == 1
    } else {
#if Z_MULTI_THREAD == 1
        int8_t locked = _z_mutex_trylock(&ztm->_mutex_tx);
        if (locked != (int8_t)0) {
            _Z_INFO("Dropping zenoh message because of congestion control\n");
            // We failed to acquire the lock, drop the message
            drop = true;
        }
#endif  // Z_MULTI_THREAD == 1
    }

    if (drop == false) {
        // Prepare the buffer eventually reserving space for the message length
        __unsafe_z_prepare_wbuf(&ztm->_wbuf, _Z_LINK_IS_STREAMED(ztm->_link._capabilities));

        _z_zint_t sn = __unsafe_z_multicast_get_sn(ztm, reliability);  // Get the next sequence number

        _z_transport_message_t t_msg = _z_frame_header(reliability, 0, 0, sn);
        ret = _z_transport_message_encode(&ztm->_wbuf, &t_msg);  // Encode the frame header
        if (ret == _Z_RES_OK) {
            ret = _z_zenoh_message_encode(&ztm->_wbuf, z_msg);  // Encode the zenoh message
            if (ret == _Z_RES_OK) {
                // Write the message legnth in the reserved space if needed
                __unsafe_z_finalize_wbuf(&ztm->_wbuf, _Z_LINK_IS_STREAMED(ztm->_link._capabilities));

                ret = _z_link_send_wbuf(&ztm->_link, &ztm->_wbuf);  // Send the wbuf on the socket
                if (ret == _Z_RES_OK) {
                    ztm->_transmitted = true;  // Mark the session that we have transmitted data
                }
            } else {
                // The message does not fit in the current batch, let's fragment it
                // Create an expandable wbuf for fragmentation
                _z_wbuf_t fbf = _z_wbuf_make(Z_IOSLICE_SIZE, true);

                ret = _z_zenoh_message_encode(&fbf, z_msg);  // Encode the message on the expandable wbuf
                if (ret == _Z_RES_OK) {
                    _Bool is_first = true;  // Fragment and send the message
                    while (_z_wbuf_len(&fbf) > 0) {
                        if (is_first == false) {  // Get the fragment sequence number
                            sn = __unsafe_z_multicast_get_sn(ztm, reliability);
                        }
                        is_first = false;

                        // Clear the buffer for serialization
                        __unsafe_z_prepare_wbuf(&ztm->_wbuf, _Z_LINK_IS_STREAMED(ztm->_link._capabilities));

                        // Serialize one fragment
                        ret = __unsafe_z_serialize_zenoh_fragment(&ztm->_wbuf, &fbf, reliability, sn);
                        if (ret == _Z_RES_OK) {
                            // Write the message length in the reserved space if needed
                            __unsafe_z_finalize_wbuf(&ztm->_wbuf, _Z_LINK_IS_STREAMED(ztm->_link._capabilities));

                            ret = _z_link_send_wbuf(&ztm->_link, &ztm->_wbuf);  // Send the wbuf on the socket
                            if (ret == _Z_RES_OK) {
                                ztm->_transmitted = true;  // Mark the session that we have transmitted data
                            }
                        }
                    }
                }

                _z_wbuf_clear(&fbf);  // Free the fragmentation buffer memory
            }
        }

#if Z_MULTI_THREAD == 1
        _z_mutex_unlock(&ztm->_mutex_tx);
#endif  // Z_MULTI_THREAD == 1
    }

    return ret;
}

#endif  // Z_MULTICAST_TRANSPORT == 1
