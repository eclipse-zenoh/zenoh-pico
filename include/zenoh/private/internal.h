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

#ifndef _ZENOH_C_INTERNAL_H
#define _ZENOH_C_INTERNAL_H

#include "zenoh/types.h"

/*-------- Operations on Bytes --------*/
void _z_bytes_copy(z_bytes_t *dst, z_bytes_t *src);
void _z_bytes_move(z_bytes_t *dst, z_bytes_t *src);
void _z_bytes_free(z_bytes_t *bs);

/*-------- Operations on StrArray --------*/
void _z_str_array_init(z_str_array_t *sa, size_t len);
void _z_str_array_copy(z_str_array_t *dst, z_str_array_t *src);
void _z_str_array_move(z_str_array_t *dst, z_str_array_t *src);
void _z_str_array_free(z_str_array_t *sa);

#endif /* _ZENOH_C_INTERNAL_H */
