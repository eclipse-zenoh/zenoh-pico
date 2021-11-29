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
 *     ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#include <stdlib.h>
#include <string.h>
#include "zenoh-pico/collections/list.h"
#include "zenoh-pico/collections/string.h"

/*------------------ noop ----------------*/
void _zn_element_free_noop(void **s)
{
    *s = NULL;
}

void *_zn_element_dup_noop(const void *s)
{
    return NULL;
}

int _zn_element_cmp_noop(const void *left, const void *right)
{
    return 0;
}

/*------------------ string ----------------*/
void z_element_free_str(void **s)
{
    z_str_t ptr = (z_str_t)s;
    free(ptr);
    s = NULL;
}

void *z_element_dup_str(const void *s)
{
    return (void *)_z_str_dup((const z_str_t)s);
}

int z_element_cmp_str(const void *left, const void *right)
{
    return strcmp((const z_str_t)left, (const z_str_t)right);
}

/*------------------ string list ----------------*/
void z_element_free_str_list(void **s)
{
    z_str_list_free((z_str_list_t **)s);
}

void *z_element_dup_str_list(const void *s)
{
    return (void *)z_str_list_dup((const z_str_list_t *)s);
}

int z_element_cmp_str_list(const void *left, const void *right)
{
    return z_str_list_cmp((const z_str_list_t *)left, (const z_str_list_t *)right);
}
