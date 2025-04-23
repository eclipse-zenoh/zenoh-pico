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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico/collections/lru_cache.h"
#include "zenoh-pico/system/common/platform.h"

#undef NDEBUG
#include <assert.h>

#define CACHE_CAPACITY 10

typedef struct _dummy_t {
    int foo;
} _dummy_t;

int _dummy_compare(const void *first, const void *second) {
    _dummy_t *d_first = (_dummy_t *)first;
    _dummy_t *d_second = (_dummy_t *)second;

    if (d_first->foo == d_second->foo) {
        return 0;
    } else if (d_first->foo > d_second->foo) {
        return 1;
    }
    return -1;
}

static inline void _dummy_elem_clear(void *e) { _z_noop_clear((_dummy_t *)e); }

_Z_LRU_CACHE_DEFINE(_dummy, _dummy_t, _dummy_compare)

void test_lru_init(void) {
    _dummy_lru_cache_t dcache = _dummy_lru_cache_init(CACHE_CAPACITY);
    assert(dcache.capacity == CACHE_CAPACITY);
    assert(dcache.len == 0);
    assert(dcache.head == NULL);
    assert(dcache.tail == NULL);
    assert(dcache.slist == NULL);
}

void test_lru_cache_insert(void) {
    _dummy_lru_cache_t dcache = _dummy_lru_cache_init(CACHE_CAPACITY);

    _dummy_t v0 = {0};
    assert(dcache.slist == NULL);
    assert(_dummy_lru_cache_get(&dcache, &v0) == NULL);
    assert(_dummy_lru_cache_insert(&dcache, &v0) == 0);
    assert(dcache.slist != NULL);
    _dummy_t *res = _dummy_lru_cache_get(&dcache, &v0);
    assert(res != NULL);
    assert(res->foo == v0.foo);

    _dummy_t data[CACHE_CAPACITY] = {0};
    for (size_t i = 1; i < CACHE_CAPACITY; i++) {
        data[i].foo = (int)i;
        assert(_dummy_lru_cache_insert(&dcache, &data[i]) == 0);
    }
    for (size_t i = 0; i < CACHE_CAPACITY; i++) {
        res = _dummy_lru_cache_get(&dcache, &data[i]);
        assert(res != NULL);
        assert(res->foo == data[i].foo);
    }
    _dummy_lru_cache_delete(&dcache);
}

void test_lru_cache_clear(void) {
    _dummy_lru_cache_t dcache = _dummy_lru_cache_init(CACHE_CAPACITY);

    _dummy_t data[CACHE_CAPACITY] = {0};
    for (size_t i = 0; i < CACHE_CAPACITY; i++) {
        data[i].foo = (int)i;
        assert(_dummy_lru_cache_insert(&dcache, &data[i]) == 0);
    }
    _dummy_lru_cache_clear(&dcache);
    assert(dcache.capacity == CACHE_CAPACITY);
    assert(dcache.len == 0);
    assert(dcache.slist != NULL);
    assert(dcache.head == NULL);
    assert(dcache.tail == NULL);
    for (size_t i = 0; i < CACHE_CAPACITY; i++) {
        assert(_dummy_lru_cache_get(&dcache, &data[i]) == NULL);
    }
    _dummy_lru_cache_delete(&dcache);
}

void test_lru_cache_deletion(void) {
    _dummy_lru_cache_t dcache = _dummy_lru_cache_init(CACHE_CAPACITY);

    _dummy_t data[CACHE_CAPACITY + 1] = {0};
    for (size_t i = 0; i < CACHE_CAPACITY + 1; i++) {
        data[i].foo = (int)i;
        assert(_dummy_lru_cache_insert(&dcache, &data[i]) == 0);
    }
    // Check value deleted
    assert(_dummy_lru_cache_get(&dcache, &data[0]) == NULL);
    // Check remaining value
    for (size_t i = 1; i < CACHE_CAPACITY + 1; i++) {
        _dummy_t *res = _dummy_lru_cache_get(&dcache, &data[i]);
        assert(res != NULL);
        assert(res->foo == data[i].foo);
    }
    _dummy_lru_cache_delete(&dcache);
}

void test_lru_cache_update(void) {
    _dummy_lru_cache_t dcache = _dummy_lru_cache_init(CACHE_CAPACITY);

    _dummy_t data[CACHE_CAPACITY] = {0};
    for (size_t i = 0; i < CACHE_CAPACITY; i++) {
        data[i].foo = (int)i;
        assert(_dummy_lru_cache_insert(&dcache, &data[i]) == 0);
    }
    // Update value
    assert(_dummy_lru_cache_get(&dcache, &data[0]) != NULL);
    // Insert extra value
    _dummy_t extra_data = {55};
    assert(_dummy_lru_cache_insert(&dcache, &extra_data) == 0);
    // Check value deleted
    assert(_dummy_lru_cache_get(&dcache, &data[1]) == NULL);
    _dummy_lru_cache_delete(&dcache);
}

static bool val_in_array(int val, int *array, size_t array_size) {
    for (size_t i = 0; i < array_size; i++) {
        if (val == array[i]) {
            return true;
        }
    }
    return false;
}

void test_lru_cache_random_val(void) {
    _dummy_lru_cache_t dcache = _dummy_lru_cache_init(10 * CACHE_CAPACITY);

    _dummy_t data[11 * CACHE_CAPACITY] = {
        {1804289383}, {846930886},  {1681692777}, {1714636915}, {1957747793}, {424238335},  {719885386},  {1649760492},
        {596516649},  {1189641421}, {1025202362}, {1350490027}, {783368690},  {1102520059}, {2044897763}, {1967513926},
        {1365180540}, {1540383426}, {304089172},  {1303455736}, {35005211},   {521595368},  {294702567},  {1726956429},
        {336465782},  {861021530},  {278722862},  {233665123},  {2145174067}, {468703135},  {1101513929}, {1801979802},
        {1315634022}, {635723058},  {1369133069}, {1125898167}, {1059961393}, {2089018456}, {628175011},  {1656478042},
        {1131176229}, {1653377373}, {859484421},  {1914544919}, {608413784},  {756898537},  {1734575198}, {1973594324},
        {149798315},  {2038664370}, {1129566413}, {184803526},  {412776091},  {1424268980}, {1911759956}, {749241873},
        {137806862},  {42999170},   {982906996},  {135497281},  {511702305},  {2084420925}, {1937477084}, {1827336327},
        {572660336},  {1159126505}, {805750846},  {1632621729}, {1100661313}, {1433925857}, {1141616124}, {84353895},
        {939819582},  {2001100545}, {1998898814}, {1548233367}, {610515434},  {1585990364}, {1374344043}, {760313750},
        {1477171087}, {356426808},  {945117276},  {1889947178}, {1780695788}, {709393584},  {491705403},  {1918502651},
        {752392754},  {1474612399}, {2053999932}, {1264095060}, {1411549676}, {1843993368}, {943947739},  {1984210012},
        {855636226},  {1749698586}, {1469348094}, {1956297539}, {1036140795}, {463480570},  {2040651434}, {1975960378},
        {317097467},  {1892066601}, {1376710097}, {927612902},  {1330573317}, {603570492},
    };
    // Insert data
    for (size_t i = 0; i < _ZP_ARRAY_SIZE(data); i++) {
        assert(_dummy_lru_cache_insert(&dcache, &data[i]) == 0);
    }
    // Check values
    for (size_t i = 0; i < _ZP_ARRAY_SIZE(data); i++) {
        _dummy_t *res = _dummy_lru_cache_get(&dcache, &data[i]);
        if (i < CACHE_CAPACITY) {
            assert(res == NULL);
        } else {
            assert(res != NULL);
            assert(res->foo == data[i].foo);
        }
    }
    // Update most values
    int not_upd_idx[CACHE_CAPACITY] = {
        34, 12, 42, 46, 56, 109, 103, 15, 96, 31,
    };
    for (size_t i = CACHE_CAPACITY; i < _ZP_ARRAY_SIZE(data); i++) {
        if (!val_in_array((int)i, not_upd_idx, _ZP_ARRAY_SIZE(not_upd_idx))) {
            assert(_dummy_lru_cache_get(&dcache, &data[i]) != NULL);
        }
    }
    // Insert back deleted value
    for (size_t i = 0; i < CACHE_CAPACITY; i++) {
        assert(_dummy_lru_cache_get(&dcache, &data[i]) == NULL);
        assert(_dummy_lru_cache_insert(&dcache, &data[i]) == 0);
    }
    // Check deleted values
    for (size_t i = 0; i < _ZP_ARRAY_SIZE(data); i++) {
        _dummy_t *res = _dummy_lru_cache_get(&dcache, &data[i]);
        if (val_in_array((int)i, not_upd_idx, _ZP_ARRAY_SIZE(not_upd_idx))) {
            assert(res == NULL);
        } else {
            assert(res != NULL);
            assert(res->foo == data[i].foo);
        }
    }
    // Clean-up
    _dummy_lru_cache_delete(&dcache);
}

#if 0
#define BENCH_THRESHOLD 1000000

void test_search_benchmark(size_t capacity) {
    _dummy_lru_cache_t dcache = _dummy_lru_cache_init(capacity);
    _dummy_t data[capacity];
    memset(data, 0, capacity * sizeof(_dummy_t));

    srand(0x55);
    // Insert data
    for (size_t i = 0; i < _ZP_ARRAY_SIZE(data); i++) {
        data[i].foo = rand();
        assert(_dummy_lru_cache_insert(&dcache, &data[i]) == 0);
    }
    z_clock_t measure_start = z_clock_now();
    for (size_t get_cnt = 0; get_cnt <= BENCH_THRESHOLD; get_cnt++) {
        int i = rand() % (int)_ZP_ARRAY_SIZE(data);
        _dummy_t *src = &data[i];
        _dummy_t *res = _dummy_lru_cache_get(&dcache, src);
        assert(res != NULL);
    }
    unsigned long elapsed_us = z_clock_elapsed_us(&measure_start);
    printf("Time to search: %ld\n", elapsed_us);
}

void test_insert_benchmark(size_t capacity) {
    _dummy_lru_cache_t dcache = _dummy_lru_cache_init(capacity);

    srand(0x55);
    // Insert data
    z_clock_t measure_start = z_clock_now();
    for (size_t get_cnt = 0; get_cnt <= BENCH_THRESHOLD; get_cnt++) {
        _dummy_t data = {.foo = rand()};
        assert(_dummy_lru_cache_insert(&dcache, &data) == 0);
    }
    unsigned long elapsed_us = z_clock_elapsed_us(&measure_start);
    printf("Time to insert: %ld\n", elapsed_us);
}

void test_benchmark(void) {
    for (size_t i = 1; i <= BENCH_THRESHOLD; i *= 10) {
        printf("Capacity: %ld\n", i);
        test_search_benchmark(i);
        test_insert_benchmark(i);
    }
}
#endif

int main(void) {
    test_lru_init();
    test_lru_cache_insert();
    test_lru_cache_clear();
    test_lru_cache_deletion();
    test_lru_cache_update();
    test_lru_cache_random_val();
#if 0
    test_benchmark();
#endif
    return 0;
}
