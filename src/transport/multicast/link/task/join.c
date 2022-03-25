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

#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/transport/link/tx.h"
#include "zenoh-pico/transport/link/task/join.h"

int _zp_multicast_send_join(_z_transport_multicast_t *ztm)
{
    // FIXME: make transport aware of qos configuration
    _z_conduit_sn_list_t next_sns;
    next_sns.is_qos = 0;
    next_sns.val.plain.best_effort = ztm->sn_tx_best_effort;
    next_sns.val.plain.reliable = ztm->sn_tx_reliable;

    _z_bytes_t pid = _z_bytes_wrap(((_z_session_t *)ztm->session)->tp_manager->local_pid.val, ((_z_session_t *)ztm->session)->tp_manager->local_pid.len);
    _z_transport_message_t jsm = _z_t_msg_make_join(Z_PROTO_VERSION, Z_PEER, Z_TRANSPORT_LEASE, Z_SN_RESOLUTION, pid, next_sns);

    return _z_multicast_send_t_msg(ztm, &jsm);
}
