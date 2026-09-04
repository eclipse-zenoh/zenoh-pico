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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "utils/assert_helpers.h"
#include "zenoh-pico.h"
#include "zenoh-pico/protocol/definitions/transport.h"
#include "zenoh-pico/protocol/iobuf.h"
#include "zenoh-pico/transport/common/tx.h"

#if Z_FEATURE_FRAGMENTATION == 1

// The R flag carried by a fragment header selects the peer's defragmentation buffer and SN
// counter on the receiving side, so it must reflect the reliability the fragment was sent with.
static void test_fragment_header_reliability(z_reliability_t reliability, bool expect_r_flag) {
    _z_wbuf_t dst = _z_wbuf_make(64, false);
    _z_wbuf_t src = _z_wbuf_make(16, false);
    uint8_t payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    _z_wbuf_write_bytes(&src, payload, 0, sizeof(payload));

    ASSERT_OK(__unsafe_z_serialize_zenoh_fragment(&dst, &src, reliability, 1, true));

    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&dst);
    uint8_t header = _z_zbuf_read(&zbf);
    if (expect_r_flag) {
        ASSERT_TRUE(_Z_HAS_FLAG(header, _Z_FLAG_T_FRAGMENT_R));
    } else {
        ASSERT_FALSE(_Z_HAS_FLAG(header, _Z_FLAG_T_FRAGMENT_R));
    }

    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&dst);
    _z_wbuf_clear(&src);
}

int main(void) {
    test_fragment_header_reliability(Z_RELIABILITY_RELIABLE, true);
    test_fragment_header_reliability(Z_RELIABILITY_BEST_EFFORT, false);
    return 0;
}

#else
int main(void) { return 0; }
#endif
