//
// Copyright (c) 2022 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   ZettaScale Zenoh Team, <zenoh@zettascale.tech>
//

#ifndef ZENOH_PICO_UTILS_ENDIANNESS_H
#define ZENOH_PICO_UTILS_ENDIANNESS_H

#include <stdint.h>
#include <stdlib.h>

// Load int from memory with specified endianness
uint16_t _z_le_load16(const uint8_t *src);
uint32_t _z_le_load32(const uint8_t *src);
uint64_t _z_le_load64(const uint8_t *src);
uint16_t _z_be_load16(const uint8_t *src);
uint32_t _z_be_load32(const uint8_t *src);
uint64_t _z_be_load64(const uint8_t *src);

// Store int to memory with specified endianness
void _z_le_store16(const uint16_t val, uint8_t *dest);
void _z_le_store32(const uint32_t val, uint8_t *dest);
void _z_le_store64(const uint64_t val, uint8_t *dest);
void _z_be_store16(const uint16_t val, uint8_t *dest);
void _z_be_store32(const uint32_t val, uint8_t *dest);
void _z_be_store64(const uint64_t val, uint8_t *dest);

// Load little-endian int from memory to host order
uint16_t _z_host_le_load16(const uint8_t *src);
uint32_t _z_host_le_load32(const uint8_t *src);
uint64_t _z_host_le_load64(const uint8_t *src);

// Store little-endian int to memory from host order
void _z_host_le_store16(const uint16_t val, uint8_t *dst);
void _z_host_le_store32(const uint32_t val, uint8_t *dst);
void _z_host_le_store64(const uint64_t val, uint8_t *dst);

// Return u16 individual bytes
uint8_t _z_get_u16_lsb(uint_fast16_t val);
uint8_t _z_get_u16_msb(uint_fast16_t val);

#endif /* ZENOH_PICO_UTILS_ENDIANNESS_H */
