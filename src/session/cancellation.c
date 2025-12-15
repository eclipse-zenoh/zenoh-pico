//
// Copyright (c) 2025 ZettaScale Technology
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

#include "zenoh-pico/session/cancellation.h"

#include "zenoh-pico/session/liveliness.h"
#include "zenoh-pico/session/query.h"

z_result_t _z_cancellation_token_on_cancel_handler_cancel(_z_cancellation_token_on_cancel_handler_t *handler) {
    z_result_t ret = handler->_on_cancel != NULL ? handler->_on_cancel(handler->_arg) : _Z_RES_OK;
    if (ret == _Z_RES_OK) {
        if (_z_sync_group_check(&handler->_sync_group)) {
            ret = _z_sync_group_wait(&handler->_sync_group);
        }
    }
    _z_cancellation_token_on_cancel_handler_drop(handler);
    return ret;
}

z_result_t _z_cancellation_token_on_cancel_handler_cancel_deadline(_z_cancellation_token_on_cancel_handler_t *handler,
                                                                   const z_clock_t *deadline) {
    z_result_t ret = handler->_on_cancel != NULL ? handler->_on_cancel(handler->_arg) : _Z_RES_OK;
    if (ret == _Z_RES_OK) {
        if (_z_sync_group_check(&handler->_sync_group)) {
            ret = _z_sync_group_wait_deadline(&handler->_sync_group, deadline);
        }
    }
    _z_cancellation_token_on_cancel_handler_drop(handler);
    return ret;
}

z_result_t _z_cancellation_token_create(_z_cancellation_token_t *ct) {
    ct->_is_cancelled = false;
    ct->_cancel_result = _Z_RES_OK;
    ct->_handlers = _z_cancellation_token_on_cancel_handler_svec_null();
#if Z_FEATURE_MULTI_THREAD == 1
    return _z_mutex_init(&ct->_mutex);
#else
    return _Z_RES_OK;
#endif
}

bool _z_cancellation_token_is_cancelled(const _z_cancellation_token_t *ct) {
#if Z_FEATURE_MULTI_THREAD == 1
    _z_mutex_lock((_z_mutex_t *)&ct->_mutex);
#endif
    bool ret = ct->_is_cancelled;
#if Z_FEATURE_MULTI_THREAD == 1
    _z_mutex_unlock((_z_mutex_t *)&ct->_mutex);
#endif
    return ret;
}

z_result_t _z_cancellation_token_cancel(_z_cancellation_token_t *ct) {
    z_result_t ret = _Z_RES_OK;
#if Z_FEATURE_MULTI_THREAD == 1
    _Z_RETURN_IF_ERR(_z_mutex_lock(&ct->_mutex));
#endif
    if (ct->_is_cancelled) {
        ret = ct->_cancel_result;
#if Z_FEATURE_MULTI_THREAD == 1
        _z_mutex_unlock(&ct->_mutex);
#endif
        return ret;
    }
    ct->_is_cancelled = true;

    for (size_t i = 0; i < _z_cancellation_token_on_cancel_handler_svec_len(&ct->_handlers); ++i) {
        _z_cancellation_token_on_cancel_handler_t *h =
            _z_cancellation_token_on_cancel_handler_svec_get(&ct->_handlers, i);
        ret = _z_cancellation_token_on_cancel_handler_cancel(h);
        if (ret != _Z_RES_OK) {
            break;
        }
    }
    _z_cancellation_token_on_cancel_handler_svec_clear(&ct->_handlers);
    ct->_cancel_result = ret;
#if Z_FEATURE_MULTI_THREAD == 1
    _z_mutex_unlock(&ct->_mutex);
#endif
    return ret;
}

z_result_t _z_cancellation_token_cancel_with_timeout(_z_cancellation_token_t *ct, uint32_t timeout_ms) {
    z_result_t ret = _Z_RES_OK;
    z_clock_t deadline = z_clock_now();
    z_clock_advance_ms(&deadline, timeout_ms);
#if Z_FEATURE_MULTI_THREAD == 1
    _Z_RETURN_IF_ERR(_z_mutex_lock(&ct->_mutex));
#endif
    if (ct->_is_cancelled) {
        ret = ct->_cancel_result;
#if Z_FEATURE_MULTI_THREAD == 1
        _z_mutex_unlock(&ct->_mutex);
#endif
        return ret;
    }

    for (size_t i = 0; i < _z_cancellation_token_on_cancel_handler_svec_len(&ct->_handlers); ++i) {
        _z_cancellation_token_on_cancel_handler_t *h =
            _z_cancellation_token_on_cancel_handler_svec_get(&ct->_handlers, i);
        ret = _z_cancellation_token_on_cancel_handler_cancel_deadline(h, &deadline);
        if (ret != _Z_RES_OK) {
            break;
        }
    }
    _z_cancellation_token_on_cancel_handler_svec_clear(&ct->_handlers);
    ct->_is_cancelled = true;
    ct->_cancel_result = ret;
#if Z_FEATURE_MULTI_THREAD == 1
    _z_mutex_unlock(&ct->_mutex);
#endif
    return ret;
}

void _z_cancellation_token_clear(_z_cancellation_token_t *ct) {
#if Z_FEATURE_MULTI_THREAD == 1
    _z_mutex_drop(&ct->_mutex);
#endif
    _z_cancellation_token_on_cancel_handler_svec_clear(&ct->_handlers);
}

z_result_t _z_cancellation_token_add_on_cancel_handler(_z_cancellation_token_t *ct,
                                                       _z_cancellation_token_on_cancel_handler_t *handler) {
    z_result_t ret = _Z_RES_OK;
#if Z_FEATURE_MULTI_THREAD == 1
    _Z_RETURN_IF_ERR(_z_mutex_lock(&ct->_mutex));
#endif
    if (ct->_is_cancelled) {
        ret = _z_cancellation_token_on_cancel_handler_cancel(handler);
    } else {
        ret = _z_cancellation_token_on_cancel_handler_svec_append(&ct->_handlers, handler, true);
        if (ret != _Z_RES_OK) {
            _z_cancellation_token_on_cancel_handler_drop(handler);
        }
    }
#if Z_FEATURE_MULTI_THREAD == 1
    _z_mutex_unlock(&ct->_mutex);
#endif
    return ret;
}

// Query cancellation

#if Z_FEATURE_QUERY == 1
typedef struct {
    _z_session_weak_t zn;
    _z_zint_t qid;
} __z_cancellation_token_remove_pending_query_arg;

z_result_t ___z_cancellation_token_remove_pending_query(void *arg) {
    __z_cancellation_token_remove_pending_query_arg *typed_arg = (__z_cancellation_token_remove_pending_query_arg *)arg;
    _z_pending_query_t pq = {0};
    pq._id = typed_arg->qid;
    _z_session_rc_t sess_rc = _z_session_weak_upgrade(&typed_arg->zn);
    if (!_Z_RC_IS_NULL(&sess_rc)) {
        _z_unregister_pending_query(_Z_RC_IN_VAL(&typed_arg->zn), &pq);
        _z_session_rc_drop(&sess_rc);
    }
    return _Z_RES_OK;
}

void ___z_cancellation_token_remove_pending_query_drop(void *arg) {
    __z_cancellation_token_remove_pending_query_arg *typed_arg = (__z_cancellation_token_remove_pending_query_arg *)arg;
    _z_session_weak_drop(&typed_arg->zn);
    z_free(arg);
}

z_result_t _z_cancellation_token_add_on_query_cancel_handler(_z_cancellation_token_t *ct, const _z_session_rc_t *zs,
                                                             _z_zint_t qid) {
    __z_cancellation_token_remove_pending_query_arg *arg = (__z_cancellation_token_remove_pending_query_arg *)z_malloc(
        sizeof(__z_cancellation_token_remove_pending_query_arg));
    if (arg == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    arg->qid = qid;
    arg->zn = _z_session_rc_clone_as_weak(zs);
    // sync group is not needed for queries since currently the callback execution is protected by session mutex
    // i.e. ___z_cancellation_token_remove_pending_query will not return until callback is dropped.
    // In particular it means that using cancel_with_timeout does not make much sense
    _z_cancellation_token_on_cancel_handler_t handler = {._arg = arg,
                                                         ._on_cancel = ___z_cancellation_token_remove_pending_query,
                                                         ._on_drop = ___z_cancellation_token_remove_pending_query_drop,
                                                         ._sync_group = _z_sync_group_null()};
    return _z_cancellation_token_add_on_cancel_handler(ct, &handler);
}

#if Z_FEATURE_LIVELINESS == 1
z_result_t ___z_cancellation_token_remove_pending_liveliness_query(void *arg) {
    __z_cancellation_token_remove_pending_query_arg *typed_arg = (__z_cancellation_token_remove_pending_query_arg *)arg;
    _z_session_rc_t sess_rc = _z_session_weak_upgrade(&typed_arg->zn);
    if (!_Z_RC_IS_NULL(&sess_rc)) {
        _z_liveliness_unregister_pending_query(_Z_RC_IN_VAL(&typed_arg->zn), (uint32_t)typed_arg->qid);
        _z_session_rc_drop(&sess_rc);
    }
    return _Z_RES_OK;
}

z_result_t _z_cancellation_token_add_on_liveliness_query_cancel_handler(_z_cancellation_token_t *ct,
                                                                        const _z_session_rc_t *zs, _z_zint_t qid) {
    __z_cancellation_token_remove_pending_query_arg *arg = (__z_cancellation_token_remove_pending_query_arg *)z_malloc(
        sizeof(__z_cancellation_token_remove_pending_query_arg));
    if (arg == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    arg->qid = qid;
    arg->zn = _z_session_rc_clone_as_weak(zs);
    // sync group is not needed for liveliness queries since currently the callback execution is protected by session
    // mutex i.e. ___z_cancellation_token_remove_pending_query will not return until callback is dropped. In particular
    // it means that using cancel_with_timeout does not make much sense
    _z_cancellation_token_on_cancel_handler_t handler = {
        ._arg = arg,
        ._on_cancel = ___z_cancellation_token_remove_pending_liveliness_query,
        ._on_drop = ___z_cancellation_token_remove_pending_query_drop,
        ._sync_group = _z_sync_group_null()};
    return _z_cancellation_token_add_on_cancel_handler(ct, &handler);
}
#endif
#endif
