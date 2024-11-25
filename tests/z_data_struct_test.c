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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/api/types.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/transport/transport.h"

#undef NDEBUG
#include <assert.h>

void entry_list_test(void) {
    _z_transport_peer_entry_list_t *root = _z_transport_peer_entry_list_new();
    for (int i = 0; i < 10; i++) {
        _z_transport_peer_entry_t *entry = (_z_transport_peer_entry_t *)z_malloc(sizeof(_z_transport_peer_entry_t));
        memset(entry, 0, sizeof(_z_transport_peer_entry_t));
        root = _z_transport_peer_entry_list_insert(root, entry);
    }
    _z_transport_peer_entry_list_t *list = root;
    for (int i = 10; list != NULL; i--, list = _z_transport_peer_entry_list_tail(list)) {
        assert(_z_transport_peer_entry_list_head(list)->_peer_id == i);
    }
    _z_transport_peer_entry_list_head(root)->_peer_id = _Z_KEYEXPR_MAPPING_UNKNOWN_REMOTE - 1;

    for (int i = 0; i < 11; i++) {
        _z_transport_peer_entry_t *entry = (_z_transport_peer_entry_t *)z_malloc(sizeof(_z_transport_peer_entry_t));
        memset(entry, 0, sizeof(_z_transport_peer_entry_t));
        root = _z_transport_peer_entry_list_insert(root, entry);
    }
    assert(_z_transport_peer_entry_list_head(root)->_peer_id == _Z_KEYEXPR_MAPPING_UNKNOWN_REMOTE - 1);
    list = _z_transport_peer_entry_list_tail(root);
    for (int i = 20; list != NULL; i--, list = _z_transport_peer_entry_list_tail(list)) {
        assert(_z_transport_peer_entry_list_head(list)->_peer_id == i);
    }
    _z_transport_peer_entry_list_free(&root);
}

void str_vec_list_intmap_test(void) {
    char *s = (char *)malloc(64);
    size_t len = 128;

    // str-vec
    printf(">>> str-vec\r\n");

    _z_str_vec_t vec = _z_str_vec_make(1);
    assert(_z_str_vec_is_empty(&vec) == true);

    for (size_t i = 0; i < len; i++) {
        snprintf(s, 64, "%zu", i);

        _z_str_vec_append(&vec, _z_str_clone(s));
        char *e = _z_str_vec_get(&vec, i);
        printf("append(%zu) = %s\r\n", i, e);
        assert(_z_str_eq(s, e) == true);

        _z_str_vec_set(&vec, i, _z_str_clone(s));
        e = _z_str_vec_get(&vec, i);
        printf("set(%zu) = %s\r\n", i, e);
        assert(_z_str_eq(s, e) == true);

        assert(_z_str_vec_len(&vec) == i + 1);
    }
    assert(_z_str_vec_len(&vec) == len);

    _z_str_vec_clear(&vec);
    assert(_z_str_vec_is_empty(&vec) == true);

    // str-list
    printf(">>> str-list\r\n");

    _z_str_list_t *list = _z_str_list_new();
    assert(_z_str_list_is_empty(list) == true);

    for (size_t i = 0; i < len; i++) {
        snprintf(s, 64, "%zu", i);
        list = _z_str_list_push(list, _z_str_clone(s));

        char *e = _z_str_list_head(list);
        printf("push(%zu) = %s\r\n", i, e);
        assert(_z_str_eq(s, e) == true);

        assert(_z_str_list_len(list) == i + 1);
    }
    assert(_z_str_list_len(list) == len);

    for (size_t i = 0; i < len; i++) {
        snprintf(s, 64, "%zu", i);
        list = _z_str_list_pop(list, NULL);
        assert(_z_str_list_len(list) == len - (i + 1));
    }
    assert(_z_str_list_is_empty(list) == true);

    for (size_t i = 0; i < len; i++) {
        snprintf(s, 64, "%zu", i);
        list = _z_str_list_push(list, _z_str_clone(s));
        assert(_z_str_eq(s, _z_str_list_head(list)) == true);
    }
    assert(_z_str_list_len(list) == len);
    _z_str_list_free(&list);
    assert(_z_str_list_is_empty(list) == true);

    // str-intmap
    printf(">>> str-intmap\r\n");

    _z_str_intmap_t map = _z_str_intmap_make();
    assert(_z_str_intmap_is_empty(&map) == true);

    for (size_t i = 0; i < len; i++) {
        snprintf(s, 64, "%zu", i);
        _z_str_intmap_insert(&map, i, _z_str_clone(s));

        char *e = _z_str_intmap_get(&map, i);
        printf("get(%zu) = %s\r\n", i, e);
        assert(_z_str_eq(s, e) == true);

        assert(_z_str_intmap_len(&map) == i + 1);
    }
    assert(_z_str_intmap_len(&map) == len);

    for (size_t i = 0; i < len; i++) {
        _z_str_intmap_remove(&map, i);
        assert(_z_str_intmap_get(&map, i) == NULL);
        assert(_z_str_intmap_len(&map) == (len - 1) - i);
    }
    assert(_z_str_intmap_is_empty(&map) == true);

    _z_str_intmap_clear(&map);
    assert(_z_str_intmap_is_empty(&map) == true);

    z_free(s);
}

void _z_slice_custom_deleter(void *data, void *context) {
    _ZP_UNUSED(data);
    size_t *cnt = (size_t *)context;
    (*cnt)++;
}

void z_slice_custom_delete_test(void) {
    size_t counter = 0;
    uint8_t data[5] = {1, 2, 3, 4, 5};
    _z_delete_context_t dc = (_z_delete_context_t){.deleter = _z_slice_custom_deleter, .context = &counter};
    _z_slice_t s1 = _z_slice_from_buf_custom_deleter(data, 5, dc);
    _z_slice_t s2 = _z_slice_from_buf_custom_deleter(data, 5, dc);
    _z_slice_t s3 = _z_slice_copy_from_buf(data, 5);
    _z_slice_t s4 = _z_slice_alias_buf(data, 5);
    assert(_z_slice_is_alloced(&s1));
    assert(_z_slice_is_alloced(&s2));
    assert(_z_slice_is_alloced(&s3));
    assert(!_z_slice_is_alloced(&s4));

    _z_slice_clear(&s1);
    _z_slice_clear(&s2);
    _z_slice_clear(&s3);
    _z_slice_clear(&s4);
    assert(counter == 2);
}

void z_string_array_test(void) {
    // create new array
    z_owned_string_array_t a;
    z_string_array_new(&a);

    char s1[] = "string1";
    char s2[] = "string2";
    char s3[] = "string3";
    char s4[] = "string4";

    // add by copy
    z_view_string_t vs1;
    z_view_string_from_str(&vs1, s1);
    assert(z_string_array_push_by_copy(z_string_array_loan_mut(&a), z_view_string_loan(&vs1)) == 1);

    z_view_string_t vs2;
    z_view_string_from_str(&vs2, s2);
    assert(z_string_array_push_by_copy(z_string_array_loan_mut(&a), z_view_string_loan(&vs2)) == 2);

    // add by alias
    z_view_string_t vs3;
    z_view_string_from_str(&vs3, s3);
    assert(z_string_array_push_by_alias(z_string_array_loan_mut(&a), z_view_string_loan(&vs3)) == 3);

    z_view_string_t vs4;
    z_view_string_from_str(&vs4, s4);
    assert(z_string_array_push_by_alias(z_string_array_loan_mut(&a), z_view_string_loan(&vs4)) == 4);

    // check values
    const z_loaned_string_t *ls1 = z_string_array_get(z_string_array_loan(&a), 0);
    assert(strncmp(z_string_data(ls1), s1, z_string_len(ls1)) == 0);

    const z_loaned_string_t *ls2 = z_string_array_get(z_string_array_loan(&a), 1);
    assert(strncmp(z_string_data(ls2), s2, z_string_len(ls2)) == 0);

    const z_loaned_string_t *ls3 = z_string_array_get(z_string_array_loan(&a), 2);
    assert(strncmp(z_string_data(ls3), s3, z_string_len(ls3)) == 0);

    const z_loaned_string_t *ls4 = z_string_array_get(z_string_array_loan(&a), 3);
    assert(strncmp(z_string_data(ls4), s4, z_string_len(ls4)) == 0);

    // modify original strings values
    s1[0] = 'X';
    s2[0] = 'X';
    s3[0] = 'X';
    s4[0] = 'X';

    // values passed by copy should NOT be changed
    ls1 = z_string_array_get(z_string_array_loan(&a), 0);
    assert(strncmp(z_string_data(ls1), "string1", z_string_len(ls1)) == 0);

    ls2 = z_string_array_get(z_string_array_loan(&a), 1);
    assert(strncmp(z_string_data(ls2), "string2", z_string_len(ls2)) == 0);

    // values passed by alias should be changed
    ls3 = z_string_array_get(z_string_array_loan(&a), 2);
    assert(strncmp(z_string_data(ls3), s3, z_string_len(ls3)) == 0);

    ls4 = z_string_array_get(z_string_array_loan(&a), 3);
    assert(strncmp(z_string_data(ls4), s4, z_string_len(ls4)) == 0);

    // cleanup
    z_string_array_drop(z_string_array_move(&a));
}

void z_id_to_string_test(void) {
    z_id_t id;
    for (uint8_t i = 0; i < sizeof(id.id); i++) {
        id.id[i] = i;
    }
    z_owned_string_t id_str;
    z_id_to_string(&id, &id_str);
    assert(z_string_len(z_string_loan(&id_str)) == 32);
    assert(strncmp("0f0e0d0c0b0a09080706050403020100", z_string_data(z_string_loan(&id_str)),
                   z_string_len(z_string_loan(&id_str))) == 0);
    z_string_drop(z_string_move(&id_str));
}

int main(void) {
    entry_list_test();
    str_vec_list_intmap_test();
    z_slice_custom_delete_test();
    z_string_array_test();
    z_id_to_string_test();

    return 0;
}
