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

#undef NDEBUG
#include <assert.h>
#include <stdio.h>

#include "zenoh-pico/net/session.h"
#include "zenoh-pico/protocol/definitions/declarations.h"
#include "zenoh-pico/session/interest.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/utils/result.h"

#if Z_FEATURE_INTEREST == 1

// An undeclare of a declaration the session never registered - in multicast,
// one made before this peer joined the group - leaves nothing to undo and is
// not an error: the read task would otherwise treat it as a transport failure.
static void test_unknown_undeclare_is_not_an_error(_z_session_t *zn) {
    _z_declaration_t decls[] = {
        _z_make_undecl_subscriber(42, NULL),
        _z_make_undecl_queryable(42, NULL),
        _z_make_undecl_token(42, NULL),
    };
    for (size_t i = 0; i < sizeof(decls) / sizeof(decls[0]); i++) {
        assert(_z_interest_process_undeclares(zn, &decls[i], NULL) == _Z_RES_OK);
        _z_declaration_clear(&decls[i]);
    }
}

int main(void) {
    _z_session_t session;
    _z_id_t zid;
    _z_session_generate_zid(&zid, Z_ZID_LENGTH);
    assert(_z_session_init(&session, &zid) == _Z_RES_OK);

    test_unknown_undeclare_is_not_an_error(&session);

    _z_session_clear(&session);
    printf("z_interest_undeclare_test: OK\n");
    return 0;
}

#else

int main(void) {
    printf("Z_FEATURE_INTEREST is disabled, skipping\n");
    return 0;
}

#endif
