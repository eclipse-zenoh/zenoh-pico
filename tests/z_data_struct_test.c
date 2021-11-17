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
    z_list_t *xs = z_list_of("one");
    zn_int_str_map_t map = zn_int_str_map_make();

    xs = z_list_cons(xs, "two");
    xs = z_list_cons(xs, "three");
    printf("list len = %zu\n", z_list_len(xs));
    xs = z_list_drop_val(xs, 1);
    printf("list len = %zu\n", z_list_len(xs));
    z_list_free(xs);

    zn_int_str_map_insert(&map, 0, "0");
    zn_int_str_map_insert(&map, 1, "1");
    zn_int_str_map_insert(&map, 2, "2");
    zn_int_str_map_insert(&map, 3, "3");
    zn_int_str_map_insert(&map, 4, "4");
    zn_int_str_map_insert(&map, 5, "5");
    zn_int_str_map_insert(&map, 6, "6");
    zn_int_str_map_insert(&map, 7, "7");
    zn_int_str_map_insert(&map, 8, "8");
    zn_int_str_map_insert(&map, 9, "9");
    zn_int_str_map_insert(&map, 10, "10");

    printf("Map size: %zu\n", zn_int_str_map_len(&map));
    printf("get(0) = %s\n", zn_int_str_map_get(&map, 0));
    printf("get(1) = %s\n", zn_int_str_map_get(&map, 1));
    printf("get(2) = %s\n", zn_int_str_map_get(&map, 2));
    printf("get(3) = %s\n", zn_int_str_map_get(&map, 3));
    printf("get(4) = %s\n", zn_int_str_map_get(&map, 4));
    printf("get(5) = %s\n", zn_int_str_map_get(&map, 5));
    printf("get(6) = %s\n", zn_int_str_map_get(&map, 6));
    printf("get(7) = %s\n", zn_int_str_map_get(&map, 7));
    printf("get(8) = %s\n", zn_int_str_map_get(&map, 8));
    printf("get(9) = %s\n", zn_int_str_map_get(&map, 9));
    printf("get(10) = %s\n", zn_int_str_map_get(&map, 10));

    zn_int_str_map_remove(&map, 7);
    assert(zn_int_str_map_get(&map, 7) == NULL);
    zn_int_str_map_remove(&map, 0);
    assert(zn_int_str_map_get(&map, 0) == NULL);
    printf("get(5) = %s\n", zn_int_str_map_get(&map, 5));
    return 0;
}
