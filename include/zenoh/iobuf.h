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

#include <stdint.h>
#include "zenoh/types.h"

/*------------------ IOBuf ------------------*/
typedef struct
{
    size_t r_pos;
    size_t w_pos;
    size_t capacity;
    uint8_t *buf;
} z_iosli_t;

z_iosli_t z_iosli_make(size_t capacity);
z_iosli_t *z_iosli_alloc(size_t capacity);
z_iosli_t z_iosli_wrap(uint8_t *buf, size_t capacity);
z_iosli_t z_iosli_wrap_wo(uint8_t *buf, size_t capacity, size_t r_pos, size_t w_pos);

size_t z_iosli_readable(const z_iosli_t *ios);
uint8_t z_iosli_read(z_iosli_t *ios);
uint8_t *z_iosli_read_n(z_iosli_t *ios, size_t length);
void z_iosli_read_to_n(z_iosli_t *ios, uint8_t *dest, size_t length);
uint8_t z_iosli_get(const z_iosli_t *ios, size_t pos);

size_t z_iosli_writable(const z_iosli_t *ios);
int z_iosli_write(z_iosli_t *ios, uint8_t b);
int z_iosli_write_slice(z_iosli_t *ios, const uint8_t *bs, size_t offset, size_t length);
int z_iosli_write_bytes(z_iosli_t *ios, const uint8_t *bs, size_t length);
void z_iosli_put(z_iosli_t *ios, uint8_t b, size_t pos);

int z_iosli_resize(z_iosli_t *ios, size_t capacity);
void z_iosli_clear(z_iosli_t *ios);
void z_iosli_compact(z_iosli_t *ios);
void z_iosli_free(z_iosli_t *ios);

/*------------------ IOBuf ------------------*/
#define Z_IOBUF_MODE_CONTIGOUS 0x01  // 1 << 0
#define Z_IOBUF_MODE_EXPANDABLE 0x02 // 1 << 1

typedef struct
{
    union
    {
        z_iosli_t cios;
        struct
        {
            size_t r_idx;
            size_t w_idx;
            size_t chunk;
            z_vec_t ioss;
        } eios;
    } value;
    unsigned int mode;
} z_iobuf_t;

z_iobuf_t z_iobuf_make(size_t capacity, unsigned int mode);
z_iobuf_t z_iobuf_wrap(uint8_t *buf, size_t capacity);
z_iobuf_t z_iobuf_wrap_wo(uint8_t *buf, size_t capacity, size_t r_pos, size_t w_pos);

int z_iobuf_is_contigous(const z_iobuf_t *iob);
int z_iobuf_is_expandable(const z_iobuf_t *iob);
void z_iobuf_add_iosli(z_iobuf_t *iob, const z_iosli_t *ios);
z_iosli_t *z_iobuf_get_iosli(const z_iobuf_t *iob, unsigned int idx);
size_t z_iobuf_len_iosli(const z_iobuf_t *iob);

size_t z_iobuf_writable(const z_iobuf_t *iob);
int z_iobuf_write(z_iobuf_t *iob, uint8_t b);
int z_iobuf_write_slice(z_iobuf_t *iob, const uint8_t *bs, size_t offset, size_t length);
int z_iobuf_write_bytes(z_iobuf_t *iob, const uint8_t *bs, size_t length);
void z_iobuf_put(z_iobuf_t *iob, uint8_t b, size_t pos);

size_t z_iobuf_readable(const z_iobuf_t *iob);
uint8_t z_iobuf_read(z_iobuf_t *iob);
void z_iobuf_read_to_n(z_iobuf_t *iob, uint8_t *dest, size_t length);
uint8_t *z_iobuf_read_n(z_iobuf_t *iob, size_t length);
uint8_t z_iobuf_get(const z_iobuf_t *iob, size_t pos);

size_t z_iobuf_get_rpos(const z_iobuf_t *iob);
size_t z_iobuf_get_wpos(const z_iobuf_t *iob);
void z_iobuf_set_rpos(z_iobuf_t *iob, size_t r_pos);
void z_iobuf_set_wpos(z_iobuf_t *iob, size_t w_pos);

z_uint8_array_t z_iobuf_to_array(const z_iobuf_t *iob);
int z_iobuf_copy_into(z_iobuf_t *dst, const z_iobuf_t *src);

void z_iobuf_clear(z_iobuf_t *iob);
void z_iobuf_compact(z_iobuf_t *iob);
void z_iobuf_free(z_iobuf_t *iob);

#endif /* ZENOH_C_IOBUF_H_ */
