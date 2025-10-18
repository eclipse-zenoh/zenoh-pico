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

#include "zenoh-pico/net/filtering.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/net/primitives.h"
#include "zenoh-pico/session/matching.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/session.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/utils/locality.h"

#if Z_FEATURE_INTEREST == 1

typedef struct _z_write_filter_session_entry_t _z_write_filter_session_entry_t;
typedef struct _z_write_filter_registration_t _z_write_filter_registration_t;

struct _z_write_filter_registration_t {
    _z_write_filter_ctx_rc_t ctx_rc;
    _z_write_filter_session_entry_t *session_entry;
    _z_write_filter_registration_t *next;
};

struct _z_write_filter_session_entry_t {
    _z_session_t *session;
    _z_write_filter_registration_t *registrations;
    _z_write_filter_session_entry_t *next;
};

static _z_write_filter_session_entry_t *_z_write_filter_sessions = NULL;

#if Z_FEATURE_MULTI_THREAD == 1
static _z_mutex_t _z_write_filter_registry_mutex;
static bool _z_write_filter_registry_mutex_initialized = false;

static void _z_write_filter_registry_lock(void) {
    if (!_z_write_filter_registry_mutex_initialized) {
        _z_mutex_init(&_z_write_filter_registry_mutex);
        _z_write_filter_registry_mutex_initialized = true;
    }
    _z_mutex_lock(&_z_write_filter_registry_mutex);
}

static void _z_write_filter_registry_unlock(void) { _z_mutex_unlock(&_z_write_filter_registry_mutex); }
#else
static void _z_write_filter_registry_lock(void) {}
static void _z_write_filter_registry_unlock(void) {}
#endif

static bool _z_filter_target_peer_eq(const void *left, const void *right) {
    const _z_filter_target_t *left_val = (const _z_filter_target_t *)left;
    const _z_filter_target_t *right_val = (const _z_filter_target_t *)right;
    return left_val->peer == right_val->peer;
}

static bool _z_filter_target_eq(const void *left, const void *right) {
    const _z_filter_target_t *left_val = (const _z_filter_target_t *)left;
    const _z_filter_target_t *right_val = (const _z_filter_target_t *)right;
    return (left_val->peer == right_val->peer) && (left_val->decl_id == right_val->decl_id);
}

#if Z_FEATURE_MULTI_THREAD == 1
static void _z_write_filter_mutex_lock(_z_write_filter_ctx_t *ctx) { _z_mutex_lock(&ctx->mutex); }
static void _z_write_filter_mutex_unlock(_z_write_filter_ctx_t *ctx) { _z_mutex_unlock(&ctx->mutex); }
#else
static void _z_write_filter_mutex_lock(_z_write_filter_ctx_t *ctx) { _ZP_UNUSED(ctx); }
static void _z_write_filter_mutex_unlock(_z_write_filter_ctx_t *ctx) { _ZP_UNUSED(ctx); }
#endif

static bool _z_write_filter_push_target(_z_write_filter_ctx_t *ctx, _z_transport_peer_common_t *peer, uint32_t id) {
    _z_filter_target_t target = {.peer = (uintptr_t)peer, .decl_id = id};
    ctx->targets = _z_filter_target_slist_push(ctx->targets, &target);
    if (ctx->targets == NULL) {  // Allocation can fail
        return false;
    }
    return true;
}

static inline bool _z_write_filter_peer_allowed(const _z_write_filter_ctx_t *ctx, _z_transport_peer_common_t *peer) {
    return ((peer == NULL) && ctx->allow_local) || ((peer != NULL) && ctx->allow_remote);
}

static void _z_write_filter_ctx_update_state(_z_write_filter_ctx_t *ctx) {
    uint8_t prev_state = ctx->state;
    ctx->state = (ctx->targets == NULL && ctx->local_targets == 0) ? WRITE_FILTER_ACTIVE : WRITE_FILTER_OFF;
    if (prev_state != ctx->state) {
        _Z_DEBUG("Updated write filter state: %d", ctx->state);
#if Z_FEATURE_MATCHING
        _z_closure_matching_status_intmap_iterator_t it =
            _z_closure_matching_status_intmap_iterator_make(&ctx->callbacks);
        _z_matching_status_t s = {.matching = ctx->state != WRITE_FILTER_ACTIVE};
        while (_z_closure_matching_status_intmap_iterator_next(&it)) {
            _z_closure_matching_status_t *c = _z_closure_matching_status_intmap_iterator_value(&it);
            c->call(&s, c->context);
        }
#endif
    }
}

#if Z_FEATURE_LOCAL_SUBSCRIBER == 1 || Z_FEATURE_LOCAL_QUERYABLE == 1
static void _z_write_filter_ctx_add_local_match(_z_write_filter_ctx_t *ctx) {
    _z_write_filter_mutex_lock(ctx);
    ctx->local_targets++;
    _z_write_filter_ctx_update_state(ctx);
    _z_write_filter_mutex_unlock(ctx);
}

static void _z_write_filter_ctx_remove_local_match(_z_write_filter_ctx_t *ctx) {
    _z_write_filter_mutex_lock(ctx);
    if (ctx->local_targets > 0) {
        ctx->local_targets--;
    }
    _z_write_filter_ctx_update_state(ctx);
    _z_write_filter_mutex_unlock(ctx);
}
#endif

static _z_write_filter_session_entry_t *_z_write_filter_get_session_entry(_z_session_t *session, bool create) {
    _z_write_filter_session_entry_t *entry = _z_write_filter_sessions;
    while (entry != NULL) {
        if (entry->session == session) {
            return entry;
        }
        entry = entry->next;
    }
    if (!create) {
        return NULL;
    }
    entry = (_z_write_filter_session_entry_t *)z_malloc(sizeof(_z_write_filter_session_entry_t));
    if (entry == NULL) {
        return NULL;
    }
    entry->session = session;
    entry->registrations = NULL;
    entry->next = _z_write_filter_sessions;
    _z_write_filter_sessions = entry;
    return entry;
}

static void _z_write_filter_session_register(_z_session_t *session, _z_write_filter_ctx_t *ctx,
                                             _z_write_filter_ctx_rc_t *ctx_rc) {
    _z_write_filter_registry_lock();
    _z_write_filter_session_entry_t *entry = _z_write_filter_get_session_entry(session, true);
    if (entry == NULL) {
        _z_write_filter_registry_unlock();
        return;
    }

    _z_write_filter_registration_t *registration =
        (_z_write_filter_registration_t *)z_malloc(sizeof(_z_write_filter_registration_t));
    if (registration == NULL) {
        _z_write_filter_registry_unlock();
        return;
    }

    registration->ctx_rc = _z_write_filter_ctx_rc_clone(ctx_rc);
    if (_Z_RC_IS_NULL(&registration->ctx_rc)) {
        z_free(registration);
        _z_write_filter_registry_unlock();
        return;
    }
    registration->session_entry = entry;
    registration->next = entry->registrations;
    entry->registrations = registration;
    ctx->session_entry = registration;
    _z_write_filter_registry_unlock();
}

static void _z_write_filter_session_unregister(_z_write_filter_ctx_t *ctx) {
    if (ctx->session_entry == NULL) {
        return;
    }
    _z_write_filter_registration_t *registration = (_z_write_filter_registration_t *)ctx->session_entry;
    ctx->session_entry = NULL;

    _z_write_filter_registry_lock();

    _z_write_filter_session_entry_t *session_entry = registration->session_entry;
    if (session_entry == NULL) {
        goto out;
    }

    _z_write_filter_registration_t **iter = &session_entry->registrations;
    // Walk the linked list to find the write-filter registration we are going to unregister
    while (*iter != NULL && *iter != registration) {
        iter = &((*iter)->next);
    }
    if (*iter == NULL) {
        goto out;
    }

    // Remove the registration from the linked list by redirecting the stored pointer.
    *iter = registration->next;

    if (session_entry->registrations == NULL) {
        // No registrations left for this session entry; remove it from the global list.
        _z_write_filter_session_entry_t **sess_iter = &_z_write_filter_sessions;
        while (*sess_iter != NULL && *sess_iter != session_entry) {
            sess_iter = &((*sess_iter)->next);
        }
        if (*sess_iter != NULL) {
            *sess_iter = session_entry->next;
            // The session entry isn't used anymore, now we can free it.
            z_free(session_entry);
        }
    }

out:
    _z_write_filter_registry_unlock();

    _z_write_filter_ctx_rc_drop(&registration->ctx_rc);
    z_free(registration);
}

static void _z_write_filter_callback(const _z_interest_msg_t *msg, _z_transport_peer_common_t *peer, void *arg) {
    _z_write_filter_ctx_t *ctx = (_z_write_filter_ctx_t *)arg;
    // Process message
    _z_write_filter_mutex_lock(ctx);
    switch (msg->type) {
        case _Z_INTEREST_MSG_TYPE_DECL_SUBSCRIBER:
        case _Z_INTEREST_MSG_TYPE_DECL_QUERYABLE: {
            // the message might be a redeclare - so we need to remove the previous one first
            _z_filter_target_t target = {.decl_id = msg->id, .peer = (uintptr_t)peer};
            ctx->targets = _z_filter_target_slist_drop_first_filter(ctx->targets, _z_filter_target_eq, &target);
            bool peer_allowed = _z_write_filter_peer_allowed(ctx, peer);
            if (peer_allowed &&
                (!ctx->is_complete ||
                 (msg->is_complete && (ctx->is_aggregate || _z_keyexpr_suffix_includes(msg->key, &ctx->key))))) {
                _z_write_filter_push_target(ctx, peer, msg->id);
            }
            break;
        }
        case _Z_INTEREST_MSG_TYPE_UNDECL_SUBSCRIBER:
        case _Z_INTEREST_MSG_TYPE_UNDECL_QUERYABLE: {
            _z_filter_target_t target = {.decl_id = msg->id, .peer = (uintptr_t)peer};
            ctx->targets = _z_filter_target_slist_drop_first_filter(ctx->targets, _z_filter_target_eq, &target);
        } break;
        case _Z_INTEREST_MSG_TYPE_CONNECTION_DROPPED: {
            _z_filter_target_t target = {.decl_id = 0, .peer = (uintptr_t)peer};
            ctx->targets = _z_filter_target_slist_drop_all_filter(ctx->targets, _z_filter_target_peer_eq, &target);
        } break;
        default:
            break;
    }
    _z_write_filter_ctx_update_state(ctx);
    _z_write_filter_mutex_unlock(ctx);
}

z_result_t _z_write_filter_create(const _z_session_rc_t *zn, _z_write_filter_t *filter, _z_keyexpr_t keyexpr,
                                  uint8_t interest_flag, bool complete, z_locality_t locality) {
    uint8_t flags = interest_flag | _Z_INTEREST_FLAG_RESTRICTED | _Z_INTEREST_FLAG_CURRENT;
    // Add client specific flags
    if (_Z_RC_IN_VAL(zn)->_mode == Z_WHATAMI_CLIENT) {
        flags |= _Z_INTEREST_FLAG_KEYEXPRS | _Z_INTEREST_FLAG_AGGREGATE | _Z_INTEREST_FLAG_FUTURE;
    }
    filter->ctx = _z_write_filter_ctx_rc_null();
    _z_write_filter_ctx_t *ctx = (_z_write_filter_ctx_t *)z_malloc(sizeof(_z_write_filter_ctx_t));

    if (ctx == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
    }
#if Z_FEATURE_MULTI_THREAD == 1
    _Z_RETURN_IF_ERR(_z_mutex_init(&ctx->mutex));
#endif
    ctx->state = WRITE_FILTER_ACTIVE;
    ctx->targets = _z_filter_target_slist_new();
#if Z_FEATURE_MATCHING
    _z_closure_matching_status_intmap_init(&ctx->callbacks);
#endif
    ctx->is_complete = complete;
    ctx->is_aggregate = (flags & _Z_INTEREST_FLAG_AGGREGATE) != 0;
    ctx->allow_local = _z_locality_allows_local(locality);
    ctx->allow_remote = _z_locality_allows_remote(locality);
    ctx->key = _z_get_expanded_key_from_key(_Z_RC_IN_VAL(zn), &keyexpr, NULL);
    ctx->zn = _z_session_rc_clone_as_weak(zn);
    if (_Z_RC_IS_NULL(&ctx->zn)) {
        _z_write_filter_ctx_clear(ctx);
        z_free(ctx);
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
    }
    ctx->local_targets = 0;
    ctx->session_entry = NULL;
    filter->ctx = _z_write_filter_ctx_rc_new(ctx);

    if (_Z_RC_IS_NULL(&filter->ctx)) {
        _z_write_filter_ctx_clear(ctx);
        z_free(ctx);
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
    }
    _z_write_filter_ctx_rc_t filter_ctx_clone = _z_write_filter_ctx_rc_clone(&filter->ctx);
    _z_void_rc_t ctx_void = _z_write_filter_ctx_rc_to_void(&filter_ctx_clone);
    filter->_interest_id = _z_add_interest(_Z_RC_IN_VAL(zn), keyexpr, _z_write_filter_callback, flags, &ctx_void);
    if (filter->_interest_id == 0) {
        _z_write_filter_ctx_rc_drop(&filter->ctx);
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    _z_write_filter_session_register(_Z_RC_IN_VAL(zn), ctx, &filter->ctx);

    return _Z_RES_OK;
}

z_result_t _z_write_filter_ctx_clear(_z_write_filter_ctx_t *ctx) {
    z_result_t res = _Z_RES_OK;
    _z_write_filter_session_unregister(ctx);
    _z_write_filter_mutex_lock(ctx);
    _z_filter_target_slist_free(&ctx->targets);
#if Z_FEATURE_MATCHING
    _z_closure_matching_status_intmap_clear(&ctx->callbacks);
#endif
    _z_keyexpr_clear(&ctx->key);
    _z_session_weak_drop(&ctx->zn);
    _z_write_filter_mutex_unlock(ctx);

#if Z_FEATURE_MULTI_THREAD == 1
    _z_mutex_drop(&ctx->mutex);
#endif
    return res;
}

z_result_t _z_write_filter_clear(_z_write_filter_t *filter) {
    if (_Z_RC_IS_NULL(&filter->ctx)) {
        return _Z_RES_OK;
    }
    _z_write_filter_session_unregister(_Z_RC_IN_VAL(&filter->ctx));
    _z_session_rc_t s = _z_session_weak_upgrade(&_Z_RC_IN_VAL(&filter->ctx)->zn);
    if (!_Z_RC_IS_NULL(&s)) {
        _z_remove_interest(_Z_RC_IN_VAL(&s), filter->_interest_id);
        _z_session_rc_drop(&s);
    }
    _z_write_filter_ctx_rc_drop(&filter->ctx);
    return _Z_RES_OK;
}

#if Z_FEATURE_MATCHING == 1
void _z_write_filter_ctx_remove_callback(_z_write_filter_ctx_t *ctx, size_t id) {
    _z_write_filter_mutex_lock(ctx);
    _z_closure_matching_status_intmap_remove(&ctx->callbacks, id);
    _z_write_filter_mutex_unlock(ctx);
}

z_result_t _z_write_filter_ctx_add_callback(_z_write_filter_ctx_t *ctx, size_t id, _z_closure_matching_status_t *v) {
    _z_closure_matching_status_t *ptr = (_z_closure_matching_status_t *)z_malloc(sizeof(_z_closure_matching_status_t));
    if (ptr == NULL) {
        _z_closure_matching_status_clear(v);
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    *ptr = *v;
    *v = (_z_closure_matching_status_t){NULL, NULL, NULL};
    _z_write_filter_mutex_lock(ctx);
    if (!_z_write_filter_ctx_active(ctx)) {
        _z_matching_status_t s = (_z_matching_status_t){.matching = true};
        v->call(&s, v->context);
    }
    _z_closure_matching_status_intmap_insert(&ctx->callbacks, id, ptr);
    _z_write_filter_mutex_unlock(ctx);
    return _Z_RES_OK;
}
#endif

void _z_write_filter_notify_local_entity(_z_session_t *session, const _z_keyexpr_t *key, z_locality_t allowed_origin,
                                         bool add) {
#if Z_FEATURE_LOCAL_SUBSCRIBER == 1 || Z_FEATURE_LOCAL_QUERYABLE == 1
    if (!_z_locality_allows_local(allowed_origin)) {
        return;
    }

    _z_write_filter_ctx_rc_t *matches = NULL;
    size_t match_len = 0;

    _z_write_filter_registry_lock();
    _z_write_filter_session_entry_t *entry = _z_write_filter_get_session_entry(session, false);
    if (entry == NULL) {
        _z_write_filter_registry_unlock();
        return;
    }

    // Count matching write filters so we can allocate a buffer once.
    size_t match_count = 0;
    for (_z_write_filter_registration_t *registration = entry->registrations; registration != NULL;
         registration = registration->next) {
        _z_write_filter_ctx_t *registration_ctx = _Z_RC_IN_VAL(&registration->ctx_rc);
        if (registration_ctx->allow_local && _z_keyexpr_suffix_intersects(&registration_ctx->key, key)) {
            match_count++;
        }
    }

    if (match_count == 0) {
        _z_write_filter_registry_unlock();
        return;
    }

    // Allocate temporary buffer holding RC clones for using outside the registry lock.
    matches = (_z_write_filter_ctx_rc_t *)z_malloc(sizeof(_z_write_filter_ctx_rc_t) * match_count);
    if (matches == NULL) {
        _z_write_filter_registry_unlock();
        return;
    }

    for (_z_write_filter_registration_t *registration = entry->registrations; registration != NULL;
         registration = registration->next) {
        _z_write_filter_ctx_t *registration_ctx = _Z_RC_IN_VAL(&registration->ctx_rc);
        if (!registration_ctx->allow_local || !_z_keyexpr_suffix_intersects(&registration_ctx->key, key)) {
            continue;
        }
        matches[match_len] = _z_write_filter_ctx_rc_clone(&registration->ctx_rc);
        if (_Z_RC_IS_NULL(&matches[match_len])) {
            while (match_len > 0) {
                match_len--;
                _z_write_filter_ctx_rc_drop(&matches[match_len]);
            }
            z_free(matches);
            matches = NULL;
            _z_write_filter_registry_unlock();
            return;
        }
        match_len++;
    }
    _z_write_filter_registry_unlock();

    // Outside the registry lock: update each matching ctx and drop its clone.
    for (size_t i = 0; i < match_len; i++) {
        _z_write_filter_ctx_t *matched_ctx = _Z_RC_IN_VAL(&matches[i]);
        if (add) {
            _z_write_filter_ctx_add_local_match(matched_ctx);
        } else {
            _z_write_filter_ctx_remove_local_match(matched_ctx);
        }
        _z_write_filter_ctx_rc_drop(&matches[i]);
    }
    z_free(matches);
#else
    _ZP_UNUSED(session);
    _ZP_UNUSED(key);
    _ZP_UNUSED(allowed_origin);
    _ZP_UNUSED(add);
#endif
}

#else  // Z_FEATURE_INTEREST == 0
z_result_t _z_write_filter_create(const _z_session_rc_t *zn, _z_write_filter_t *filter, _z_keyexpr_t keyexpr,
                                  uint8_t interest_flag, bool complete, z_locality_t locality) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(keyexpr);
    _ZP_UNUSED(filter);
    _ZP_UNUSED(interest_flag);
    _ZP_UNUSED(complete);
    _ZP_UNUSED(locality);
    return _Z_RES_OK;
}

z_result_t _z_write_filter_clear(_z_write_filter_t *filter) {
    _ZP_UNUSED(filter);
    return _Z_RES_OK;
}

void _z_write_filter_notify_local_entity(_z_session_t *session, const _z_keyexpr_t *key, z_locality_t allowed_origin,
                                         bool add) {
    _ZP_UNUSED(session);
    _ZP_UNUSED(key);
    _ZP_UNUSED(allowed_origin);
    _ZP_UNUSED(add);
}

#endif
