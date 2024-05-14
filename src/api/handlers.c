//
// Copyright (c) 2024 ZettaScale Technology
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

#include "zenoh-pico/api/handlers.h"

#include "zenoh-pico/net/sample.h"
#include "zenoh-pico/system/platform.h"

// -- Sample
void _z_owned_sample_move(z_owned_sample_t *dst, z_owned_sample_t *src) {
    memcpy(dst, src, sizeof(z_owned_sample_t));
    zp_free(src);
}

z_owned_sample_t *_z_sample_to_owned_ptr(const _z_sample_t *src) {
    z_owned_sample_t *dst = (z_owned_sample_t *)zp_malloc(sizeof(z_owned_sample_t));
    if (dst == NULL) {
        return NULL;
    }
    if (src != NULL) {
        dst->_value = (_z_sample_t *)zp_malloc(sizeof(_z_sample_t));
        _z_sample_copy(dst->_value, src);
    } else {
        dst->_value = NULL;
    }
    return dst;
}

#if Z_FEATURE_QUERYABLE == 1
// -- Query
void _z_owned_query_move(z_owned_query_t *dst, z_owned_query_t *src) {
    memcpy(dst, src, sizeof(z_owned_query_t));
    zp_free(src);
}

z_owned_query_t *_z_query_to_owned_ptr(const z_query_t *src) {
    z_owned_query_t *dst = (z_owned_query_t *)zp_malloc(sizeof(z_owned_query_t));
    _z_query_rc_copy(&dst->_rc, &src->_val._rc);
    return dst;
}
#endif  // Z_FEATURE_QUERYABLE

#if Z_FEATURE_QUERY == 1
// -- Reply
void _z_owned_reply_move(z_owned_reply_t *dst, z_owned_reply_t *src) {
    memcpy(dst, src, sizeof(z_owned_reply_t));
    zp_free(src);
}

z_owned_reply_t *_z_reply_clone(const z_owned_reply_t *src) {
    z_owned_reply_t *dst = (z_owned_reply_t *)zp_malloc(sizeof(z_owned_reply_t));
    if (dst == NULL) {
        return NULL;
    }
    if (src != NULL && src->_value) {
        dst->_value = (_z_reply_t *)zp_malloc(sizeof(_z_reply_t));
        _z_reply_copy(dst->_value, src->_value);
    } else {
        dst->_value = NULL;
    }
    return dst;
}
#endif  // Z_FEATURE_QUERY
