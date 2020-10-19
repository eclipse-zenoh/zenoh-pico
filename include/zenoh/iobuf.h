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
} z_iobuf_t;

z_iobuf_t z_iobuf_make(size_t capacity);
z_iobuf_t z_iobuf_wrap(uint8_t *buf, size_t capacity);
z_iobuf_t z_iobuf_wrap_wo(uint8_t *buf, size_t capacity, size_t rpos, size_t wpos);

size_t z_iobuf_readable(const z_iobuf_t *iob);
uint8_t z_iobuf_read(z_iobuf_t *iob);
uint8_t *z_iobuf_read_n(z_iobuf_t *iob, size_t length);
uint8_t *z_iobuf_read_to_n(z_iobuf_t *iob, uint8_t *dest, size_t length);
uint8_t z_iobuf_get(z_iobuf_t *iob, size_t pos);

size_t z_iobuf_writable(const z_iobuf_t *iob);
int z_iobuf_write(z_iobuf_t *iob, uint8_t b);
int z_iobuf_write_slice(z_iobuf_t *iob, const uint8_t *bs, size_t offset, size_t length);
int z_iobuf_write_bytes(z_iobuf_t *iob, const uint8_t *bs, size_t length);
void z_iobuf_put(z_iobuf_t *iob, uint8_t b, size_t pos);

void z_iobuf_clear(z_iobuf_t *iob);
void z_iobuf_compact(z_iobuf_t *iob);
void z_iobuf_free(z_iobuf_t *iob);

/*------------------ WBuf ------------------*/
typedef struct
{
    int is_expandable;
    int is_contigous;
    size_t idx;
    size_t capacity;
    z_vec_t iobs;
} z_wbuf_t;

z_wbuf_t z_wbuf_make(size_t capacity, int is_expandable, int is_contigous);
size_t z_iobuf_writable(const z_iobuf_t *iob);
int z_iobuf_write(z_iobuf_t *iob, uint8_t b);
int z_iobuf_write_slice(z_iobuf_t *iob, const uint8_t *bs, size_t offset, size_t length);
int z_iobuf_write_bytes(z_iobuf_t *iob, const uint8_t *bs, size_t length);
void z_iobuf_put(z_iobuf_t *iob, uint8_t b, size_t pos);

/*------------------ RBuf ------------------*/
typedef struct
{
    size_t r_idx;
    z_vec_t slis;
} z_rbuf_t;

size_t z_rbuf_readable(const z_iobuf_t *iob);
uint8_t z_iobuf_read(z_iobuf_t *iob);
uint8_t *z_iobuf_read_to_n(z_iobuf_t *iob, uint8_t *dest, size_t length);
uint8_t *z_iobuf_read_n(z_iobuf_t *iob, size_t length);
uint8_t z_iobuf_get(z_iobuf_t *iob, size_t pos);

#endif /* ZENOH_C_IOBUF_H_ */
