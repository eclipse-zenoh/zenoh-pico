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

/*-------- element functions --------*/
typedef void (*z_element_clear_f)(void *e);
typedef void (*z_element_free_f)(void **e);
typedef void *(*z_element_clone_f)(const void *e);
typedef int (*z_element_cmp_f)(const void *left, const void *right);

#define _Z_ELEMENT(name, type, elem_clear_f)             \
    static inline void z_element_free_##name##(void *e)  \
    {                                                    \
        return elem_clear_f((type *)e);                  \
    }                                                    \
    static inline void z_element_free_##name##(void **e) \
    {                                                    \
        return elem_free_f((type **)e);                  \
    }

#define _Z_ELEMENT_clone(name, type, elem_clone_f)              \
    static inline void *z_element_clone_##name##(const void *e) \
    {                                                           \
        return (void *)elem_clone_f((const type *)e);           \
    }

#define _Z_ELEMENT_CMP(name, type, elem_cmp_f)                                    \
    static inline int z_element_cmp_##name##(const void *left, const void *right) \
    {                                                                             \
        return elem_cmp_f((const type *)left, (const type *)right);               \
    }

/*-------- noop --------*/
void _zn_element_free_noop(void **s);
void *_zn_element_clone_noop(const void *s);
int _zn_element_cmp_noop(const void *left, const void *right);

/*-------- string --------*/
void z_element_free_str(void **s);
void *z_element_clone_str(const void *s);
int z_element_cmp_str(const void *left, const void *right);

/*-------- list of string --------*/
void z_element_free_list_str(void **s);
void *z_element_clone_list_str(const void *s);
int z_element_cmp_list_str(const void *left, const void *right);

#endif /* ZENOH_PICO_COLLECTIONS_ELEMENT_H */
