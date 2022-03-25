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

#ifndef ZENOH_PICO_RESOURCE_NETAPI_H
#define ZENOH_PICO_RESOURCE_NETAPI_H

#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/collections/string.h"

/**
 * Create a resource key from a resource id.
 *
 * Parameters:
 *     rid: The resource id.
 *
 * Returns:
 *     A :c:type:`_z_reskey_t` containing a new resource key.
 */
_z_reskey_t _z_rid(unsigned long rid);

/**
 * Create a resource key from a resource name.
 *
 * Parameters:
 *     rname: The resource name. The caller keeps its ownership.
 *
 * Returns:
 *     A :c:type:`_z_reskey_t` containing a new resource key.
 */
_z_reskey_t _z_rname(const _z_str_t rname);

/**
 * Create a resource key from a resource id and a suffix.
 *
 * Parameters:
 *     id: The resource id.
 *     suffix: The suffix.
 *
 * Returns:
 *     A :c:type:`_z_reskey_t` containing a new resource key.
 */
_z_reskey_t _z_rid_with_suffix(unsigned long rid, const _z_str_t suffix);

#endif /* ZENOH_PICO_RESOURCE_NETAPI_H */
