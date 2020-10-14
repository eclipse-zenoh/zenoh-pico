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

#ifndef ZENOH_C_TYPES_H_
#define ZENOH_C_TYPES_H_

#include <stdlib.h>
#include "zenoh/result.h"
#include "zenoh/collection.h"

extern const int _z_dummy_arg;

#define Z_UNUSED_ARG(z) (void)(z)
#define Z_UNUSED_ARG_2(z1, z2) \
    (void)(z1);                \
    (void)(z2)
#define Z_UNUSED_ARG_3(z1, z2, z3) \
    (void)(z1);                    \
    (void)(z2);                    \
    (void)(z3)
#define Z_UNUSED_ARG_4(z1, z2, z3, z4) \
    (void)(z1);                        \
    (void)(z2);                        \
    (void)(z3);                        \
    (void)(z4)
#define Z_UNUSED_ARG_5(z1, z2, z3, z4, z5) \
    (void)(z1);                            \
    (void)(z2);                            \
    (void)(z3);                            \
    (void)(z4);                            \
    (void)(z5)

typedef size_t z_zint_t;
Z_RESULT_DECLARE(z_zint_t, zint)

ARRAY_DECLARE(uint8_t, uint8, z_)
Z_RESULT_DECLARE(z_uint8_array_t, uint8_array)

typedef char *z_string_t;
Z_RESULT_DECLARE(z_string_t, string)

// -- Timestamp is optionally included in the DataInfo Field
//
//  7 6 5 4 3 2 1 0
// +-+-+-+---------+
// ~     Time      ~  Encoded as z_zint_t
// +---------------+
// ~      ID       ~
// +---------------+
typedef struct
{
    uint64_t time;
    z_uint8_array_t id;
} z_timestamp_t;
Z_RESULT_DECLARE(z_timestamp_t, timestamp)

#endif /* ZENOH_C_TYPES_H_ */
