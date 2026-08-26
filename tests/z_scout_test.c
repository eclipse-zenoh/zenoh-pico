//
// Copyright (c) 2026 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License 2.0 which is
// available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//

#include <assert.h>

#include "zenoh-pico/session/utils.h"

int main(void) {
#if Z_FEATURE_SCOUTING == 1
    _z_s_msg_hello_t message = {0};
    _z_hello_slist_t *hellos = NULL;

    assert(_z_scout_process_hello(&message, &hellos, true) == _Z_NO_DATA_PROCESSED);
    assert(hellos == NULL);

    assert(_z_scout_process_hello(&message, &hellos, false) == _Z_RES_OK);
    assert(hellos != NULL);
    assert(_z_string_svec_len(&_z_hello_slist_value(hellos)->_locators) == 0);

    _z_hello_slist_free(&hellos);
#endif
    return 0;
}
