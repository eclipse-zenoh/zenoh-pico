//
// Copyright (c) 2022 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   ZettaScale Zenoh Team, <zenoh@zettascale.tech>
//

#ifndef ZENOH_PICO_RESOURCE_API_H
#define ZENOH_PICO_RESOURCE_API_H

#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/collections/string.h"

/**
 * Create a resource key from a resource id.
 *
 * Parameters:
 *     rid: The resource id.
 *
 * Returns:
 *     A :c:type:`zn_reskey_t` containing a new resource key.
 */
zn_reskey_t zn_rid(unsigned long rid);

/**
 * Create a resource key from a resource name.
 *
 * Parameters:
 *     rname: The resource name. The caller keeps its ownership.
 *
 * Returns:
 *     A :c:type:`zn_reskey_t` containing a new resource key.
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
 *     A :c:type:`zn_reskey_t` containing a new resource key.
 */
zn_reskey_t zn_rid_with_suffix(unsigned long rid, const z_str_t suffix);

#endif /* ZENOH_PICO_RESOURCE_API_H */
