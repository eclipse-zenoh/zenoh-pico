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

#ifndef ZENOH_C_MVAR_H_
#define ZENOH_C_MVAR_H_

typedef void* z_mvar_t;

z_mvar_t *z_mvar_empty();
int z_mvar_is_empty(z_mvar_t *mv);

z_mvar_t *z_mvar_of(void *e);
void * z_mvar_get(z_mvar_t *mv);
void z_mvar_put(z_mvar_t *mv, void *e);

#endif /* ZENOH_C_MVAR_H_ */
