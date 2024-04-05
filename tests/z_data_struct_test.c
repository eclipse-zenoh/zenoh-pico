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

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/transport/transport.h"

#undef NDEBUG
#include <assert.h>

void entry_list_test(void) {
    _z_transport_peer_entry_list_t *root = _z_transport_peer_entry_list_new();
    for (int i = 0; i < 10; i++) {
        _z_transport_peer_entry_t *entry = (_z_transport_peer_entry_t *)z_malloc(sizeof(_z_transport_peer_entry_t));
        root = _z_transport_peer_entry_list_insert(root, entry);
    }
    _z_transport_peer_entry_list_t *list = root;
    for (int i = 10; list != NULL; i--, list = _z_transport_peer_entry_list_tail(list)) {
        assert(_z_transport_peer_entry_list_head(list)->_peer_id == i);
    }
    _z_transport_peer_entry_list_head(root)->_peer_id = _Z_KEYEXPR_MAPPING_UNKNOWN_REMOTE - 1;

    for (int i = 0; i < 11; i++) {
        _z_transport_peer_entry_t *entry = (_z_transport_peer_entry_t *)z_malloc(sizeof(_z_transport_peer_entry_t));
        root = _z_transport_peer_entry_list_insert(root, entry);
    }
    assert(_z_transport_peer_entry_list_head(root)->_peer_id == _Z_KEYEXPR_MAPPING_UNKNOWN_REMOTE - 1);
    list = _z_transport_peer_entry_list_tail(root);
    for (int i = 20; list != NULL; i--, list = _z_transport_peer_entry_list_tail(list)) {
        assert(_z_transport_peer_entry_list_head(list)->_peer_id == i);
    }
    _z_transport_peer_entry_list_free(&root);
}

int main(void) {
    entry_list_test();
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

    return 0;
}
