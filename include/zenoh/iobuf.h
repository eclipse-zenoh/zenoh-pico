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

/*------------------ IOBuf ------------------*/
typedef struct
{
    size_t r_pos;
    size_t w_pos;
    size_t capacity;
    uint8_t *buf;
} _z_iosli_t;

_z_iosli_t _z_iosli_make(size_t capacity);
_z_iosli_t _z_iosli_wrap(uint8_t *buf, size_t capacity);
_z_iosli_t _z_iosli_wrap_wo(uint8_t *buf, size_t capacity, size_t rpos, size_t wpos);

size_t _z_iosli_readable(const _z_iosli_t *ios);
uint8_t _z_iosli_read(_z_iosli_t *ios);
uint8_t *_z_iosli_read_n(_z_iosli_t *ios, size_t length);
uint8_t *_z_iosli_read_to_n(_z_iosli_t *ios, uint8_t *dest, size_t length);
uint8_t _z_iosli_get(_z_iosli_t *ios, size_t pos);

size_t _z_iosli_writable(const _z_iosli_t *ios);
int _z_iosli_write(_z_iosli_t *ios, uint8_t b);
int _z_iosli_write_slice(_z_iosli_t *ios, const uint8_t *bs, size_t offset, size_t length);
int _z_iosli_write_bytes(_z_iosli_t *ios, const uint8_t *bs, size_t length);
void _z_iosli_put(_z_iosli_t *ios, uint8_t b, size_t pos);

int _z_iosli_resize(_z_iosli_t *ios, size_t capacity);
void _z_iosli_clear(_z_iosli_t *ios);
void _z_iosli_compact(_z_iosli_t *ios);
void _z_iosli_free(_z_iosli_t *ios);

/*------------------ IOBuf ------------------*/
#define Z_IOBUF_MODE_CONTIGOUS 0x01
#define Z_IOBUF_MODE_EXPANDABLE 0x02

typedef struct
{
    unsigned int mode;
    size_t r_idx;
    size_t w_idx;
    size_t chunk;
    z_vec_t ioss;
} z_iobuf_t;

z_iobuf_t z_iobuf_make(size_t capacity, unsigned int mode);
int z_iobuf_is_contigous(const z_iobuf_t *iob);
int z_iobuf_is_expandable(const z_iobuf_t *iob);

size_t z_iobuf_writable(const z_iobuf_t *iob);
int z_iobuf_write(z_iobuf_t *iob, uint8_t b);
int z_iobuf_write_slice(z_iobuf_t *iob, const uint8_t *bs, size_t offset, size_t length);
int z_iobuf_write_bytes(z_iobuf_t *iob, const uint8_t *bs, size_t length);
void z_iobuf_put(z_iobuf_t *iob, uint8_t b, size_t pos);

size_t z_iobuf_readable(const z_iobuf_t *iob);
uint8_t z_iobuf_read(z_iobuf_t *iob);
uint8_t *z_iobuf_read_to_n(z_iobuf_t *iob, uint8_t *dest, size_t length);
uint8_t *z_iobuf_read_n(z_iobuf_t *iob, size_t length);
uint8_t z_iobuf_get(z_iobuf_t *iob, size_t pos);

void z_iobuf_clear(z_iobuf_t *iob);
void z_iobuf_compact(z_iobuf_t *iob);
void z_iobuf_free(z_iobuf_t *iob);

#endif /* ZENOH_C_IOBUF_H_ */
