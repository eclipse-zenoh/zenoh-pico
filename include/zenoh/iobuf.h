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

#ifndef ZENOH_C_IOBUF_H_
#define ZENOH_C_IOBUF_H_

#include "zenoh/types.h"

typedef struct
{
    size_t r_pos;
    size_t w_pos;
    size_t capacity;
    uint8_t *buf;
} z_iobuf_t;

z_iobuf_t z_iobuf_make(size_t capacity);
z_iobuf_t z_iobuf_wrap(uint8_t *buf, size_t capacity);
z_iobuf_t z_iobuf_wrap_wo(uint8_t *buf, size_t capacity, size_t rpos, size_t wpos);
z_uint8_array_t z_iobuf_to_array(z_iobuf_t *buf);

size_t z_iobuf_readable(const z_iobuf_t *buf);
uint8_t z_iobuf_read(z_iobuf_t *buf);
uint8_t *z_iobuf_read_n(z_iobuf_t *buf, size_t length);
uint8_t *z_iobuf_read_to_n(z_iobuf_t *buf, uint8_t *dest, size_t length);
uint8_t z_iobuf_get(z_iobuf_t *buf, size_t pos);

size_t z_iobuf_writable(const z_iobuf_t *buf);
void z_iobuf_write(z_iobuf_t *buf, uint8_t b);
void z_iobuf_write_slice(z_iobuf_t *buf, const uint8_t *bs, size_t offset, size_t length);
void z_iobuf_write_bytes(z_iobuf_t *buf, const uint8_t *bs, size_t length);
void z_iobuf_put(z_iobuf_t *buf, uint8_t b, size_t pos);

void z_iobuf_clear(z_iobuf_t *buf);
void z_iobuf_compact(z_iobuf_t *buf);
void z_iobuf_free(z_iobuf_t *buf);

#endif /* ZENOH_C_IOBUF_H_ */
