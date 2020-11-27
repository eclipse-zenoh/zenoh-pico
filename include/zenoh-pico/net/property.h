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

#ifndef ZENOH_PICO_PROPERTY_H
#define ZENOH_PICO_PROPERTY_H

#include <stdint.h>
#include "zenoh-pico/net/types.h"

// Properties returned by zn_info()
#define ZN_INFO_PID_KEY 0x00
#define ZN_INFO_PEER_PID_KEY 0x01
#define ZN_INFO_ROUTER_PID_KEY 0x02

/**
 * Return a new empty map of properties.
 */
zn_properties_t *zn_properties_make(void);

/**
 * Insert a property with a given key to a properties map.
 * If a property with the same key already exists in the properties map, it is replaced.
 *
 * Parameters:
 *   ps: A pointer to the properties map.
 *   key: The key of the property to add.
 *   value: The value of the property to add.
 *
 * Returns:
 *     A pointer to the updated properties map.
 */
zn_properties_t *zn_properties_insert(zn_properties_t *ps, unsigned int key, z_string_t value);

/**
 * Get the property with the given key from a properties map.
 *
 * Parameters:
 *     ps: A pointer to properties map.
 *     key: The key of the property.
 *
 * Returns:
 *     The value of the property with key ``key`` in properties map ``ps``.
 */
z_string_t zn_properties_get(zn_properties_t *ps, unsigned int key);

/**
 * Get the length of the given properties map.
 *
 * Parameters:
 *     ps: A pointer to the properties map.
 *
 * Returns:
 *     The length of the given properties map.
 */
unsigned int zn_properties_len(zn_properties_t *ps);

/**
 * Free a set of properties.
 *
 * Parameters:
 *   ps: A pointer to the properties.
 */
void zn_properties_free(zn_properties_t *ps);

#endif /* ZENOH_PICO_PROPERTY_H */
