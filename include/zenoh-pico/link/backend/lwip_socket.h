//
// Copyright (c) 2026 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   ZettaScale Zenoh Team, <zenoh@zettascale.tech>
//

#ifndef ZENOH_PICO_LINK_BACKEND_LWIP_SOCKET_H
#define ZENOH_PICO_LINK_BACKEND_LWIP_SOCKET_H

#include "zenoh-pico/system/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(ZP_PLATFORM_SOCKET_LWIP) && defined(ZP_PLATFORM_SOCKET_LINKS_ENABLED) && \
    !defined(ZP_LWIP_SOCKET_HELPERS_DEFINED)
#error "LWIP socket helpers must be provided by the selected platform header"
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_LINK_BACKEND_LWIP_SOCKET_H */
