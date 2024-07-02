//
// Copyright (c) 2022 ZettaScale Technology
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

#include "zenoh-pico/net/publish.h"

#include <stddef.h>

#if Z_FEATURE_PUBLICATION == 1
void _z_publisher_clear(_z_publisher_t *pub) {
    _z_keyexpr_clear(&pub->_key);
    *pub = _z_publisher_null();
}

void _z_publisher_free(_z_publisher_t **pub) {
    _z_publisher_t *ptr = *pub;

    if (ptr != NULL) {
        _z_publisher_clear(ptr);

        z_free(ptr);
        *pub = NULL;
    }
}

_Bool _z_publisher_check(const _z_publisher_t *publisher) { return !_Z_RC_IS_NULL(&publisher->_zn); }
_z_publisher_t _z_publisher_null(void) {
    return (_z_publisher_t) {
        ._congestion_control = Z_CONGESTION_CONTROL_DEFAULT, ._id = 0, ._key = _z_keyexpr_null(),
        ._priority = Z_PRIORITY_DEFAULT, ._zn = _z_session_rc_null(),
#if Z_FEATURE_INTEREST == 1
        ._filter = (_z_write_filter_t) {
            ._interest_id = 0, .ctx = NULL
        }
#endif
    };
}
#endif
