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

#ifndef ZENOH_PICO_SYSTEM_LINK_BT_H
#define ZENOH_PICO_SYSTEM_LINK_BT_H

#include <stdint.h>
#include "zenoh-pico/config.h"
#include "zenoh-pico/collections/string.h"

#if ZN_LINK_BLUETOOTH == 1

#ifdef __cplusplus
extern "C" {
#endif

#define _ZN_BT_MODE_MASTER 0
#define _ZN_BT_MODE_SLAVE  1

#define _ZN_BT_PROFILE_UNSUPPORTED 255
#define _ZN_BT_PROFILE_SPP 0

typedef struct
{
    void *sock;
    z_str_t gname;
} _zn_bt_socket_t;

void *_zn_open_bt(z_str_t gname, uint8_t mode, uint8_t profile);
void *_zn_listen_bt(z_str_t gname, uint8_t mode, uint8_t profile);
void _zn_close_bt(void *);
size_t _zn_read_exact_bt(void *, uint8_t *ptr, size_t len);
size_t _zn_read_bt(void *, uint8_t *ptr, size_t len);
size_t _zn_send_bt(void *, const uint8_t *ptr, size_t len);

#ifdef __cplusplus
}
#endif

#endif

#endif /* ZENOH_PICO_SYSTEM_LINK_BT_H */
