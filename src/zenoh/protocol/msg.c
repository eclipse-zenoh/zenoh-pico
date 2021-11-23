/*
 * Copyright (c) 2017, 2021 ADLINK Technology Inc.
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

#include "zenoh-pico/config.h"
#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/msg.h"

/*=============================*/
/*     Transport Messages      */
/*=============================*/
_zn_transport_message_t _zn_t_msg_make_scout(z_zint_t what, int request_pid)
{
    _zn_transport_message_t msg;

    msg.attachment = NULL;

    msg.header = _ZN_MID_SCOUT;
    if (request_pid)
        msg.header |= _ZN_FLAG_T_I;
    if (what != ZN_ROUTER)
        msg.header |= _ZN_FLAG_T_W;

    msg.body.scout.what = what;

    return msg;
}

_zn_transport_message_t _zn_t_msg_make_hello(z_zint_t whatami, z_bytes_t pid, _zn_locator_array_t locators)
{
    _zn_transport_message_t msg;

    msg.attachment = NULL;

    msg.header = _ZN_MID_HELLO;
    if (!_z_bytes_is_empty(&pid))
        msg.header |= _ZN_FLAG_T_I;
    if (whatami != ZN_ROUTER)
        msg.header |= _ZN_FLAG_T_W;
    if (_zn_locator_array_is_empty(&locators))
        msg.header |= _ZN_FLAG_T_L;

    msg.body.hello.whatami = whatami;
    _z_bytes_move(&msg.body.hello.pid, &pid);
    _zn_locator_array_move(&msg.body.hello.locators, &locators);

    return msg;
}

_zn_transport_message_t _zn_t_msg_make_join(uint8_t version, z_zint_t whatami, z_zint_t lease, z_zint_t sn_resolution, z_bytes_t pid, _zn_sns_t next_sns)
{
    _zn_transport_message_t msg;

    msg.attachment = NULL;

    msg.header = _ZN_MID_JOIN;
    if (lease % 1000 == 0)
        msg.header |= _ZN_FLAG_T_T1;
    if (sn_resolution != ZN_SN_RESOLUTION_DEFAULT)
        msg.header |= _ZN_FLAG_T_S;

    msg.body.join.options = 0; // No QoS support in zenoh-pico
    msg.body.join.version = version;
    msg.body.join.whatami = whatami;
    msg.body.join.lease = lease;
    msg.body.join.sn_resolution = sn_resolution;
    msg.body.join.next_sns = next_sns;
    _z_bytes_move(&msg.body.join.pid, &pid);

    return msg;
}

_zn_transport_message_t _zn_t_msg_make_init_syn(uint8_t version, z_zint_t whatami, z_zint_t sn_resolution, z_bytes_t pid)
{
    _zn_transport_message_t msg;

    msg.attachment = NULL;

    msg.header = _ZN_MID_INIT;
    if (sn_resolution != ZN_SN_RESOLUTION_DEFAULT)
        msg.header |= _ZN_FLAG_T_S;

    msg.body.init.options = 0; // No QoS support in zenoh-pico
    msg.body.init.version = version;
    msg.body.init.whatami = whatami;
    msg.body.init.sn_resolution = sn_resolution;
    _z_bytes_move(&msg.body.init.pid, &pid);
    _z_bytes_reset(&msg.body.init.cookie);

    return msg;
}

_zn_transport_message_t _zn_t_msg_make_init_ack(uint8_t version, z_zint_t whatami, z_zint_t sn_resolution, z_bytes_t pid, z_bytes_t cookie)
{
    _zn_transport_message_t msg;

    msg.attachment = NULL;

    msg.header = _ZN_MID_INIT | _ZN_FLAG_T_A;
    if (sn_resolution != ZN_SN_RESOLUTION_DEFAULT)
        msg.header |= _ZN_FLAG_T_S;

    msg.body.init.options = 0; // No QoS support in zenoh-pico
    msg.body.init.version = version;
    msg.body.init.whatami = whatami;
    msg.body.init.sn_resolution = sn_resolution;
    _z_bytes_move(&msg.body.init.pid, &pid);
    _z_bytes_move(&msg.body.init.cookie, &cookie);

    return msg;
}

_zn_transport_message_t _zn_t_msg_make_open_syn(z_zint_t lease, z_zint_t initial_sn, z_bytes_t cookie)
{
    _zn_transport_message_t msg;

    msg.attachment = NULL;

    msg.header = _ZN_MID_OPEN;
    if (lease % 1000 == 0)
        msg.header |= _ZN_FLAG_T_T2;

    msg.body.open.lease = lease;
    msg.body.open.initial_sn = initial_sn;
    _z_bytes_move(&msg.body.open.cookie, &cookie);

    return msg;
}

_zn_transport_message_t _zn_t_msg_make_open_ack(z_zint_t lease, z_zint_t initial_sn)
{
    _zn_transport_message_t msg;

    msg.attachment = NULL;

    msg.header = _ZN_MID_OPEN | _ZN_FLAG_T_A;
    if (lease % 1000 == 0)
        msg.header |= _ZN_FLAG_T_T2;

    msg.body.open.lease = lease;
    msg.body.open.initial_sn = initial_sn;
    _z_bytes_reset(&msg.body.open.cookie);

    return msg;
}

_zn_transport_message_t _zn_t_msg_make_close(z_bytes_t pid, uint8_t reason)
{
    _zn_transport_message_t msg;

    msg.attachment = NULL;

    msg.header = _ZN_MID_CLOSE;
    if (!_z_bytes_is_empty(&pid))
        msg.header |= _ZN_FLAG_T_I;

    msg.body.close.reason = reason;
    _z_bytes_move(&msg.body.close.pid, &pid);

    return msg;
}

_zn_transport_message_t _zn_t_msg_make_sync(z_zint_t sn, int is_reliable, z_zint_t count)
{
    _zn_transport_message_t msg;

    msg.attachment = NULL;

    msg.header = _ZN_MID_SYNC;
    if (is_reliable)
    {
        msg.header |= _ZN_FLAG_T_R;
        if (count != 0)
            msg.header |= _ZN_FLAG_T_C;
    }

    msg.body.sync.sn = sn;
    msg.body.sync.count = count;

    return msg;
}

_zn_transport_message_t _zn_t_msg_make_ack_nack(z_zint_t sn, z_zint_t mask)
{
    _zn_transport_message_t msg;

    msg.attachment = NULL;

    msg.header = _ZN_MID_ACK_NACK;
    if (mask != 0)
        msg.header |= _ZN_FLAG_T_M;

    msg.body.ack_nack.sn = sn;
    msg.body.ack_nack.mask = mask;

    return msg;
}

_zn_transport_message_t _zn_t_msg_make_keep_alive(z_bytes_t pid)
{
    _zn_transport_message_t msg;

    msg.attachment = NULL;

    msg.header = _ZN_MID_KEEP_ALIVE;
    if (!_z_bytes_is_empty(&pid))
        msg.header |= _ZN_FLAG_T_I;

    _z_bytes_move(&msg.body.keep_alive.pid, &pid);

    return msg;
}

_zn_transport_message_t _zn_t_msg_make_ping(z_zint_t hash)
{
    _zn_transport_message_t msg;

    msg.attachment = NULL;

    msg.header = _ZN_MID_PING_PONG;

    msg.body.ping_pong.hash = hash;

    return msg;
}

_zn_transport_message_t _zn_t_msg_make_pong(z_zint_t hash)
{
    _zn_transport_message_t msg;

    msg.attachment = NULL;

    msg.header = _ZN_MID_PING_PONG | _ZN_FLAG_T_P;

    msg.body.ping_pong.hash = hash;

    return msg;
}

_zn_transport_message_t _zn_t_msg_make_frame(z_zint_t sn, _zn_frame_payload_t payload, int is_reliable, int is_fragment, int is_final)
{
    _zn_transport_message_t msg;

    msg.attachment = NULL;

    msg.header = _ZN_MID_FRAME;
    if (is_reliable)
        msg.header |= _ZN_FLAG_T_R;
    if (is_fragment)
    {
        msg.header |= _ZN_FLAG_T_F;
        if (is_final)
            msg.header |= _ZN_FLAG_T_E;
    }

    msg.body.frame.sn = sn;
    msg.body.frame.payload = payload;

    return msg;
}
