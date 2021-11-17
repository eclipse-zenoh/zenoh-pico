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

#ifndef ZENOH_PICO_UTILS_COLLECTION_BYTES_H
#define ZENOH_PICO_UTILS_COLLECTION_BYTES_H

#include <stdint.h>
#include <stdio.h>

/*-------- Bytes --------*/
/**
 * An array of bytes.
 *
 * Members:
 *   size_t len: The length of the bytes array.
 *   uint8_t *val: A pointer to the bytes array.
 */
typedef struct z_bytes_t
{
    size_t len;
    const uint8_t *val;
} z_bytes_t;

z_bytes_t _z_bytes_make(size_t capacity);
void _z_bytes_init(z_bytes_t *bs, size_t capacity);
void _z_bytes_copy(z_bytes_t *dst, const z_bytes_t *src);
void _z_bytes_move(z_bytes_t *dst, z_bytes_t *src);
void _z_bytes_free(z_bytes_t *bs);
void _z_bytes_reset(z_bytes_t *bs);

#endif /* ZENOH_PICO_UTILS_COLLECTION_BYTES_H */
