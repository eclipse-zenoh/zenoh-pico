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

#include <string.h>
#include <assert.h>
#include "zenoh/iobuf.h"

z_iobuf_t z_iobuf_wrap_wo(unsigned char *buf, unsigned int capacity, unsigned int rpos, unsigned int wpos) {
  assert(rpos <= capacity && wpos <= capacity);
  z_iobuf_t iobuf;
  iobuf.r_pos = rpos;
  iobuf.w_pos = wpos;
  iobuf.capacity = capacity;
  iobuf.buf = buf;
  return iobuf;
}

z_iobuf_t
z_iobuf_wrap(uint8_t *buf, unsigned int capacity) {
  return z_iobuf_wrap_wo(buf, capacity, 0, 0);
}


z_iobuf_t z_iobuf_make(unsigned int capacity) {
  return z_iobuf_wrap((uint8_t*)malloc(capacity), capacity);
}


void z_iobuf_free(z_iobuf_t* buf) {
  buf->r_pos = 0;
  buf->w_pos = 0;
  buf->capacity = 0;
  free(buf->buf);
  buf = 0;
}

unsigned int z_iobuf_readable(const z_iobuf_t* iob) {
  return iob->w_pos - iob->r_pos;
}
unsigned int z_iobuf_writable(const z_iobuf_t* iob) {
  return iob->capacity - iob->w_pos;
}

void z_iobuf_write(z_iobuf_t* iob, uint8_t b) {
  assert(iob->w_pos < iob->capacity);
  iob->buf[iob->w_pos++] = b;
}

void z_iobuf_write_slice(z_iobuf_t* iob, const uint8_t* bs, unsigned int offset, unsigned int length) {
  assert(z_iobuf_writable(iob) >= length);
  memcpy(iob->buf + iob->w_pos, bs + offset, length);
  iob->w_pos += length;
}

void z_iobuf_write_bytes(z_iobuf_t* iob, const unsigned char *bs, unsigned int length) {
  assert(z_iobuf_writable(iob) >= length);
  memcpy(iob->buf + iob->w_pos, bs, length);
  iob->w_pos += length;
}

uint8_t z_iobuf_read(z_iobuf_t* iob) {
  assert(iob->r_pos < iob->w_pos);
  return iob->buf[iob->r_pos++];
}


uint8_t* z_iobuf_read_to_n(z_iobuf_t* iob, uint8_t* dst, unsigned int length) {
  assert(z_iobuf_readable(iob) >= length);
  memcpy(dst, iob->buf + iob->r_pos, length);
  iob->r_pos += length;
  return dst;

}

uint8_t* z_iobuf_read_n(z_iobuf_t* iob, unsigned int length) {
  uint8_t* dst = (uint8_t*)malloc(length);
  return z_iobuf_read_to_n(iob, dst, length);
}


void z_iobuf_put(z_iobuf_t* iob, uint8_t b, unsigned int pos) {
  assert(pos < iob->capacity);
  iob->buf[pos] = b;
}

uint8_t z_iobuf_sget(z_iobuf_t* iob, unsigned int pos) {
  assert(pos < iob->capacity);
  return iob->buf[pos];
}

void z_iobuf_clear(z_iobuf_t *buf) {
  buf->r_pos = 0;
  buf->w_pos = 0;
}

z_uint8_array_t z_iobuf_to_array(z_iobuf_t* buf) {
  z_uint8_array_t a = {z_iobuf_readable(buf), &buf->buf[buf->r_pos]};
  return a;
}

void z_iobuf_compact(z_iobuf_t *buf) {
  if (buf->r_pos == 0 && buf->w_pos == 0) {
    return;
  }
  size_t len = buf->w_pos - buf->r_pos;
  uint8_t *cp = buf->buf + buf->r_pos;
  memcpy(buf->buf, cp, len);
  buf->r_pos = 0;
  buf->w_pos = len;
}
