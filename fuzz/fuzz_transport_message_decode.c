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
    _z_zbuf_t zbf = _z_slice_as_zbuf(&slice);
    _z_transport_message_t msg = {0};

    if (_z_transport_message_decode(&msg, &zbf) == _Z_RES_OK) {
        // Re-encode valid decoded messages to exercise the transport encode path.
        _z_wbuf_t wbf = _z_wbuf_null();

        if (_z_wbuf_init(&wbf, size + 64, true) == _Z_RES_OK) {
            if (_z_transport_message_encode(&wbf, &msg) == _Z_RES_OK) {
                _z_zbuf_t encoded = _z_wbuf_to_zbuf(&wbf);
                _z_transport_message_t roundtrip = {0};

                (void)_z_transport_message_decode(&roundtrip, &encoded);
                _z_zbuf_clear(&encoded);
            }

            _z_wbuf_clear(&wbf);
        }
    }

    _z_zbuf_clear(&zbf);

    return 0;
}
