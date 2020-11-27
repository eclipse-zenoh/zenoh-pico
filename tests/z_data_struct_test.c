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

#include <assert.h>
#include <stdio.h>
#include "zenoh-pico/private/collection.h"
#include "zenoh-pico/types.h"

int main(void)
{
    _z_list_t *xs = _z_list_of("one");
    _z_i_map_t *map = _z_i_map_make(5);

    xs = _z_list_cons(xs, "two");
    xs = _z_list_cons(xs, "three");
    printf("list len = %zu\n", _z_list_len(xs));
    xs = _z_list_drop_val(xs, 1);
    printf("list len = %zu\n", _z_list_len(xs));
    _z_list_free(xs);

    _z_i_map_set(map, 0, "0");
    _z_i_map_set(map, 1, "1");
    _z_i_map_set(map, 2, "2");
    _z_i_map_set(map, 3, "3");
    _z_i_map_set(map, 4, "4");
    _z_i_map_set(map, 5, "5");
    _z_i_map_set(map, 6, "6");
    _z_i_map_set(map, 7, "7");
    _z_i_map_set(map, 8, "8");
    _z_i_map_set(map, 9, "9");
    _z_i_map_set(map, 10, "10");

    printf("Map size: %zu\n", _z_i_map_len(map));
    printf("get(0) = %s\n", (char *)_z_i_map_get(map, 0));
    printf("get(1) = %s\n", (char *)_z_i_map_get(map, 1));
    printf("get(2) = %s\n", (char *)_z_i_map_get(map, 2));
    printf("get(3) = %s\n", (char *)_z_i_map_get(map, 3));
    printf("get(4) = %s\n", (char *)_z_i_map_get(map, 4));
    printf("get(5) = %s\n", (char *)_z_i_map_get(map, 5));
    printf("get(6) = %s\n", (char *)_z_i_map_get(map, 6));
    printf("get(7) = %s\n", (char *)_z_i_map_get(map, 7));
    printf("get(8) = %s\n", (char *)_z_i_map_get(map, 8));
    printf("get(9) = %s\n", (char *)_z_i_map_get(map, 9));
    printf("get(10) = %s\n", (char *)_z_i_map_get(map, 10));

    _z_i_map_remove(map, 7);
    assert(0 == _z_i_map_get(map, 7));
    _z_i_map_remove(map, 0);
    assert(0 == _z_i_map_get(map, 0));
    printf("get(5) = %s\n", (char *)_z_i_map_get(map, 5));
    return 0;
}
