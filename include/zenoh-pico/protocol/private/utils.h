
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

#ifndef _ZENOH_PICO_PROTOCOL_PRIVATE_UTILS_H
#define _ZENOH_PICO_PROTOCOL_PRIVATE_UTILS_H

#include <stdint.h>
#include "zenoh-pico/protocol/types.h"
#include "zenoh-pico/protocol/private/msg.h"

/*------------------ Message helper ------------------*/
_zn_transport_message_t _zn_transport_message_init(uint8_t header);
_zn_zenoh_message_t _zn_zenoh_message_init(uint8_t header);
_zn_reply_context_t *_zn_reply_context_init(void);
_zn_attachment_t *_zn_attachment_init(void);

/*------------------ Clone/Copy/Free helpers ------------------*/
zn_reskey_t _zn_reskey_clone(const zn_reskey_t *resky);
z_timestamp_t z_timestamp_clone(const z_timestamp_t *tstamp);
void z_timestamp_reset(z_timestamp_t *tstamp);

#endif /* _ZENOH_PICO_PROTOCOL_PRIVATE_UTILS_H */
