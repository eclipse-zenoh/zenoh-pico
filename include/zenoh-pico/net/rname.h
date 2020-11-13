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

#ifndef ZENOH_NET_PICO_RNAME_H
#define ZENOH_NET_PICO_RNAME_H

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
int zn_rname_intersect(const char *left, const char *right);

#endif /* ZENOH_NET_PICO_RNAME_H */
