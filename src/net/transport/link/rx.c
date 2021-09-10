/*
 * Copyright (c) 2017, 2020 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#include "zenoh-pico/net/private/internal.h"
#include "zenoh-pico/net/private/msg.h"
#include "zenoh-pico/net/private/msgcodec.h"
#include "zenoh-pico/net/private/system.h"
#include "zenoh-pico/net/rname.h"
#include "zenoh-pico/private/logging.h"
#include "zenoh-pico/private/system.h"
#include "zenoh-pico/utils.h"

/*------------------ Reception helper ------------------*/
void _zn_recv_t_msg_na(zn_session_t *zn, _zn_transport_message_p_result_t *r)
{
    _Z_DEBUG(">> recv session msg\n");
    r->tag = _z_res_t_OK;

    // Acquire the lock
    _z_mutex_lock(&zn->mutex_rx);

    // Prepare the buffer
    _z_zbuf_clear(&zn->zbuf);

#ifdef ZN_TRANSPORT_TCP_IP
    // NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
    //       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
    //       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
    //       the boundary of the serialized messages. The length is encoded as little-endian.
    //       In any case, the length of a message must not exceed 65_535 bytes.

    // Read the message length
    if (_zn_recv_bytes(zn->sock, zn->zbuf.ios.buf, _ZN_MSG_LEN_ENC_SIZE) < 0)
    {
        _zn_transport_message_p_result_free(r);
        r->tag = _z_res_t_ERR;
        r->value.error = _zn_err_t_IO_GENERIC;
        goto EXIT_SRCV_PROC;
    }
    _z_zbuf_set_wpos(&zn->zbuf, _ZN_MSG_LEN_ENC_SIZE);

    uint16_t len = _z_zbuf_read(&zn->zbuf) | (_z_zbuf_read(&zn->zbuf) << 8);
    _Z_DEBUG_VA(">> \t msg len = %hu\n", len);
    size_t writable = _z_zbuf_capacity(&zn->zbuf) - _z_zbuf_len(&zn->zbuf);
    if (writable < len)
    {
        _zn_transport_message_p_result_free(r);
        r->tag = _z_res_t_ERR;
        r->value.error = _zn_err_t_IOBUF_NO_SPACE;
        goto EXIT_SRCV_PROC;
    }

    // Read enough bytes to decode the message
    if (_zn_recv_bytes(zn->sock, zn->zbuf.ios.buf, len) < 0)
    {
        _zn_transport_message_p_result_free(r);
        r->tag = _z_res_t_ERR;
        r->value.error = _zn_err_t_IO_GENERIC;
        goto EXIT_SRCV_PROC;
    }

    _z_zbuf_set_rpos(&zn->zbuf, 0);
    _z_zbuf_set_wpos(&zn->zbuf, len);
#else
    if (_zn_recv_buf(sock, buf) < 0)
    {
        _zn_transport_message_p_result_free(r);
        r->tag = _z_res_t_ERR;
        r->value.error = _zn_err_t_IO_GENERIC;
        goto EXIT_SRCV_PROC;
    }
#endif /* ZN_TRANSPORT_TCP_IP */

    // Mark the session that we have received data
    zn->received = 1;

    _Z_DEBUG(">> \t transport_message_decode\n");
    _zn_transport_message_decode_na(&zn->zbuf, r);

EXIT_SRCV_PROC:
    // Release the lock
    _z_mutex_unlock(&zn->mutex_rx);
}

_zn_transport_message_p_result_t _zn_recv_t_msg(zn_session_t *zn)
{
    _zn_transport_message_p_result_t r;
    _zn_transport_message_p_result_init(&r);
    _zn_recv_t_msg_na(zn, &r);
    return r;
}
