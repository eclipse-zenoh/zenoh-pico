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

#ifndef ZENOH_PICO_RESOURCE_API_H
#define ZENOH_PICO_RESOURCE_API_H

#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/collections/list.h"

/**
 * Create a resource key from a resource id.
 *
 * Parameters:
 *     rid: The resource id.
 *
 * Returns:
 *     Return a new resource key.
 */
zn_reskey_t zn_rid(unsigned long rid);

/**
 * Create a resource key from a resource name.
 *
 * Parameters:
 *     rname: The resource name.
 *
 * Returns:
 *     Return a new resource key.
 */
zn_reskey_t zn_rname(const z_str_t rname);

/**
 * Create a resource key from a resource id and a suffix.
 *
 * Parameters:
 *     id: The resource id.
 *     suffix: The suffix.
 *
 * Returns:
 *     A new resource key.
 */
zn_reskey_t zn_rid_with_suffix(unsigned long id, const z_str_t suffix);

#endif /* ZENOH_PICO_RESOURCE_API_H */
