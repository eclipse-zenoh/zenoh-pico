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

#include "zenoh-pico/transport/unicast/read.h"

#include <stddef.h>

#include "zenoh-pico/api/types.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/protocol/codec/transport.h"
#include "zenoh-pico/transport/common/rx.h"
#include "zenoh-pico/transport/transport.h"
#include "zenoh-pico/transport/unicast/rx.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_UNICAST_TRANSPORT == 1

z_result_t _zp_unicast_read(_z_transport_unicast_t *ztu) {
    z_result_t ret = _Z_RES_OK;

    _z_transport_message_t t_msg;
    _z_transport_unicast_peer_t *peer = _z_transport_unicast_peer_list_head(ztu->_peers);
    assert(peer != NULL);
    ret = _z_unicast_recv_t_msg(ztu, &t_msg);
    if (ret == _Z_RES_OK) {
        ret = _z_unicast_handle_transport_message(ztu, &t_msg, peer);
        _z_t_msg_clear(&t_msg);
    }
    ret = _z_unicast_update_rx_buffer(ztu);
    if (ret != _Z_RES_OK) {
        _Z_ERROR("Failed to allocate rx buffer");
    }
    return ret;
}
#else

z_result_t _zp_unicast_read(_z_transport_unicast_t *ztu) {
    _ZP_UNUSED(ztu);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}
#endif  // Z_FEATURE_UNICAST_TRANSPORT == 1

#if Z_FEATURE_MULTI_THREAD == 1 && Z_FEATURE_UNICAST_TRANSPORT == 1

static z_result_t _z_unicast_process_messages(_z_transport_unicast_t *ztu, _z_transport_unicast_peer_t *peer,
                                              size_t to_read) {
    // Wrap the main buffer for to_read bytes
    _z_zbuf_t zbuf = _z_zbuf_view(&ztu->_common._zbuf, to_read);

    peer->_received = true;
    while (_z_zbuf_len(&zbuf) > 0) {
        // Decode one session message
        _z_transport_message_t t_msg;
        z_result_t ret = _z_transport_message_decode(&t_msg, &zbuf, &ztu->_common._arc_pool, &ztu->_common._msg_pool);

        if (ret != _Z_RES_OK) {
            _Z_ERROR("Connection closed due to malformed message: %d", ret);
            return ret;
        }
        ret = _z_unicast_handle_transport_message(ztu, &t_msg, peer);
        if (ret != _Z_RES_OK) {
            if (ret != _Z_ERR_CONNECTION_CLOSED) {
                _Z_ERROR("Connection closed due to message processing error: %d", ret);
            }
            return ret;
        }
    }
    // Move the read position of the read buffer
    _z_zbuf_set_rpos(&ztu->_common._zbuf, _z_zbuf_get_rpos(&ztu->_common._zbuf) + to_read);

    if (_z_unicast_update_rx_buffer(ztu) != _Z_RES_OK) {
        _Z_ERROR("Connection closed due to lack of memory to allocate rx buffer");
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    return _Z_RES_OK;
}

static bool _z_unicast_client_read(_z_transport_unicast_t *ztu, _z_transport_unicast_peer_t *peer, size_t *to_read) {
    switch (ztu->_common._link._cap._flow) {
        case Z_LINK_CAP_FLOW_STREAM:
            if (_z_zbuf_len(&ztu->_common._zbuf) < _Z_MSG_LEN_ENC_SIZE) {
                _z_link_socket_recv_zbuf(&ztu->_common._link, &ztu->_common._zbuf, peer->_socket);
                if (_z_zbuf_len(&ztu->_common._zbuf) < _Z_MSG_LEN_ENC_SIZE) {
                    _z_zbuf_compact(&ztu->_common._zbuf);
                    return false;
                }
            }
            // Get stream size
            *to_read = _z_read_stream_size(&ztu->_common._zbuf);
            // Read data
            if (_z_zbuf_len(&ztu->_common._zbuf) < *to_read) {
                _z_link_socket_recv_zbuf(&ztu->_common._link, &ztu->_common._zbuf, peer->_socket);
                if (_z_zbuf_len(&ztu->_common._zbuf) < *to_read) {
                    _z_zbuf_set_rpos(&ztu->_common._zbuf, _z_zbuf_get_rpos(&ztu->_common._zbuf) - _Z_MSG_LEN_ENC_SIZE);
                    _z_zbuf_compact(&ztu->_common._zbuf);
                    return false;
                }
            }
            break;
        case Z_LINK_CAP_FLOW_DATAGRAM:
            _z_zbuf_compact(&ztu->_common._zbuf);
            *to_read = _z_link_socket_recv_zbuf(&ztu->_common._link, &ztu->_common._zbuf, peer->_socket);
            if (*to_read == SIZE_MAX) {
                return false;
            }
            break;
        default:
            break;
    }
    return true;
}

static int _z_unicast_peer_read(_z_transport_unicast_t *ztu, _z_transport_unicast_peer_t *peer, size_t *to_read) {
    // Warning: if we receive fragmented data we have to discard it because we only have one buffer for all sockets
    size_t read_size = 0;
    switch (ztu->_common._link._cap._flow) {
        case Z_LINK_CAP_FLOW_STREAM:
            read_size = _z_link_socket_recv_zbuf(&ztu->_common._link, &ztu->_common._zbuf, peer->_socket);
            if (read_size == 0) {
                _Z_DEBUG("Socket closed");
                return -2;
            } else if (read_size == SIZE_MAX) {
                return -1;
            }
            if (_z_zbuf_len(&ztu->_common._zbuf) < _Z_MSG_LEN_ENC_SIZE) {
                _Z_DEBUG("Not enough bytes to read stream size");
                return -1;
            }
            // Get stream size
            *to_read = _z_read_stream_size(&ztu->_common._zbuf);
            // Read data if needed
            if (_z_zbuf_len(&ztu->_common._zbuf) < *to_read) {
                _Z_DEBUG("Not enough bytes to read packet");
                return -1;
            }
            break;
        case Z_LINK_CAP_FLOW_DATAGRAM:
            *to_read = _z_link_socket_recv_zbuf(&ztu->_common._link, &ztu->_common._zbuf, peer->_socket);
            if (*to_read == SIZE_MAX) {
                return -1;
            }
            break;
        default:
            break;
    }
    return 0;
}

void *_zp_unicast_read_task(void *ztu_arg) {
    _z_transport_unicast_t *ztu = (_z_transport_unicast_t *)ztu_arg;

    // Acquire and keep the lock
    _z_mutex_lock(&ztu->_common._mutex_rx);

    // Prepare the buffer
    _z_zbuf_reset(&ztu->_common._zbuf);
    z_whatami_t mode = _Z_RC_IN_VAL(ztu->_common._session)->_mode;
    _z_transport_unicast_peer_t *curr_peer = NULL;
    if (mode == Z_WHATAMI_CLIENT) {
        curr_peer = _z_transport_unicast_peer_list_head(ztu->_peers);
        assert(curr_peer != NULL);
    }
    while (ztu->_common._read_task_running) {
        // Read bytes from socket to the main buffer
        size_t to_read = 0;

        if (mode == Z_WHATAMI_PEER) {
            // Wait for at least one peer
            _z_transport_peer_mutex_lock(&ztu->_common);
            size_t peer_len = _z_transport_unicast_peer_list_len(ztu->_peers);
            _z_transport_peer_mutex_unlock(&ztu->_common);
            if (peer_len == 0) {
                z_sleep_s(1);
                continue;
            }
            // Wait for events on sockets (need mutex)
            if (_z_socket_wait_event(ztu->_peers) != _Z_RES_OK) {
                continue;  // Might need to process errors other than timeout
            }
            // Process events
            _z_transport_peer_mutex_lock(&ztu->_common);
            _z_transport_unicast_peer_list_t *curr_list = ztu->_peers;
            _z_transport_unicast_peer_list_t *to_drop = NULL;
            _z_transport_unicast_peer_list_t *prev = NULL;
            while (curr_list != NULL) {
                bool drop_peer = false;
                curr_peer = _z_transport_unicast_peer_list_head(curr_list);
                if (curr_peer->_pending) {
                    curr_peer->_pending = false;
                    int res = _z_unicast_peer_read(ztu, curr_peer, &to_read);
                    if (res == 0) {
                        if (_z_unicast_process_messages(ztu, curr_peer, to_read) != _Z_RES_OK) {
                            drop_peer = true;
                            to_drop = curr_list;
                        }
                    } else if (res == -2) {
                        drop_peer = true;
                        to_drop = curr_list;
                    }
                }
                if (drop_peer) {
                    _Z_DEBUG("Dropping peer");
                    ztu->_peers = _z_transport_unicast_peer_list_drop_element(ztu->_peers, to_drop, prev);
                }
                prev = curr_list;
                curr_list = _z_transport_unicast_peer_list_tail(curr_list);
                _z_zbuf_reset(&ztu->_common._zbuf);
            }
            _z_transport_peer_mutex_unlock(&ztu->_common);
        } else {
            // Retrieve data
            if (!_z_unicast_client_read(ztu, curr_peer, &to_read)) {
                continue;
            }
            // Process data
            if (_z_unicast_process_messages(ztu, curr_peer, to_read) != _Z_RES_OK) {
                ztu->_common._read_task_running = false;
                continue;
            }
        }
    }
    _z_mutex_unlock(&ztu->_common._mutex_rx);
    return NULL;
}

z_result_t _zp_unicast_start_read_task(_z_transport_t *zt, z_task_attr_t *attr, _z_task_t *task) {
    // Init memory
    (void)memset(task, 0, sizeof(_z_task_t));
    zt->_transport._unicast._common._read_task_running = true;  // Init before z_task_init for concurrency issue
    // Init task
    if (_z_task_init(task, attr, _zp_unicast_read_task, &zt->_transport._unicast) != _Z_RES_OK) {
        zt->_transport._unicast._common._read_task_running = false;
        return _Z_ERR_SYSTEM_TASK_FAILED;
    }
    // Attach task
    zt->_transport._unicast._common._read_task = task;
    return _Z_RES_OK;
}

z_result_t _zp_unicast_stop_read_task(_z_transport_t *zt) {
    zt->_transport._unicast._common._read_task_running = false;
    return _Z_RES_OK;
}

#else

void *_zp_unicast_read_task(void *ztu_arg) {
    _ZP_UNUSED(ztu_arg);
    return NULL;
}

z_result_t _zp_unicast_start_read_task(_z_transport_t *zt, void *attr, void *task) {
    _ZP_UNUSED(zt);
    _ZP_UNUSED(attr);
    _ZP_UNUSED(task);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

z_result_t _zp_unicast_stop_read_task(_z_transport_t *zt) {
    _ZP_UNUSED(zt);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}
#endif
