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
#include <stdint.h>
#include "zenoh/result.h"
#include "zenoh/collection.h"

extern const int _z_dummy_arg;

# define Z_UNUSED_ARG(z) (void)(z)
# define Z_UNUSED_ARG_2(z1, z2) (void)(z1); (void)(z2)
# define Z_UNUSED_ARG_3(z1, z2, z3) (void)(z1); (void)(z2); (void)(z3)
# define Z_UNUSED_ARG_4(z1, z2, z3, z4) (void)(z1); (void)(z2); (void)(z3); (void)(z4)
# define Z_UNUSED_ARG_5(z1, z2, z3, z4, z5) (void)(z1); (void)(z2); (void)(z3); (void)(z4); (void)(z5)

typedef size_t z_vle_t;
Z_RESULT_DECLARE (z_vle_t, vle)

ARRAY_DECLARE(uint8_t, uint8, z_)
Z_RESULT_DECLARE (z_uint8_array_t, uint8_array)

Z_RESULT_DECLARE (char*, string)

typedef struct {
  uint8_t clock_id[16];
  z_vle_t time;
} z_timestamp_t;

#endif /* ZENOH_C_TYPES_H_ */
