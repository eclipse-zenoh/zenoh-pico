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
    size_t _capacity;
    size_t _len;
    void **_val;
} z_vec_t;

// @TODO: use element also for vector
z_vec_t z_vec_make(size_t capacity);
z_vec_t z_vec_clone(const z_vec_t *v);

size_t z_vec_len(const z_vec_t *v);
void z_vec_append(z_vec_t *v, void *e);
const void *z_vec_get(const z_vec_t *v, size_t i);
void z_vec_set(z_vec_t *sv, size_t i, void *e);

void z_vec_clear(z_vec_t *v);
void z_vec_free(z_vec_t *v);

#endif /* ZENOH_PICO_UTILS_COLLECTION_VECTOR_H */
