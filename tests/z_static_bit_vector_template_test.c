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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#undef NDEBUG
#include <assert.h>

#include "zenoh-pico/collections/algorithms_template.h"

// ── Instantiate a bit vector, capacity 20 (spans more than one storage byte) ──

#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME bitvec
#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_SIZE 20
#include "zenoh-pico/collections/static_bit_vector_template.h"

// ── Instantiate a second bit vector with the default name to check derivation ─

#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_SIZE 8
#include "zenoh-pico/collections/static_bit_vector_template.h"

// ── Instantiate a uint8_t-backed bit vector to exercise byte-granular storage ─

#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME bytevec
#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_SIZE 20
#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_TYPE uint8_t
#include "zenoh-pico/collections/static_bit_vector_template.h"

// ── Instantiate a uint32_t-backed bit vector to exercise wide storage blocks ──

#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME wordvec
#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_SIZE 40
#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_TYPE uint32_t
#include "zenoh-pico/collections/static_bit_vector_template.h"

// ── Tests ─────────────────────────────────────────────────────────────────────

static void test_new_is_empty(void) {
    printf("Test: new bit vector is empty\n");
    bitvec_t v = bitvec_new();
    assert(bitvec_is_empty(&v));
    assert(bitvec_size(&v) == 0);
    assert(bitvec_capacity(&v) == 20);
    bool out = true;
    assert(!bitvec_front(&v, &out));
    assert(!bitvec_back(&v, &out));
    assert(!bitvec_get(&v, 0, &out));
    bitvec_destroy(&v);
}

static void test_init_matches_new(void) {
    printf("Test: init zero-initialises like new\n");
    bitvec_t v;
    bitvec_init(&v);
    assert(bitvec_is_empty(&v));
    assert(bitvec_size(&v) == 0);
    bitvec_destroy(&v);
}

static void test_push_back_and_at(void) {
    printf("Test: push_back appends bits in order\n");
    bitvec_t v = bitvec_new();
    bool vals[] = {true, false, true, true, false};
    for (size_t i = 0; i < 5; i++) {
        assert(bitvec_push_back(&v, vals[i]));
    }
    assert(bitvec_size(&v) == 5);
    for (size_t i = 0; i < 5; i++) {
        assert(bitvec_at(&v, i) == vals[i]);
        assert(bitvec_const_at(&v, i) == vals[i]);
        bool out = !vals[i];
        assert(bitvec_get(&v, i, &out));
        assert(out == vals[i]);
    }
    bitvec_destroy(&v);
}

static void test_set_flip(void) {
    printf("Test: set/set_at/flip/flip_at update individual bits\n");
    bitvec_t v = bitvec_new();
    for (size_t i = 0; i < 10; i++) {
        assert(bitvec_push_back(&v, false));
    }
    assert(bitvec_set(&v, 3, true));
    assert(bitvec_at(&v, 3) == true);
    bitvec_set_at(&v, 3, false);
    assert(bitvec_at(&v, 3) == false);

    assert(bitvec_flip(&v, 7));
    assert(bitvec_at(&v, 7) == true);
    bitvec_flip_at(&v, 7);
    assert(bitvec_at(&v, 7) == false);

    // Out of bounds variants return false and do not modify the vector.
    assert(!bitvec_set(&v, 10, true));
    assert(!bitvec_flip(&v, 10));
    bitvec_destroy(&v);
}

static void test_pop_back(void) {
    printf("Test: pop_back removes from the end (LIFO order)\n");
    bitvec_t v = bitvec_new();
    bool vals[] = {true, false, true};
    for (size_t i = 0; i < 3; i++) {
        assert(bitvec_push_back(&v, vals[i]));
    }
    for (size_t i = 3; i-- > 0;) {
        bool out = !vals[i];
        assert(bitvec_pop_back(&v, &out));
        assert(out == vals[i]);
    }
    assert(bitvec_is_empty(&v));
    // pop_back on empty returns false.
    bool out = false;
    assert(!bitvec_pop_back(&v, &out));
    bitvec_destroy(&v);
}

static void test_front_back_peek(void) {
    printf("Test: front and back return correct values without removing\n");
    bitvec_t v = bitvec_new();
    assert(bitvec_push_back(&v, true));
    assert(bitvec_push_back(&v, false));
    assert(bitvec_push_back(&v, false));
    bool f = false, b = true;
    assert(bitvec_front(&v, &f) && f == true);
    assert(bitvec_back(&v, &b) && b == false);
    assert(bitvec_size(&v) == 3);  // unchanged
    bitvec_destroy(&v);
}

static void test_capacity_full(void) {
    printf("Test: push_back fails when the vector is full\n");
    bitvec_t v = bitvec_new();
    for (size_t i = 0; i < 20; i++) {
        assert(bitvec_push_back(&v, (i % 3) == 0));
    }
    assert(bitvec_size(&v) == 20);
    assert(!bitvec_push_back(&v, true));
    // Contents preserved.
    for (size_t i = 0; i < 20; i++) {
        assert(bitvec_at(&v, i) == ((i % 3) == 0));
    }
    bitvec_destroy(&v);
}

static void test_append(void) {
    printf("Test: append copies a range of bits to the back\n");
    bitvec_t v = bitvec_new();
    bool a[] = {true, true, false};
    bool b[] = {false, true};
    assert(bitvec_append(&v, a, 3));
    assert(bitvec_append(&v, b, 2));
    assert(bitvec_size(&v) == 5);
    bool expected[] = {true, true, false, false, true};
    for (size_t i = 0; i < 5; i++) {
        assert(bitvec_at(&v, i) == expected[i]);
    }
    bitvec_destroy(&v);
}

static void test_append_empty_and_overflow(void) {
    printf("Test: append handles empty range and rejects overflow\n");
    bitvec_t v = bitvec_new();
    assert(bitvec_append(&v, NULL, 0));  // empty range always succeeds
    assert(bitvec_size(&v) == 0);

    bool big[21];
    for (size_t i = 0; i < 21; i++) big[i] = true;
    assert(!bitvec_append(&v, big, 21));  // exceeds capacity 20
    assert(bitvec_size(&v) == 0);         // unchanged

    assert(bitvec_append(&v, big, 20));  // exactly fills
    assert(bitvec_size(&v) == 20);
    assert(!bitvec_append(&v, big, 1));  // no room left
    bitvec_destroy(&v);
}

static void test_insert(void) {
    printf("Test: insert places a bit and shifts the tail right\n");
    bitvec_t v = bitvec_new();
    bool init[] = {true, false, true};
    assert(bitvec_append(&v, init, 3));  // [1, 0, 1]

    assert(bitvec_insert(&v, 1, true));  // [1, 1, 0, 1]
    bool e1[] = {true, true, false, true};
    assert(bitvec_size(&v) == 4);
    for (size_t i = 0; i < 4; i++) assert(bitvec_at(&v, i) == e1[i]);

    assert(bitvec_insert(&v, 0, false));  // front: [0, 1, 1, 0, 1]
    assert(bitvec_at(&v, 0) == false);

    assert(bitvec_insert(&v, bitvec_size(&v), true));  // back
    assert(bitvec_at(&v, bitvec_size(&v) - 1) == true);

    // Out of bounds index.
    assert(!bitvec_insert(&v, bitvec_size(&v) + 1, true));
    bitvec_destroy(&v);
}

static void test_insert_full(void) {
    printf("Test: insert fails when full\n");
    bitvec_t v = bitvec_new();
    for (size_t i = 0; i < 20; i++) assert(bitvec_push_back(&v, false));
    assert(!bitvec_insert(&v, 0, true));
    bitvec_destroy(&v);
}

static void test_remove(void) {
    printf("Test: remove deletes a bit and shifts the tail left\n");
    bitvec_t v = bitvec_new();
    bool init[] = {true, false, true, true, false};
    assert(bitvec_append(&v, init, 5));

    bool out = false;
    assert(bitvec_remove(&v, 1, &out));  // remove the 0 -> [1, 1, 1, 0]
    assert(out == false);
    assert(bitvec_size(&v) == 4);
    bool expected[] = {true, true, true, false};
    for (size_t i = 0; i < 4; i++) assert(bitvec_at(&v, i) == expected[i]);

    // remove with NULL out and out-of-bounds.
    assert(bitvec_remove(&v, 3, NULL));  // remove last
    assert(bitvec_size(&v) == 3);
    assert(!bitvec_remove(&v, 3, NULL));
    bitvec_destroy(&v);
}

static void test_swap_remove(void) {
    printf("Test: swap_remove is O(1) and does not preserve order\n");
    bitvec_t v = bitvec_new();
    bool init[] = {true, false, false, false, true};  // last is 1
    assert(bitvec_append(&v, init, 5));

    bool out = true;
    assert(bitvec_swap_remove(&v, 1, &out));  // removes index 1 (0), moves last (1) into it
    assert(out == false);
    assert(bitvec_size(&v) == 4);
    // Expected: [1, 1, 0, 0]
    bool expected[] = {true, true, false, false};
    for (size_t i = 0; i < 4; i++) assert(bitvec_at(&v, i) == expected[i]);

    // Removing the last element does not move anything.
    assert(bitvec_swap_remove(&v, 3, NULL));
    assert(bitvec_size(&v) == 3);

    assert(!bitvec_swap_remove(&v, 3, NULL));  // out of bounds
    bitvec_destroy(&v);
}

static void test_remove_at(void) {
    printf("Test: remove_at mirrors remove and reports next index\n");
    bitvec_t v = bitvec_new();
    bool init[] = {true, false, true};
    assert(bitvec_append(&v, init, 3));

    bitvec_iter_t next = 99;
    bool out = false;
    bitvec_remove_at(&v, 0, &out, &next);  // [0, 1]
    assert(out == true);
    assert(next == 0);
    assert(bitvec_size(&v) == 2);

    // next_idx may be NULL.
    bitvec_remove_at(&v, 1, NULL, NULL);  // [0]
    assert(bitvec_size(&v) == 1);
    assert(bitvec_at(&v, 0) == false);
    bitvec_destroy(&v);
}

static void test_set_all_and_count(void) {
    printf("Test: set_all and count operate over the live bits\n");
    bitvec_t v = bitvec_new();
    for (size_t i = 0; i < 13; i++) assert(bitvec_push_back(&v, (i % 2) == 0));
    assert(bitvec_count(&v) == 7);  // indices 0,2,4,6,8,10,12

    bitvec_set_all(&v, true);
    assert(bitvec_count(&v) == 13);
    bitvec_set_all(&v, false);
    assert(bitvec_count(&v) == 0);
    bitvec_destroy(&v);
}

static void test_set_all_count_block_based(void) {
    printf("Test: block-based set_all/count span full and partial blocks, keep dead bits zero\n");
    // bytevec uses 8-bit blocks; 20 bits => two full blocks + a 4-bit partial block.
    bytevec_t v = bytevec_new();
    for (size_t i = 0; i < 20; i++) assert(bytevec_push_back(&v, false));

    bytevec_set_all(&v, true);
    assert(bytevec_count(&v) == 20);
    // Every live bit reads back as 1...
    for (size_t i = 0; i < 20; i++) assert(bytevec_at(&v, i) == true);
    // ...and the dead high bits of the trailing partial block stay zero (low 4 bits set => 0x0F).
    const uint8_t *blocks = bytevec_const_blocks(&v);
    assert(blocks[0] == 0xFFu);
    assert(blocks[1] == 0xFFu);
    assert(blocks[2] == 0x0Fu);

    bytevec_set_all(&v, false);
    assert(bytevec_count(&v) == 0);
    assert(blocks[0] == 0x00u && blocks[1] == 0x00u && blocks[2] == 0x00u);
    bytevec_destroy(&v);

    // Exercise the all-full-blocks path (no partial block): 16 bits over 8-bit blocks.
    bytevec_t full = bytevec_new();
    for (size_t i = 0; i < 16; i++) assert(bytevec_push_back(&full, false));
    bytevec_set_all(&full, true);
    assert(bytevec_count(&full) == 16);
    assert(bytevec_const_blocks(&full)[0] == 0xFFu && bytevec_const_blocks(&full)[1] == 0xFFu);
    bytevec_destroy(&full);

    // Wide (32-bit) blocks: 40 bits => one full block + an 8-bit partial block.
    wordvec_t w = wordvec_new();
    for (size_t i = 0; i < 40; i++) assert(wordvec_push_back(&w, false));
    wordvec_set_all(&w, true);
    assert(wordvec_count(&w) == 40);
    const uint32_t *wb = wordvec_const_blocks(&w);
    assert(wb[0] == 0xFFFFFFFFu);
    assert(wb[1] == 0x000000FFu);  // only low 8 bits live
    wordvec_destroy(&w);
}

static void test_blocks_layout(void) {
    printf("Test: raw block accessors expose LSB-first packed storage\n");
    // Use the uint8_t-backed instance for deterministic byte-granular layout assertions.
    bytevec_t v = bytevec_new();
    assert(bytevec_block_bits(&v) == 8);
    assert(bytevec_block_count(&v) == 3);  // ceil(20 / 8)
    // Set bits 0 and 9 -> byte0 bit0, byte1 bit1.
    for (size_t i = 0; i < 10; i++) assert(bytevec_push_back(&v, false));
    bytevec_set_at(&v, 0, true);
    bytevec_set_at(&v, 9, true);
    const uint8_t *blocks = bytevec_const_blocks(&v);
    assert((blocks[0] & 0x01u) != 0);
    assert((blocks[1] & 0x02u) != 0);
    // Mutable accessor returns the same storage.
    assert(bytevec_blocks(&v) == v._blocks);
    bytevec_destroy(&v);
}

static void test_block_type_layout(void) {
    printf("Test: block type controls storage width and packing\n");
    // Default instance is size_t-backed: a single block holds at least 16 bits, so 20 bits fit
    // in ceil(20 / (8 * sizeof(size_t))) blocks.
    bitvec_t d = bitvec_new();
    assert(bitvec_block_bits(&d) == sizeof(size_t) * 8);
    size_t expected_default = (20 + bitvec_block_bits(&d) - 1) / bitvec_block_bits(&d);
    assert(bitvec_block_count(&d) == expected_default);
    bitvec_destroy(&d);

    // uint32_t-backed instance: 32 bits per block, ceil(40 / 32) == 2 blocks.
    wordvec_t w = wordvec_new();
    assert(wordvec_block_bits(&w) == 32);
    assert(wordvec_block_count(&w) == 2);
    for (size_t i = 0; i < 40; i++) assert(wordvec_push_back(&w, false));
    // Bit 0 -> block0 bit0; bit 35 -> block1 bit3.
    wordvec_set_at(&w, 0, true);
    wordvec_set_at(&w, 35, true);
    const uint32_t *wb = wordvec_const_blocks(&w);
    assert((wb[0] & 0x00000001u) != 0);
    assert((wb[1] & 0x00000008u) != 0);
    // The value round-trips through the bit accessors regardless of block width.
    assert(wordvec_at(&w, 0) == true);
    assert(wordvec_at(&w, 35) == true);
    assert(wordvec_count(&w) == 2);
    wordvec_destroy(&w);
}

static void test_iteration(void) {
    printf("Test: begin/end/iter_next provide index iteration\n");
    bitvec_t v = bitvec_new();
    bool init[] = {true, false, true, true};
    assert(bitvec_append(&v, init, 4));
    size_t ones = 0, visited = 0;
    for (bitvec_iter_t it = bitvec_begin(&v); it != bitvec_end(&v); it = bitvec_iter_next(&v, it)) {
        if (bitvec_at(&v, it)) ones++;
        visited++;
    }
    assert(visited == 4);
    assert(ones == 3);
    bitvec_destroy(&v);
}

static void test_foreach(void) {
    printf("Test: _ZP_FOREACH visits every bit by value in order\n");
    bitvec_t v = bitvec_new();
    bool init[] = {true, false, true, true, false, true};
    assert(bitvec_append(&v, init, 6));

    // _ZP_FOREACH binds `elem` to the value returned by at() (a bool), not a pointer, because a
    // single bit is not addressable. Iterate and check order, total, and set-bit count.
    size_t idx = 0, ones = 0;
    bool elem = false;
    _ZP_FOREACH (bitvec, &v, elem) {
        assert(elem == init[idx]);
        ones += elem ? 1u : 0u;
        idx++;
    }
    assert(idx == 6);
    assert(ones == bitvec_count(&v));

    // Iterating an empty bit vector executes the body zero times.
    bitvec_t empty = bitvec_new();
    size_t empty_visits = 0;
    _ZP_FOREACH (bitvec, &empty, elem) {
        (void)elem;
        empty_visits++;
    }
    assert(empty_visits == 0);

    bitvec_destroy(&empty);
    bitvec_destroy(&v);
}

static void test_const_foreach(void) {
    printf("Test: _ZP_CONST_FOREACH visits every bit by value via const access\n");
    // Use a wide-block instance so iteration crosses a block boundary (40 bits over 32-bit blocks).
    wordvec_t v = wordvec_new();
    for (size_t i = 0; i < 40; i++) assert(wordvec_push_back(&v, (i % 3) == 0));

    size_t idx = 0, ones = 0;
    bool elem = false;
    _ZP_CONST_FOREACH (wordvec, &v, elem) {
        assert(elem == ((idx % 3) == 0));
        ones += elem ? 1u : 0u;
        idx++;
    }
    assert(idx == 40);
    assert(ones == wordvec_count(&v));
    wordvec_destroy(&v);
}

static void test_set_bit_iteration(void) {
    printf("Test: begin_true/iter_next_true/end visit only set bits\n");
    // wordvec uses 32-bit blocks; capacity 40 spans two blocks. Set bits 1, 31, 32 and 39 so the
    // scan crosses the block boundary and exercises the high/low ends of both blocks.
    wordvec_t v = wordvec_new();
    for (size_t i = 0; i < 40; i++) assert(wordvec_push_back(&v, false));
    const size_t set_idx[] = {1, 31, 32, 39};
    for (size_t k = 0; k < 4; k++) wordvec_set_at(&v, set_idx[k], true);

    size_t k = 0;
    for (wordvec_iter_t it = wordvec_begin_true(&v); it != wordvec_end(&v); it = wordvec_iter_next_true(&v, it)) {
        assert(k < 4);
        assert(it == set_idx[k]);
        assert(wordvec_at(&v, it) == true);
        k++;
    }
    assert(k == 4);  // visited exactly the set bits, in order
    wordvec_destroy(&v);

    // All-zero (and empty) vectors: begin_true() == end(), so the loop body never runs.
    wordvec_t zeros = wordvec_new();
    for (size_t i = 0; i < 40; i++) assert(wordvec_push_back(&zeros, false));
    assert(wordvec_begin_true(&zeros) == wordvec_end(&zeros));
    wordvec_destroy(&zeros);

    wordvec_t empty = wordvec_new();
    assert(wordvec_begin_true(&empty) == wordvec_end(&empty));
    wordvec_destroy(&empty);

    // All-ones vector: set-bit iteration visits every index 0..size-1.
    bytevec_t ones = bytevec_new();
    for (size_t i = 0; i < 20; i++) assert(bytevec_push_back(&ones, true));
    size_t expected = 0;
    for (bytevec_iter_t it = bytevec_begin_true(&ones); it != bytevec_end(&ones);
         it = bytevec_iter_next_true(&ones, it)) {
        assert(it == expected);
        expected++;
    }
    assert(expected == 20);
    bytevec_destroy(&ones);
}

static void test_default_named_instance(void) {
    printf("Test: default-named instance (bitvec_8) works independently\n");
    bitvec_8_t v = bitvec_8_new();
    assert(bitvec_8_capacity(&v) == 8);
    for (size_t i = 0; i < 8; i++) assert(bitvec_8_push_back(&v, true));
    assert(!bitvec_8_push_back(&v, true));  // full
    assert(bitvec_8_count(&v) == 8);
    bitvec_8_destroy(&v);
}

static void test_wide_block_across_boundary(void) {
    printf("Test: insert/remove work across block boundaries with wide blocks\n");
    // wordvec uses 32-bit blocks; capacity 40 spans two blocks.
    wordvec_t v = wordvec_new();
    // Fill with an alternating pattern up to capacity.
    for (size_t i = 0; i < 40; i++) assert(wordvec_push_back(&v, (i % 2) == 0));
    assert(wordvec_size(&v) == 40);
    assert(wordvec_count(&v) == 20);

    // Removing a bit at index 0 shifts everything (including across the 32-bit boundary) left.
    bool out = false;
    assert(wordvec_remove(&v, 0, &out));
    assert(out == true);
    assert(wordvec_size(&v) == 39);
    // Pattern is now inverted relative to its original alignment.
    for (size_t i = 0; i < 39; i++) assert(wordvec_at(&v, i) == ((i % 2) == 1));

    // Re-inserting at the front restores the original pattern and refills capacity.
    assert(wordvec_insert(&v, 0, true));
    assert(wordvec_size(&v) == 40);
    for (size_t i = 0; i < 40; i++) assert(wordvec_at(&v, i) == ((i % 2) == 0));
    wordvec_destroy(&v);
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main(void) {
    test_new_is_empty();
    test_init_matches_new();
    test_push_back_and_at();
    test_set_flip();
    test_pop_back();
    test_front_back_peek();
    test_capacity_full();
    test_append();
    test_append_empty_and_overflow();
    test_insert();
    test_insert_full();
    test_remove();
    test_swap_remove();
    test_remove_at();
    test_set_all_and_count();
    test_set_all_count_block_based();
    test_blocks_layout();
    test_block_type_layout();
    test_iteration();
    test_foreach();
    test_const_foreach();
    test_set_bit_iteration();
    test_default_named_instance();
    test_wide_block_across_boundary();
    printf("All bit vector tests passed.\n");
    return 0;
}
