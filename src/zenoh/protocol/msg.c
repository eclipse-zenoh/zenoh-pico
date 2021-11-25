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
/*       Zenoh Messages        */
/*=============================*/
_zn_zenoh_message_t _zn_z_msg_make_declare(_zn_declaration_array_t declarations)
{
    _zn_zenoh_message_t msg;

    msg.body.declare.declarations = declarations;

    msg.header = _ZN_MID_DECLARE;

    msg.attachment = NULL;
    msg.reply_context = NULL;

    return msg;
}
_zn_zenoh_message_t _zn_z_msg_make_data(zn_reskey_t key, _zn_data_info_t info, _zn_payload_t payload, int can_be_dropped)
{
    _zn_zenoh_message_t msg;

    msg.body.data.key = key;
    msg.body.data.info = info;
    msg.body.data.payload = payload;

    msg.header = _ZN_MID_DATA;
    if (msg.body.data.info.flags != 0)
        msg.header |= _ZN_FLAG_Z_I;
    if (msg.body.data.key.rname != NULL)
        msg.header |= _ZN_FLAG_Z_K;
    if (can_be_dropped)
        msg.header |= _ZN_FLAG_Z_D;

    msg.attachment = NULL;
    msg.reply_context = NULL;

    return msg;
}
_zn_zenoh_message_t _zn_z_msg_make_unit(int can_be_dropped)
{
    _zn_zenoh_message_t msg;

    msg.header = _ZN_MID_UNIT;
    if (can_be_dropped)
        msg.header |= _ZN_FLAG_Z_D;

    msg.attachment = NULL;
    msg.reply_context = NULL;

    return msg;
}

_zn_zenoh_message_t _zn_z_msg_make_pull(zn_reskey_t key, z_zint_t pull_id, z_zint_t max_samples, int is_final)
{
    _zn_zenoh_message_t msg;

    msg.body.pull.key = key;
    msg.body.pull.pull_id = pull_id;
    msg.body.pull.max_samples = max_samples;

    msg.header = _ZN_MID_PULL;
    if (is_final)
        msg.header |= _ZN_FLAG_Z_F;
    if (max_samples != 0)
        msg.header |= _ZN_FLAG_Z_N;
    if (msg.body.pull.key.rname != NULL)
        msg.header |= _ZN_FLAG_Z_K;

    msg.attachment = NULL;
    msg.reply_context = NULL;

    return msg;
}

_zn_zenoh_message_t _zn_z_msg_make_query(zn_reskey_t key, z_str_t predicate, z_zint_t qid, zn_query_target_t target, zn_query_consolidation_t consolidation)
{
    _zn_zenoh_message_t msg;

    msg.body.query.key = key;
    msg.body.query.predicate = predicate;
    msg.body.query.qid = qid;
    msg.body.query.target = target;
    msg.body.query.consolidation = consolidation;

    msg.header = _ZN_MID_QUERY;
    if (msg.body.query.target.kind != ZN_QUERYABLE_ALL_KINDS)
        msg.header |= _ZN_FLAG_Z_T;
    if (msg.body.query.key.rname != NULL)
        msg.header |= _ZN_FLAG_Z_K;

    msg.attachment = NULL;
    msg.reply_context = NULL;

    return msg;
}

/*=============================*/
/*     Transport Messages      */
/*=============================*/
_zn_transport_message_t _zn_t_msg_make_scout(z_zint_t what, int request_pid)
{
    _zn_transport_message_t msg;

    msg.body.scout.what = what;

    msg.header = _ZN_MID_SCOUT;
    if (request_pid)
        msg.header |= _ZN_FLAG_T_I;
    if (what != ZN_ROUTER)
        msg.header |= _ZN_FLAG_T_W;

    msg.attachment = NULL;

    return msg;
}

_zn_transport_message_t _zn_t_msg_make_hello(z_zint_t whatami, z_bytes_t pid, _zn_locator_array_t locators)
{
    _zn_transport_message_t msg;

    msg.body.hello.whatami = whatami;
    _z_bytes_move(&msg.body.hello.pid, &pid);
    _zn_locator_array_move(&msg.body.hello.locators, &locators);

    msg.header = _ZN_MID_HELLO;
    if (whatami != ZN_ROUTER)
        msg.header |= _ZN_FLAG_T_W;
    if (!_z_bytes_is_empty(&pid))
        msg.header |= _ZN_FLAG_T_I;
    if (!_zn_locator_array_is_empty(&locators))
        msg.header |= _ZN_FLAG_T_L;

    msg.attachment = NULL;

    return msg;
}

_zn_transport_message_t _zn_t_msg_make_join(uint8_t version, z_zint_t whatami, z_zint_t lease, z_zint_t sn_resolution, z_bytes_t pid, _zn_conduit_sn_list_t next_sns)
{
    _zn_transport_message_t msg;

    msg.body.join.options = 0;
    if (next_sns.is_qos)
        msg.body.join.options |= _ZN_OPT_JOIN_QOS;
    msg.body.join.version = version;
    msg.body.join.whatami = whatami;
    msg.body.join.lease = lease;
    msg.body.join.sn_resolution = sn_resolution;
    msg.body.join.next_sns = next_sns;
    _z_bytes_move(&msg.body.join.pid, &pid);

    msg.header = _ZN_MID_JOIN;
    if (lease % 1000 == 0)
        msg.header |= _ZN_FLAG_T_T1;
    if (sn_resolution != ZN_SN_RESOLUTION_DEFAULT)
        msg.header |= _ZN_FLAG_T_S;
    if (msg.body.join.options != 0)
        msg.header |= _ZN_FLAG_T_O;

    msg.attachment = NULL;

    return msg;
}

_zn_transport_message_t _zn_t_msg_make_init_syn(uint8_t version, z_zint_t whatami, z_zint_t sn_resolution, z_bytes_t pid, int is_qos)
{
    _zn_transport_message_t msg;

    msg.body.init.options = 0;
    if (is_qos)
        msg.body.init.options |= _ZN_OPT_INIT_QOS;
    msg.body.init.version = version;
    msg.body.init.whatami = whatami;
    msg.body.init.sn_resolution = sn_resolution;
    _z_bytes_move(&msg.body.init.pid, &pid);
    _z_bytes_reset(&msg.body.init.cookie);

    msg.header = _ZN_MID_INIT;
    if (sn_resolution != ZN_SN_RESOLUTION_DEFAULT)
        msg.header |= _ZN_FLAG_T_S;
    if (msg.body.init.options != 0)
        msg.header |= _ZN_FLAG_T_O;

    msg.attachment = NULL;

    return msg;
}

_zn_transport_message_t _zn_t_msg_make_init_ack(uint8_t version, z_zint_t whatami, z_zint_t sn_resolution, z_bytes_t pid, z_bytes_t cookie, int is_qos)
{
    _zn_transport_message_t msg;

    msg.body.init.options = 0;
    if (is_qos)
        msg.body.init.options |= _ZN_OPT_INIT_QOS;
    msg.body.init.version = version;
    msg.body.init.whatami = whatami;
    msg.body.init.sn_resolution = sn_resolution;
    _z_bytes_move(&msg.body.init.pid, &pid);
    _z_bytes_move(&msg.body.init.cookie, &cookie);

    msg.header = _ZN_MID_INIT | _ZN_FLAG_T_A;
    if (sn_resolution != ZN_SN_RESOLUTION_DEFAULT)
        msg.header |= _ZN_FLAG_T_S;
    if (msg.body.init.options != 0)
        msg.header |= _ZN_FLAG_T_O;

    msg.attachment = NULL;

    return msg;
}

_zn_transport_message_t _zn_t_msg_make_open_syn(z_zint_t lease, z_zint_t initial_sn, z_bytes_t cookie)
{
    _zn_transport_message_t msg;

    msg.body.open.lease = lease;
    msg.body.open.initial_sn = initial_sn;
    _z_bytes_move(&msg.body.open.cookie, &cookie);

    msg.header = _ZN_MID_OPEN;
    if (lease % 1000 == 0)
        msg.header |= _ZN_FLAG_T_T2;

    msg.attachment = NULL;

    return msg;
}

_zn_transport_message_t _zn_t_msg_make_open_ack(z_zint_t lease, z_zint_t initial_sn)
{
    _zn_transport_message_t msg;

    msg.body.open.lease = lease;
    msg.body.open.initial_sn = initial_sn;
    _z_bytes_reset(&msg.body.open.cookie);

    msg.header = _ZN_MID_OPEN | _ZN_FLAG_T_A;
    if (lease % 1000 == 0)
        msg.header |= _ZN_FLAG_T_T2;

    msg.attachment = NULL;

    return msg;
}

_zn_transport_message_t _zn_t_msg_make_close(uint8_t reason, z_bytes_t pid, int link_only)
{
    _zn_transport_message_t msg;

    msg.body.close.reason = reason;
    _z_bytes_move(&msg.body.close.pid, &pid);

    msg.header = _ZN_MID_CLOSE;
    if (!_z_bytes_is_empty(&pid))
        msg.header |= _ZN_FLAG_T_I;
    if (link_only)
        msg.header |= _ZN_FLAG_T_K;

    msg.attachment = NULL;

    return msg;
}

_zn_transport_message_t _zn_t_msg_make_sync(z_zint_t sn, int is_reliable, z_zint_t count)
{
    _zn_transport_message_t msg;

    msg.body.sync.sn = sn;
    msg.body.sync.count = count;

    msg.header = _ZN_MID_SYNC;
    if (is_reliable)
    {
        msg.header |= _ZN_FLAG_T_R;
        if (count != 0)
            msg.header |= _ZN_FLAG_T_C;
    }

    msg.attachment = NULL;

    return msg;
}

_zn_transport_message_t _zn_t_msg_make_ack_nack(z_zint_t sn, z_zint_t mask)
{
    _zn_transport_message_t msg;

    msg.body.ack_nack.sn = sn;
    msg.body.ack_nack.mask = mask;

    msg.header = _ZN_MID_ACK_NACK;
    if (mask != 0)
        msg.header |= _ZN_FLAG_T_M;

    msg.attachment = NULL;

    return msg;
}

_zn_transport_message_t _zn_t_msg_make_keep_alive(z_bytes_t pid)
{
    _zn_transport_message_t msg;

    _z_bytes_move(&msg.body.keep_alive.pid, &pid);

    msg.header = _ZN_MID_KEEP_ALIVE;
    if (!_z_bytes_is_empty(&pid))
        msg.header |= _ZN_FLAG_T_I;

    msg.attachment = NULL;

    return msg;
}

_zn_transport_message_t _zn_t_msg_make_ping(z_zint_t hash)
{
    _zn_transport_message_t msg;

    msg.body.ping_pong.hash = hash;

    msg.header = _ZN_MID_PING_PONG;

    msg.attachment = NULL;

    return msg;
}

_zn_transport_message_t _zn_t_msg_make_pong(z_zint_t hash)
{
    _zn_transport_message_t msg;

    msg.body.ping_pong.hash = hash;

    msg.header = _ZN_MID_PING_PONG | _ZN_FLAG_T_P;

    msg.attachment = NULL;

    return msg;
}

_zn_transport_message_t _zn_t_msg_make_frame(z_zint_t sn, _zn_frame_payload_t payload, int is_reliable, int is_fragment, int is_final)
{
    _zn_transport_message_t msg;

    msg.body.frame.sn = sn;
    msg.body.frame.payload = payload;

    msg.header = _ZN_MID_FRAME;
    if (is_reliable)
        msg.header |= _ZN_FLAG_T_R;
    if (is_fragment)
    {
        msg.header |= _ZN_FLAG_T_F;
        if (is_final)
            msg.header |= _ZN_FLAG_T_E;
    }

    msg.attachment = NULL;

    return msg;
}
