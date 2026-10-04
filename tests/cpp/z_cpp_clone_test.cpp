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

#include "zenoh-pico.h"

#undef NDEBUG
#include <cassert>

int main() {
#if defined(Z_FEATURE_UNSTABLE_API) && Z_FEATURE_QUERY == 1
    z_owned_cancellation_token_t token;
    z_owned_cancellation_token_t clone;
    assert(z_cancellation_token_new(&token) == Z_OK);
    assert(z_clone(&clone, z_loan(token)) == Z_OK);
    assert(z_internal_check(clone));
    assert(!z_cancellation_token_is_cancelled(z_loan(clone)));
    assert(z_cancellation_token_cancel(z_loan_mut(token)) == Z_OK);
    assert(z_cancellation_token_is_cancelled(z_loan(clone)));
    z_drop(z_move(token));
    assert(z_cancellation_token_is_cancelled(z_loan(clone)));
    z_drop(z_move(clone));
#endif
    return 0;
}
