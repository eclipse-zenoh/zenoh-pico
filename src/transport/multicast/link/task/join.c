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

int _znp_multicast_send_join(_zn_transport_multicast_t *ztm)
{
    // FIXME: make transport aware of qos configuration
    _zn_conduit_sn_list_t next_sns;
    next_sns.is_qos = 0;
    next_sns.val.plain.best_effort = ztm->sn_tx_best_effort;
    next_sns.val.plain.reliable = ztm->sn_tx_reliable;

    _zn_transport_message_t jsm = _zn_t_msg_make_join(ZN_PROTO_VERSION, ZN_PEER, ZN_TRANSPORT_LEASE, ZN_SN_RESOLUTION, ((zn_session_t *)ztm->session)->tp_manager->local_pid, next_sns);

    return _zn_multicast_send_t_msg(ztm, &jsm);
}
