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

#ifndef ZENOH_C_NET_PRIVATE_SESSION_H
#define ZENOH_C_NET_PRIVATE_SESSION_H

#include "zenoh/net/types.h"

zn_session_t *_zn_session_init();
void _zn_session_close(zn_session_t *z, unsigned int reason);
void _zn_session_free(zn_session_t *z);

int _zn_handle_session_message();
int _zn_handle_zenoh_message(zn_session_t *z);

#endif /* ZENOH_C_NET_PRIVATE_SESSION_H */
