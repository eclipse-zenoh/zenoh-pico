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

#include <stdint.h>

/**
 * Converts an UUID in string format to a byte array.
 *
 * Parameters:
 *   bytes: Pointer to an already allocated byte array of size (at least) 16 bytes.
 *   uuid_str: A valid UUID string.
 */
void _z_uuid_to_bytes(uint8_t *bytes, const char *uuid_str);
