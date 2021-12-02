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
typedef size_t (*z_element_size_f)(void *e);
typedef void (*z_element_clear_f)(void *e);
typedef void (*z_element_free_f)(void **e);
typedef void (*z_element_copy_f)(void *dst, const void *src);
typedef void *(*z_element_clone_f)(const void *e);
typedef int (*z_element_eq_f)(const void *left, const void *right);

#define _Z_ELEM_DEFINE(name, type, elem_size_f, elem_clear_f, elem_copy_f) \
    typedef int (*name##_eq_f)(const type *left, const type *right);       \
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
    static inline void name##_elem_copy(void *dst, const void *src)        \
    {                                                                      \
        elem_copy_f((type *)dst, (type *)src);                             \
    }                                                                      \
    static inline void *name##_elem_clone(const void *src)                 \
    {                                                                      \
        type *dst = (type *)malloc(elem_size_f((type *)src));              \
        elem_copy_f(dst, (type *)src);                                     \
        return dst;                                                        \
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

static inline void _zn_noop_elem_copy(void *dst, const void *src)
{
    (void)(dst);
    (void)(src);
}

static inline void *_zn_noop_elem_clone(const void *src)
{
    (void)(src);
    return NULL;
}

#endif /* ZENOH_PICO_COLLECTIONS_ELEMENT_H */
