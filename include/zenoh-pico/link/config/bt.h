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

#ifndef ZENOH_PICO_LINK_CONFIG_BT_H
#define ZENOH_PICO_LINK_CONFIG_BT_H

#include "zenoh-pico/config.h"
#include "zenoh-pico/collections/intmap.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/system/platform.h"

#if Z_LINK_BLUETOOTH == 1

#define BT_CONFIG_MODE_KEY     0x01
#define BT_CONFIG_MODE_STR     "mode"

#define BT_CONFIG_RNAME_KEY    0x02
#define BT_CONFIG_RNAME_STR    "rname"

#define BT_CONFIG_LNAME_KEY    0x03
#define BT_CONFIG_LNAME_STR    "lname"

#define BT_CONFIG_PROFILE_KEY  0x04
#define BT_CONFIG_PROFILE_STR  "profile"

#define BT_CONFIG_TOUT_KEY     0x05
#define BT_CONFIG_TOUT_STR     "tout"

#define BT_CONFIG_MAPPING_BUILD          \
    int argc = 5;                        \
    _z_str_intmapping_t args[argc];      \
    args[0].key = BT_CONFIG_MODE_KEY;    \
    args[0].str = BT_CONFIG_MODE_STR;    \
    args[1].key = BT_CONFIG_RNAME_KEY;   \
    args[1].str = BT_CONFIG_RNAME_STR;   \
    args[2].key = BT_CONFIG_LNAME_KEY;   \
    args[2].str = BT_CONFIG_LNAME_STR;   \
    args[3].key = BT_CONFIG_PROFILE_KEY; \
    args[3].str = BT_CONFIG_PROFILE_STR; \
    args[4].key = BT_CONFIG_TOUT_KEY;    \
    args[4].str = BT_CONFIG_TOUT_STR;

size_t _z_bt_config_strlen(const _z_str_intmap_t *s);

void _z_bt_config_onto_str(_z_str_t dst, const _z_str_intmap_t *s);
_z_str_t _z_bt_config_to_str(const _z_str_intmap_t *s);

_z_str_intmap_result_t _z_bt_config_from_str(const _z_str_t s);
_z_str_intmap_result_t _z_bt_config_from_strn(const _z_str_t s, size_t n);
#endif

#endif /* ZENOH_PICO_LINK_CONFIG_BT_H */