//
// Copyright (c) 2026 ZettaScale Technology
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

// Tests for static_bit_vector_template.h in IS_SET mode (bitset: fixed-full-capacity).
// A bitset has no _size field; its logical size is always equal to its capacity.
// push_back / append / pop_back / front / back / insert / remove / remove_at / swap_remove
// are intentionally absent in this mode.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#undef NDEBUG
#include <assert.h>

#include "zenoh-pico/collections/algorithms_template.h"

// ── Instantiate a bitset, capacity 20 (spans more than one storage word) ────

#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_IS_SET 1
#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME bitset
#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_SIZE 20
#include "zenoh-pico/collections/static_bit_vector_template.h"

// ── Default name derivation: no NAME defined → bitset_8_t ────────────────────

#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_IS_SET 1
#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_SIZE 8
#include "zenoh-pico/collections/static_bit_vector_template.h"

// ── uint8_t-backed bitset to exercise byte-granular storage ─────────────────

#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_IS_SET 1
#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME byteset
#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_SIZE 20
#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_TYPE uint8_t
#include "zenoh-pico/collections/static_bit_vector_template.h"

// ── uint32_t-backed bitset to exercise wide storage blocks ──────────────────

#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_IS_SET 1
#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME wordset
#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_SIZE 40
#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_TYPE uint32_t
#include "zenoh-pico/collections/static_bit_vector_template.h"

// ── Tests ─────────────────────────────────────────────────────────────────────

// The defining property of a bitset: size() always equals capacity().
static void test_size_is_capacity(void) {
    printf("Test: size() always equals capacity() in bitset mode\n");
    bitset_t s = bitset_new();
    assert(bitset_size(&s) == 20);
    assert(bitset_size(&s) == bitset_capacity(&s));

    byteset_t b = byteset_new();
    assert(byteset_size(&b) == 20);
    assert(byteset_size(&b) == byteset_capacity(&b));

    wordset_t w = wordset_new();
    assert(wordset_size(&w) == 40);
    assert(wordset_size(&w) == wordset_capacity(&w));
}

// new() zero-initialises all bits; every bit reads back as false.
static void test_new_zero_bits(void) {
    printf("Test: new() zero-initialises all bits\n");
    bitset_t s = bitset_new();
    for (size_t i = 0; i < 20; i++) {
        assert(bitset_const_get(&s, i) != NULL);
        assert(*bitset_const_get(&s, i) == false);
        assert(*bitset_const_at(&s, i) == false);
    }
    assert(bitset_count(&s) == 0);
}

// init() produces the same zero state as new().
static void test_init_matches_new(void) {
    printf("Test: init() zero-initialises like new()\n");
    bitset_t s;
    bitset_init(&s);
    assert(bitset_size(&s) == 20);
    assert(bitset_count(&s) == 0);
    for (size_t i = 0; i < 20; i++) {
        assert(*bitset_const_at(&s, i) == false);
    }
}

// const_at / set_at write and read individual bits.
static void test_set_at_and_const_at(void) {
    printf("Test: set_at/const_at read and write individual bits\n");
    bitset_t s = bitset_new();
    bitset_set_at(&s, 0, true);
    bitset_set_at(&s, 7, true);
    bitset_set_at(&s, 19, true);
    assert(*bitset_const_at(&s, 0) == true);
    assert(*bitset_const_at(&s, 7) == true);
    assert(*bitset_const_at(&s, 19) == true);
    assert(*bitset_const_at(&s, 1) == false);
    assert(bitset_count(&s) == 3);

    // set_at(false) clears a bit.
    bitset_set_at(&s, 7, false);
    assert(*bitset_const_at(&s, 7) == false);
    assert(bitset_count(&s) == 2);
}

// Bounds-checked set() and flip() return false for index >= capacity.
static void test_set_and_flip_bounds_checked(void) {
    printf("Test: set() and flip() respect capacity as the upper bound\n");
    bitset_t s = bitset_new();

    // Valid indices succeed.
    assert(bitset_set(&s, 0, true));
    assert(bitset_set(&s, 19, true));
    assert(*bitset_const_at(&s, 0) == true);
    assert(*bitset_const_at(&s, 19) == true);

    // Index == capacity is out of bounds.
    assert(!bitset_set(&s, 20, true));
    assert(!bitset_flip(&s, 20));

    // Index far out of bounds.
    assert(!bitset_set(&s, 100, true));
    assert(!bitset_flip(&s, 100));

    // In-bounds flip toggles the bit.
    assert(bitset_flip(&s, 5));
    assert(*bitset_const_at(&s, 5) == true);
    assert(bitset_flip(&s, 5));
    assert(*bitset_const_at(&s, 5) == false);
}

// flip_at toggles a bit without bounds checking.
static void test_flip_at(void) {
    printf("Test: flip_at toggles a bit in-place\n");
    bitset_t s = bitset_new();
    assert(*bitset_const_at(&s, 3) == false);
    bitset_flip_at(&s, 3);
    assert(*bitset_const_at(&s, 3) == true);
    bitset_flip_at(&s, 3);
    assert(*bitset_const_at(&s, 3) == false);
}

// const_get returns NULL for index >= capacity.
static void test_const_get_bounds(void) {
    printf("Test: const_get returns NULL for out-of-bounds index\n");
    bitset_t s = bitset_new();
    assert(bitset_const_get(&s, 0) != NULL);
    assert(bitset_const_get(&s, 19) != NULL);
    assert(bitset_const_get(&s, 20) == NULL);
    assert(bitset_const_get(&s, 100) == NULL);
}

// set_all(true) fills all capacity bits; set_all(false) clears them.
static void test_set_all_and_count(void) {
    printf("Test: set_all and count operate over all capacity bits\n");
    bitset_t s = bitset_new();
    assert(bitset_count(&s) == 0);

    bitset_set_all(&s, true);
    assert(bitset_count(&s) == 20);
    for (size_t i = 0; i < 20; i++) assert(*bitset_const_at(&s, i) == true);

    bitset_set_all(&s, false);
    assert(bitset_count(&s) == 0);
    for (size_t i = 0; i < 20; i++) assert(*bitset_const_at(&s, i) == false);
}

// set_all(true) with a partial trailing block must not set dead high bits.
static void test_set_all_canonical_zero(void) {
    printf("Test: set_all(true) keeps dead high bits zero (canonical-zero invariant)\n");
    // byteset: 8-bit blocks, 20 bits → two full bytes + one 4-bit partial byte.
    byteset_t b = byteset_new();
    byteset_set_all(&b, true);
    assert(byteset_count(&b) == 20);

    const uint8_t *blocks = byteset_const_blocks(&b);
    assert(blocks[0] == 0xFFu);
    assert(blocks[1] == 0xFFu);
    assert(blocks[2] == 0x0Fu);  // only low 4 bits live

    byteset_set_all(&b, false);
    assert(blocks[0] == 0x00u && blocks[1] == 0x00u && blocks[2] == 0x00u);

    // wordset: 32-bit blocks, 40 bits → one full block + 8-bit partial block.
    wordset_t w = wordset_new();
    wordset_set_all(&w, true);
    assert(wordset_count(&w) == 40);

    const uint32_t *wb = wordset_const_blocks(&w);
    assert(wb[0] == 0xFFFFFFFFu);
    assert(wb[1] == 0x000000FFu);  // only low 8 bits live

    wordset_set_all(&w, false);
    assert(wb[0] == 0u && wb[1] == 0u);
}

// flip_all inverts every live bit and leaves dead high bits at zero.
static void test_flip_all(void) {
    printf("Test: flip_all inverts all live bits, preserves canonical-zero\n");
    byteset_t b = byteset_new();
    // Set every third bit.
    for (size_t i = 0; i < 20; i += 3) byteset_set_at(&b, i, true);
    size_t before = byteset_count(&b);

    byteset_flip_all(&b);
    assert(byteset_count(&b) == 20 - before);

    // All bits are now inverted.
    for (size_t i = 0; i < 20; i++) {
        bool expected = (i % 3) != 0;
        assert(*byteset_const_at(&b, i) == expected);
    }

    // Dead high bits of the trailing byte must still be zero.
    const uint8_t *blocks = byteset_const_blocks(&b);
    assert((blocks[2] & 0xF0u) == 0);

    // Double flip restores original state.
    byteset_flip_all(&b);
    for (size_t i = 0; i < 20; i++) {
        bool expected = (i % 3) == 0;
        assert(*byteset_const_at(&b, i) == expected);
    }
}

// eq returns true iff two bitsets have the same bits.
// (Two bitsets of the same type always have the same capacity, so only bits differ.)
static void test_eq(void) {
    printf("Test: eq compares two bitsets for equality\n");
    bitset_t a = bitset_new();
    bitset_t b = bitset_new();

    // Both zero → equal.
    assert(bitset_eq(&a, &b));

    // Set one bit in a.
    bitset_set_at(&a, 5, true);
    assert(!bitset_eq(&a, &b));

    // Mirror the change in b.
    bitset_set_at(&b, 5, true);
    assert(bitset_eq(&a, &b));

    // Differ in last bit.
    bitset_set_at(&a, 19, true);
    assert(!bitset_eq(&a, &b));
    bitset_set_at(&b, 19, true);
    assert(bitset_eq(&a, &b));

    // fill all → still equal.
    bitset_set_all(&a, true);
    bitset_set_all(&b, true);
    assert(bitset_eq(&a, &b));

    // clear one → unequal.
    bitset_set_at(&a, 0, false);
    assert(!bitset_eq(&a, &b));
}

// all() and any() reflect whether all / any live bits are set.
static void test_all_and_any(void) {
    printf("Test: all() and any() reflect bitset state\n");
    bitset_t s = bitset_new();

    // All-zero: all() false, any() false.
    assert(!bitset_all(&s));
    assert(!bitset_any(&s));

    // One bit set: all() false, any() true.
    bitset_set_at(&s, 10, true);
    assert(!bitset_all(&s));
    assert(bitset_any(&s));

    // All bits set: both true.
    bitset_set_all(&s, true);
    assert(bitset_all(&s));
    assert(bitset_any(&s));

    // Clear one bit: all() false, any() still true.
    bitset_set_at(&s, 0, false);
    assert(!bitset_all(&s));
    assert(bitset_any(&s));

    // Clear all: any() false.
    bitset_set_all(&s, false);
    assert(!bitset_any(&s));

    // Verify with wordset (40-bit, two 32-bit blocks).
    wordset_t w = wordset_new();
    assert(!wordset_all(&w));
    assert(!wordset_any(&w));
    wordset_set_all(&w, true);
    assert(wordset_all(&w));
    assert(wordset_any(&w));
    wordset_set_at(&w, 39, false);
    assert(!wordset_all(&w));
    assert(wordset_any(&w));
}

// copy duplicates the bit pattern; size remains capacity in the copy.
static void test_copy(void) {
    printf("Test: copy duplicates bit pattern and preserves size == capacity\n");
    bitset_t src = bitset_new();
    for (size_t i = 0; i < 20; i += 2) bitset_set_at(&src, i, true);

    bitset_t dst;
    bitset_copy(&dst, &src);

    assert(bitset_size(&dst) == 20);
    assert(bitset_eq(&src, &dst));
    for (size_t i = 0; i < 20; i++) {
        assert(*bitset_const_at(&dst, i) == ((i % 2) == 0));
    }

    // Modifying dst does not affect src.
    bitset_set_at(&dst, 0, false);
    assert(*bitset_const_at(&src, 0) == true);

    // Self-copy is a no-op.
    bitset_copy(&src, &src);
    assert(bitset_count(&src) == 10);
}

// destroy clears all bits but does NOT reset size (there is no _size field).
static void test_destroy(void) {
    printf("Test: destroy clears bits; size is still capacity afterwards\n");
    bitset_t s = bitset_new();
    bitset_set_all(&s, true);
    assert(bitset_count(&s) == 20);

    bitset_destroy(&s);

    // All bits cleared.
    assert(bitset_count(&s) == 0);
    for (size_t i = 0; i < 20; i++) assert(*bitset_const_at(&s, i) == false);

    // Size is still capacity (the defining property of a bitset).
    assert(bitset_size(&s) == 20);
    assert(bitset_size(&s) == bitset_capacity(&s));
}

// block accessors expose the correct storage layout.
static void test_blocks_layout(void) {
    printf("Test: block accessors expose LSB-first packed storage\n");
    // Use uint8_t-backed instance for byte-granular assertions.
    byteset_t b = byteset_new();
    assert(byteset_block_bits(&b) == 8);
    assert(byteset_block_count(&b) == 3);  // ceil(20 / 8)

    // Set bits 0 and 9 → byte0 bit0, byte1 bit1.
    byteset_set_at(&b, 0, true);
    byteset_set_at(&b, 9, true);
    const uint8_t *cb = byteset_const_blocks(&b);
    assert((cb[0] & 0x01u) != 0);  // bit 0
    assert((cb[1] & 0x02u) != 0);  // bit 9 = block1 bit1

    // Mutable and const accessors point to the same storage.
    assert(byteset_blocks(&b) == byteset_const_blocks(&b));

    // wordset: 32-bit blocks, 40 bits → 2 blocks.
    wordset_t w = wordset_new();
    assert(wordset_block_bits(&w) == 32);
    assert(wordset_block_count(&w) == 2);

    // Bit 35 → block1, bit position 3.
    wordset_set_at(&w, 35, true);
    const uint32_t *wb = wordset_const_blocks(&w);
    assert((wb[1] & 0x00000008u) != 0);
    assert(*wordset_const_at(&w, 35) == true);

    // No _size field: struct is exactly NBLOCKS * sizeof(block_t).
    assert(sizeof(byteset_t) == 3 * sizeof(uint8_t));
    assert(sizeof(wordset_t) == 2 * sizeof(uint32_t));
}

// begin/end/iter_next iterate over all capacity bits (0..capacity-1).
static void test_full_iteration(void) {
    printf("Test: begin/end/iter_next iterate over all capacity bits\n");
    bitset_t s = bitset_new();
    for (size_t i = 0; i < 20; i += 3) bitset_set_at(&s, i, true);

    size_t visited = 0, ones = 0;
    for (bitset_iter_t it = bitset_begin(&s); it != bitset_end(&s); it = bitset_iter_next(&s, it)) {
        ones += *bitset_const_at(&s, it) ? 1u : 0u;
        visited++;
    }
    assert(visited == 20);
    assert(ones == bitset_count(&s));

    // begin() is always 0; end() is always capacity.
    assert(bitset_begin(&s) == 0);
    assert(bitset_end(&s) == 20);
}

// begin_true/iter_next_true visit only set bits, whole zero blocks are skipped.
static void test_set_bit_iteration(void) {
    printf("Test: begin_true/iter_next_true visit only set bits\n");
    // wordset: 32-bit blocks, 40 bits.  Set bits that straddle the block boundary.
    wordset_t w = wordset_new();
    const size_t set_idx[] = {0, 31, 32, 39};
    for (size_t k = 0; k < 4; k++) wordset_set_at(&w, set_idx[k], true);

    size_t k = 0;
    for (wordset_iter_t it = wordset_begin_true(&w); it != wordset_end(&w); it = wordset_iter_next_true(&w, it)) {
        assert(k < 4);
        assert(it == set_idx[k]);
        assert(*wordset_const_at(&w, it) == true);
        k++;
    }
    assert(k == 4);

    // All-zero: begin_true() == end().
    wordset_t zeros = wordset_new();
    assert(wordset_begin_true(&zeros) == wordset_end(&zeros));

    // All-ones: every index is visited.
    byteset_t ones = byteset_new();
    byteset_set_all(&ones, true);
    size_t expected = 0;
    for (byteset_iter_t it = byteset_begin_true(&ones); it != byteset_end(&ones);
         it = byteset_iter_next_true(&ones, it)) {
        assert(it == expected);
        expected++;
    }
    assert(expected == 20);
}

// _ZP_CONST_FOREACH works correctly with a bitset.
static void test_const_foreach(void) {
    printf("Test: _ZP_CONST_FOREACH visits every bit of a bitset\n");
    wordset_t w = wordset_new();
    for (size_t i = 0; i < 40; i += 4) wordset_set_at(&w, i, true);

    size_t idx = 0, ones = 0;
    const bool *elem = NULL;
    _ZP_CONST_FOREACH (wordset, &w, elem) {
        assert(*elem == ((idx % 4) == 0));
        ones += *elem ? 1u : 0u;
        idx++;
    }
    assert(idx == 40);
    assert(ones == wordset_count(&w));
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main(void) {
    test_size_is_capacity();
    test_new_zero_bits();
    test_init_matches_new();
    test_set_at_and_const_at();
    test_set_and_flip_bounds_checked();
    test_flip_at();
    test_const_get_bounds();
    test_set_all_and_count();
    test_set_all_canonical_zero();
    test_flip_all();
    test_eq();
    test_all_and_any();
    test_copy();
    test_destroy();
    test_blocks_layout();
    test_full_iteration();
    test_set_bit_iteration();
    test_const_foreach();
    printf("All bitset tests passed.\n");
    return 0;
}
