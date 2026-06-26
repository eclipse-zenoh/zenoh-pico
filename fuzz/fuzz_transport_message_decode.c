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

#include <stddef.h>
#include <stdint.h>

#include "zenoh-pico/collections/slice.h"
#include "zenoh-pico/protocol/codec/transport.h"
#include "zenoh-pico/protocol/definitions/transport.h"
#include "zenoh-pico/protocol/iobuf.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    _z_slice_t slice = _z_slice_alias_buf(data, size);
    _z_zbuf_t zbf = _z_slice_as_zbuf(slice);
    _z_transport_message_t msg = {0};

    (void)_z_transport_message_decode(&msg, &zbf);
    _z_t_msg_clear(&msg);

    return 0;
}
