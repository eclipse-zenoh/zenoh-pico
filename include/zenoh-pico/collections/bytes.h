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

#ifndef ZENOH_PICO_COLLECTIONS_BYTES_H
#define ZENOH_PICO_COLLECTIONS_BYTES_H

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
typedef struct
{
    const uint8_t *val;
    size_t len;
    uint8_t _is_alloc;
} _z_bytes_t;

void _z_bytes_init(_z_bytes_t *bs, size_t capacity);
_z_bytes_t _z_bytes_make(size_t capacity);
_z_bytes_t _z_bytes_wrap(const uint8_t *bs, size_t len);

void _z_bytes_copy(_z_bytes_t *dst, const _z_bytes_t *src);
_z_bytes_t _z_bytes_duplicate(const _z_bytes_t *src);
void _z_bytes_move(_z_bytes_t *dst, _z_bytes_t *src);
void _z_bytes_reset(_z_bytes_t *bs);
int _z_bytes_is_empty(const _z_bytes_t *bs);

int _z_bytes_eq(const _z_bytes_t *left, const _z_bytes_t *right);
void _z_bytes_clear(_z_bytes_t *bs);
void _z_bytes_free(_z_bytes_t **bs);

#endif /* ZENOH_PICO_COLLECTIONS_BYTES_H */
