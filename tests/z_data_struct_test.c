/*
 * Copyright (c) 2017, 2021 ADLINK Technology Inc.
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

#include <assert.h>
#include <stdio.h>
#include "zenoh-pico/collections/intmap.h"
#include "zenoh-pico/collections/list.h"

int main(void)
{
    _z_str_list_t *xs = _z_str_list_make();
    _z_str_list_push(xs, _z_str_clone("one"));

    xs = _z_str_list_push(xs, _z_str_clone("two"));
    xs = _z_str_list_push(xs, _z_str_clone("three"));
    printf("list len = %zu\n", _z_str_list_len(xs));
    xs = _z_str_list_pop(xs);
    printf("list len = %zu\n", _z_str_list_len(xs));
    _z_str_list_free(&xs);

    // str-intmap
    _z_str_intmap_t map = _z_str_intmap_make();
    assert(_z_str_intmap_is_empty(&map));

    char s[64];
    int len = 128;
    for (int i = 0; i < len; i++)
    {
        sprintf(s, "%d", i);
        _z_str_intmap_insert(&map, i, _z_str_clone(s));
    }

    printf("Map size: %zu\n", _z_str_intmap_len(&map));
    assert(_z_str_intmap_len(&map) == len);

    for (int i = 0; i < len; i++)
    {
        sprintf(s, "%d", i);
        z_str_t e = _z_str_intmap_get(&map, i);
        printf("get(%d) = %s\n", i, e);
        assert(e != NULL);
        assert(_z_str_cmp(s, e));
    }

    for (int i = 0; i < len; i++)
    {
        _z_str_intmap_remove(&map, i);
        assert(_z_str_intmap_get(&map, i) == NULL);
        assert(_z_str_intmap_len(&map) == (len - 1) - i);
    }

    assert(_z_str_intmap_is_empty(&map));

    return 0;
}
