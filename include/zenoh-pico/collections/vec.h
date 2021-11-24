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

#ifndef ZENOH_PICO_UTILS_COLLECTION_VECTOR_H
#define ZENOH_PICO_UTILS_COLLECTION_VECTOR_H

#include <stdint.h>
#include <stdio.h>
#include "zenoh-pico/collections/element.h"

/*-------- Dynamically allocated vector --------*/
/**
 * A dynamically allocate vector.
 */
typedef struct
{
    size_t capacity;
    size_t len;
    void **val;
} _z_vec_t;

_z_vec_t _z_vec_make(size_t capacity);
_z_vec_t _z_vec_clone(const _z_vec_t *v, z_element_clone_f f);

size_t _z_vec_len(const _z_vec_t *v);
void _z_vec_append(_z_vec_t *v, void *e);
void *_z_vec_get(const _z_vec_t *v, size_t i);
void _z_vec_set(_z_vec_t *sv, size_t i, void *e, z_element_free_f f);

void _z_vec_reset(_z_vec_t *v, z_element_free_f f);
void _z_vec_clear(_z_vec_t *v, z_element_free_f f);
void _z_vec_free(_z_vec_t **v, z_element_free_f f);

#endif /* ZENOH_PICO_UTILS_COLLECTION_VECTOR_H */
