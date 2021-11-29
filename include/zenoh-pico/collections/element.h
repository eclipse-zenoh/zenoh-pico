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

#ifndef ZENOH_PICO_COLLECTIONS_ELEMENT_H
#define ZENOH_PICO_COLLECTIONS_ELEMENT_H

#include <stdlib.h>

/*-------- element functions --------*/
typedef void (*z_element_clear_f)(void *e);
typedef void (*z_element_free_f)(void **e);
typedef void *(*z_element_clone_f)(const void *e);
typedef int (*z_element_cmp_f)(const void *left, const void *right);

#define _Z_ELEM_DEFINE(name, type, elem_clear_f, elem_clone_f, elem_cmp_f) \
    typedef int (*name##_cmp_f)(const type *left, const type *right);      \
    static inline void name##_elem_clear(void *e)                          \
    {                                                                      \
        elem_clear_f((type *)e);                                           \
    }                                                                      \
    static inline void name##_elem_free(void **e)                          \
    {                                                                      \
        type *ptr = (type *)*e;                                            \
        elem_clear_f(ptr);                                                 \
        *e = NULL;                                                         \
    }                                                                      \
    static inline void *name##_elem_clone(const void *e)                   \
    {                                                                      \
        return (void *)elem_clone_f((type *)e);                            \
    }                                                                      \
    static inline int name##_elem_cmp(const void *left, const void *right) \
    {                                                                      \
        return elem_cmp_f((type *)left, (type *)right);                    \
    }

/*------------------ void ----------------*/
static inline void _zn_noop_elem_clear(void *s)
{
    (void)(s);
}

static inline void _zn_noop_elem_free(void **s)
{
    (void)(s);
}

static inline void *_zn_noop_elem_clone(const void *s)
{
    (void)(s);
    return NULL;
}

static inline int _zn_noop_elem_cmp(const void *left, const void *right)
{
    (void)(left);
    (void)(right);
    return 0;
}

#endif /* ZENOH_PICO_COLLECTIONS_ELEMENT_H */
