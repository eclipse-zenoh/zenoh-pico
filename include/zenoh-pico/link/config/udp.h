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

#ifndef ZENOH_PICO_LINK_CONFIG_UDP_H
#define ZENOH_PICO_LINK_CONFIG_UDP_H

#include "zenoh-pico/utils/collections.h"
#include "zenoh-pico/utils/properties.h"

#define UDP_CONFIG_MULTICAST_IFACE_KEY 0x01
#define UDP_CONFIG_MULTICAST_IFACE_STR "iface"

zn_properties_result_t _zn_udp_config_from_str(const z_str_t s);

#endif /* ZENOH_PICO_LINK_CONFIG_UDP_H */