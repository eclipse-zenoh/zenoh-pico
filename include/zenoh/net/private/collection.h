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

#ifndef ZENOH_C_NET_COLLECTION_H
#define ZENOH_C_NET_COLLECTION_H

#include "zenoh/private/collection.h"

#define _ZN_ARRAY_DECLARE(name) _ARRAY_DECLARE(_zn_##name##_t, name, _zn_)
#define _ZN_ARRAY_P_DECLARE(name) _ARRAY_P_DECLARE(_zn_##name##_t, name, _zn_)
#define _ZN_ARRAY_S_DEFINE(name, arr, len) _ARRAY_S_DEFINE(_zn_##name##_t, name, _zn_, arr, len)
#define _ZN_ARRAY_P_S_DEFINE(name, arr, len) _ARRAY_P_S_DEFINE(_zn_##name##_t, name, _zn_, arr, len)
#define _ZN_ARRAY_S_INIT(name, arr, len) _ARRAY_S_INIT(_zn_##name##_t, arr, len)
#define _ZN_ARRAY_P_S_INIT(name, arr, len) _ARRAY_P_S_INIT(_zn_##name##_t, arr, len)
#define _ZN_ARRAY_H_INIT(name, arr, len) _ARRAY_H_INIT(_zn_##name##_t, arr, len)
#define _ZN_ARRAY_S_COPY(name, arr, len) _ARRAY_S_COPY(_zn_##name##_t, arr, len)
#define _ZN_ARRAY_H_COPY(name, arr, len) _ARRAY_H_COPY(_zn_##name##_t, arr, len)

#endif /* ZENOH_C_NET_COLLECTION_H */
