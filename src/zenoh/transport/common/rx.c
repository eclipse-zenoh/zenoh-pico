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

#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/transport/link/rx.h"

/*------------------ Reception helper ------------------*/
_zn_transport_message_p_result_t _zn_recv_t_msg(_zn_transport_t *zt)
{
    _zn_transport_message_p_result_t r;
    _zn_transport_message_p_result_init(&r);

    if (zt->type == _ZN_TRANSPORT_UNICAST_TYPE)
        _zn_unicast_recv_t_msg_na(&zt->transport.unicast, &r);
    else if (zt->type == _ZN_TRANSPORT_MULTICAST_TYPE)
        _zn_multicast_recv_t_msg_na(&zt->transport.multicast, &r);

    return r;
}

void _zn_recv_t_msg_na(_zn_transport_t *zt, _zn_transport_message_p_result_t *r)
{
    if (zt->type == _ZN_TRANSPORT_UNICAST_TYPE)
        _zn_unicast_recv_t_msg_na(&zt->transport.unicast, r);
    else if (zt->type == _ZN_TRANSPORT_MULTICAST_TYPE)
        _zn_multicast_recv_t_msg_na(&zt->transport.multicast, r);
}

int _zn_handle_transport_message(_zn_transport_t *zt, _zn_transport_message_t *msg)
{
    if (zt->type == _ZN_TRANSPORT_UNICAST_TYPE)
        return _zn_unicast_handle_transport_message(&zt->transport.unicast, msg);
    else if (zt->type == _ZN_TRANSPORT_MULTICAST_TYPE)
        return _zn_multicast_handle_transport_message(&zt->transport.multicast, msg);
    else
        return -1;
}
