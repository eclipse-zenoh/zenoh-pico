//
// Copyright (c) 2024 ZettaScale Technology
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

#include "zenoh-pico/utils/endianness.h"

#if !defined(ZENOH_ENDIANNNESS_BIG) && !defined(ZENOH_ENDIANNNESS_LITTLE)
// Gcc/clang test
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define ZENOH_ENDIANNNESS_BIG
#else
#define ZENOH_ENDIANNNESS_LITTLE
#endif
#endif

// *** Little endian ***
uint16_t _z_le_load16(const uint8_t *src) { return (uint16_t)(src[0] << 0) | (uint16_t)(src[1] << 8); }

uint32_t _z_le_load32(const uint8_t *src) {
    return ((uint32_t)src[0] << 0) | ((uint32_t)src[1] << 8) | ((uint32_t)src[2] << 16) | ((uint32_t)src[3] << 24);
}

uint64_t _z_le_load64(const uint8_t *src) {
    return ((uint64_t)src[0] << 0) | ((uint64_t)src[1] << 8) | ((uint64_t)src[2] << 16) | ((uint64_t)src[3] << 24) |
           ((uint64_t)src[4] << 32) | ((uint64_t)src[5] << 40) | ((uint64_t)src[6] << 48) | ((uint64_t)src[7] << 56);
}

void _z_le_store16(const uint16_t val, uint8_t *dst) {
    dst[0] = (uint8_t)(val >> 0);
    dst[1] = (uint8_t)(val >> 8);
}

void _z_le_store32(const uint32_t val, uint8_t *dst) {
    dst[0] = (uint8_t)(val >> 0);
    dst[1] = (uint8_t)(val >> 8);
    dst[2] = (uint8_t)(val >> 16);
    dst[3] = (uint8_t)(val >> 24);
}

void _z_le_store64(const uint64_t val, uint8_t *dst) {
    dst[0] = (uint8_t)(val >> 0);
    dst[1] = (uint8_t)(val >> 8);
    dst[2] = (uint8_t)(val >> 16);
    dst[3] = (uint8_t)(val >> 24);
    dst[4] = (uint8_t)(val >> 32);
    dst[5] = (uint8_t)(val >> 40);
    dst[6] = (uint8_t)(val >> 48);
    dst[7] = (uint8_t)(val >> 56);
}

// *** Big endian ***
uint16_t _z_be_load16(const uint8_t *src) { return (uint16_t)(src[0] << 8) | (uint16_t)(src[1] << 0); }

uint32_t _z_be_load32(const uint8_t *src) {
    return ((uint32_t)src[0] << 24) | ((uint32_t)src[1] << 16) | ((uint32_t)src[2] << 8) | ((uint32_t)src[3] << 0);
}

uint64_t _z_be_load64(const uint8_t *src) {
    return ((uint64_t)src[0] << 56) | ((uint64_t)src[1] << 48) | ((uint64_t)src[2] << 40) | ((uint64_t)src[3] << 32) |
           ((uint64_t)src[4] << 24) | ((uint64_t)src[5] << 16) | ((uint64_t)src[6] << 8) | ((uint64_t)src[7] << 0);
}

void _z_be_store16(const uint16_t val, uint8_t *dst) {
    dst[0] = (uint8_t)(val >> 8);
    dst[1] = (uint8_t)(val >> 0);
}

void _z_be_store32(const uint32_t val, uint8_t *dst) {
    dst[0] = (uint8_t)(val >> 24);
    dst[1] = (uint8_t)(val >> 16);
    dst[2] = (uint8_t)(val >> 8);
    dst[3] = (uint8_t)(val >> 0);
}

void _z_be_store64(const uint64_t val, uint8_t *dst) {
    dst[0] = (uint8_t)(val >> 56);
    dst[1] = (uint8_t)(val >> 48);
    dst[2] = (uint8_t)(val >> 40);
    dst[3] = (uint8_t)(val >> 32);
    dst[4] = (uint8_t)(val >> 24);
    dst[5] = (uint8_t)(val >> 16);
    dst[6] = (uint8_t)(val >> 8);
    dst[7] = (uint8_t)(val >> 0);
}

// *** Host ***
uint16_t _z_host_le_load16(const uint8_t *src) {
#if defined(ZENOH_ENDIANNNESS_BIG)
    return _z_be_load16(src);
#elif defined(ZENOH_ENDIANNNESS_LITTLE)
    return _z_le_load16(src);
#endif
}

uint32_t _z_host_le_load32(const uint8_t *src) {
#if defined(ZENOH_ENDIANNNESS_BIG)
    return _z_be_load32(src);
#elif defined(ZENOH_ENDIANNNESS_LITTLE)
    return _z_le_load32(src);
#endif
}

uint64_t _z_host_le_load64(const uint8_t *src) {
#if defined(ZENOH_ENDIANNNESS_BIG)
    return _z_be_load64(src);
#elif defined(ZENOH_ENDIANNNESS_LITTLE)
    return _z_le_load64(src);
#endif
}

void _z_host_le_store16(const uint16_t val, uint8_t *dst) {
#if defined(ZENOH_ENDIANNNESS_BIG)
    _z_be_store16(val, dst);
#elif defined(ZENOH_ENDIANNNESS_LITTLE)
    _z_le_store16(val, dst);
#endif
}
void _z_host_le_store32(const uint32_t val, uint8_t *dst) {
#if defined(ZENOH_ENDIANNNESS_BIG)
    _z_be_store32(val, dst);
#elif defined(ZENOH_ENDIANNNESS_LITTLE)
    _z_le_store32(val, dst);
#endif
}

void _z_host_le_store64(const uint64_t val, uint8_t *dst) {
#if defined(ZENOH_ENDIANNNESS_BIG)
    _z_be_store64(val, dst);
#elif defined(ZENOH_ENDIANNNESS_LITTLE)
    _z_le_store64(val, dst);
#endif
}

uint8_t _z_get_u16_lsb(uint_fast16_t val) { return (uint8_t)(val >> 0); }

uint8_t _z_get_u16_msb(uint_fast16_t val) { return (uint8_t)(val >> 8); }
