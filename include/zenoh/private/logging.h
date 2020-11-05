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

#ifndef _ZENOH_PICO_LOGGING_H
#define _ZENOH_PICO_LOGGING_H

#if (ZENOH_DEBUG == 2)
#include <stdio.h>

#define _Z_DEBUG(x) printf(x)
#define _Z_DEBUG_VA(x, ...) printf(x, __VA_ARGS__)
#define _Z_ERROR(x, ...) printf(x, __VA_ARGS__)
#elif (ZENOH_DEBUG == 1)
#include <stdio.h>

#define _Z_ERROR(x, ...) printf(x, __VA_ARGS__)
#define _Z_DEBUG_VA(x, ...) (void)(0)
#define _Z_DEBUG(x) (void)(0)
#elif (ZENOH_DEBUG == 0)
#define _Z_DEBUG(x) (void)(0)
#define _Z_DEBUG_VA(x, ...) (void)(0)
#define _Z_ERROR(x, ...) (void)(0)
#endif

#endif /* _ZENOH_PICO_LOGGING_H */
