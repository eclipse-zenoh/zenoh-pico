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

/*------------------ IOSli ------------------*/
typedef struct
{
    size_t r_pos;
    size_t w_pos;
    size_t capacity;
    int is_alloc;
    uint8_t *buf;
} z_iosli_t;

z_iosli_t z_iosli_wrap(uint8_t *buf, size_t capacity, size_t r_pos, size_t w_pos);
z_iosli_t z_iosli_make(size_t capacity);

size_t z_iosli_readable(const z_iosli_t *ios);
uint8_t z_iosli_read(z_iosli_t *ios);
void z_iosli_read_bytes(z_iosli_t *ios, uint8_t *dest, size_t offset, size_t length);
uint8_t z_iosli_get(const z_iosli_t *ios, size_t pos);

size_t z_iosli_writable(const z_iosli_t *ios);
void z_iosli_write(z_iosli_t *ios, uint8_t b);
void z_iosli_write_bytes(z_iosli_t *ios, const uint8_t *bs, size_t offset, size_t length);
void z_iosli_put(z_iosli_t *ios, uint8_t b, size_t pos);

z_uint8_array_t z_iosli_to_array(const z_iosli_t *ios);

void z_iosli_clear(z_iosli_t *ios);
void z_iosli_free(z_iosli_t *ios);

/*------------------ RBuf ------------------*/
typedef struct
{
    z_iosli_t ios;
} z_rbuf_t;

z_rbuf_t z_rbuf_make(size_t capacity);
z_rbuf_t z_rbuf_view(z_rbuf_t *rbf, size_t length);

size_t z_rbuf_capacity(const z_rbuf_t *rbf);
size_t z_rbuf_len(const z_rbuf_t *rbf);
size_t z_rbuf_space_left(const z_rbuf_t *rbf);

uint8_t z_rbuf_read(z_rbuf_t *rbf);
void z_rbuf_read_bytes(z_rbuf_t *rbf, uint8_t *dest, size_t offset, size_t length);
uint8_t z_rbuf_get(const z_rbuf_t *rbf, size_t pos);

size_t z_rbuf_get_rpos(const z_rbuf_t *rbf);
size_t z_rbuf_get_wpos(const z_rbuf_t *rbf);
void z_rbuf_set_rpos(z_rbuf_t *rbf, size_t r_pos);
void z_rbuf_set_wpos(z_rbuf_t *rbf, size_t w_pos);

uint8_t *z_rbuf_get_rptr(const z_rbuf_t *rbf);
uint8_t *z_rbuf_get_wptr(const z_rbuf_t *rbf);

void z_rbuf_clear(z_rbuf_t *rbf);
void z_rbuf_compact(z_rbuf_t *rbf);
void z_rbuf_free(z_rbuf_t *rbf);

/*------------------ WBuf ------------------*/
typedef struct
{
    size_t r_idx;
    size_t w_idx;
    size_t capacity;
    z_vec_t ioss;
    int is_expandable;
} z_wbuf_t;

z_wbuf_t z_wbuf_make(size_t capacity, int is_expandable);

size_t z_wbuf_capacity(const z_wbuf_t *wbf);
size_t z_wbuf_len(const z_wbuf_t *wbf);
size_t z_wbuf_space_left(const z_wbuf_t *wbf);

int z_wbuf_write(z_wbuf_t *wbf, uint8_t b);
int z_wbuf_write_bytes(z_wbuf_t *wbf, const uint8_t *bs, size_t offset, size_t length);
void z_wbuf_put(z_wbuf_t *wbf, uint8_t b, size_t pos);

size_t z_wbuf_get_rpos(const z_wbuf_t *wbf);
size_t z_wbuf_get_wpos(const z_wbuf_t *wbf);
void z_wbuf_set_rpos(z_wbuf_t *wbf, size_t r_pos);
void z_wbuf_set_wpos(z_wbuf_t *wbf, size_t w_pos);

void z_wbuf_add_iosli(z_wbuf_t *wbf, z_iosli_t *ios);
void z_wbuf_add_iosli_from(z_wbuf_t *wbf, uint8_t *buf, size_t capacity);
z_iosli_t *z_wbuf_get_iosli(const z_wbuf_t *wbf, size_t idx);
size_t z_wbuf_len_iosli(const z_wbuf_t *wbf);

z_rbuf_t z_wbuf_to_rbuf(const z_wbuf_t *wbf);
int z_wbuf_copy_into(z_wbuf_t *dst, z_wbuf_t *src, size_t length);

void z_wbuf_clear(z_wbuf_t *wbf);
void z_wbuf_reset(z_wbuf_t *wbf);
void z_wbuf_free(z_wbuf_t *wbf);

#endif /* ZENOH_C_IOBUF_H_ */
