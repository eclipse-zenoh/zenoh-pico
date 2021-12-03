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

#ifndef ZENOH_PICO_PROTOCOL_UTILS_H
#define ZENOH_PICO_PROTOCOL_UTILS_H

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/msg.h"

/**
 * Intersects two resource names. This function compares two resource names
 * and verifies that the first resource name intersects (i.e., matches) the
 * second resource name. E.g., /foo/\* interesects /foo/a.
 *
 * Parameters:
 *     left: The resource name to match against.
 *     right: The resource name to be compared.
 * Returns:
 *     ``0`` in case of success, ``-1`` in case of failure.
 */
int zn_rname_intersect(const z_str_t left, const z_str_t right);

/*------------------ clone/Copy/Free helpers ------------------*/
zn_reskey_t _zn_reskey_duplicate(const zn_reskey_t *resky);
z_timestamp_t z_timestamp_duplicate(const z_timestamp_t *tstamp);
void z_timestamp_reset(z_timestamp_t *tstamp);

#endif /* ZENOH_PICO_PROTOCOL_UTILS_H */
