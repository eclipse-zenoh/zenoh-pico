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

#include "zenoh-pico/protocol/utils.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/transport/link/tx.h"

int _zn_send_t_msg(_zn_transport_t *zt, _zn_transport_message_t *t_msg)
{
    if (zt->type == _ZN_TRANSPORT_UNICAST_TYPE)
        return _zn_unicast_send_t_msg(&zt->transport.unicast, t_msg);
    else if (zt->type == _ZN_TRANSPORT_MULTICAST_TYPE)
        return _zn_multicast_send_t_msg(&zt->transport.multicast, t_msg);
    else
        return -1;
}