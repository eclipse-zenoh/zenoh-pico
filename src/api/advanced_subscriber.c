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

#include "zenoh-pico/api/advanced_subscriber.h"

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/utils/query_params.h"
#include "zenoh-pico/utils/string.h"
#include "zenoh-pico/utils/time_range.h"

void ze_closure_miss_call(const ze_loaned_closure_miss_t *closure, const ze_miss_t *miss) {
    if (closure->call != NULL) {
        (closure->call)(miss, closure->context);
    }
}

#if Z_FEATURE_ADVANCED_SUBSCRIPTION == 1

#define ZE_ADVANCED_SUBSCRIBER_QUERY_PARAM_BUF_SIZE 256

_Z_OWNED_FUNCTIONS_CLOSURE_IMPL_PREFIX(ze, closure_miss, ze_closure_miss_callback_t, z_closure_drop_callback_t)

static void _ze_advanced_subscriber_sequenced_state_init(_ze_advanced_subscriber_sequenced_state_t *state) {
    state->_has_last_delivered = false;
    state->_last_delivered = 0;
    state->_pending_queries = 0;
    _z_uint32__z_sample_sortedmap_init(&state->_pending_samples);
}

void _ze_advanced_subscriber_sequenced_state_clear(_ze_advanced_subscriber_sequenced_state_t *state) {
    state->_has_last_delivered = false;
    state->_last_delivered = 0;
    state->_pending_queries = 0;
    _z_uint32__z_sample_sortedmap_clear(&state->_pending_samples);
}

void _ze_advanced_subscriber_sequenced_state_copy(_ze_advanced_subscriber_sequenced_state_t *dst,
                                                  const _ze_advanced_subscriber_sequenced_state_t *src) {
    dst->_has_last_delivered = src->_has_last_delivered;
    dst->_last_delivered = src->_last_delivered;
    dst->_pending_queries = src->_pending_queries;
    _z_uint32__z_sample_sortedmap_copy(&dst->_pending_samples, &src->_pending_samples);
}

static void _ze_advanced_subscriber_timestamped_state_init(_ze_advanced_subscriber_timestamped_state_t *state) {
    state->_has_last_delivered = false;
    state->_last_delivered = _z_timestamp_null();
    state->_pending_queries = 0;
    _z_timestamp__z_sample_sortedmap_init(&state->_pending_samples);
}

void _ze_advanced_subscriber_timestamped_state_clear(_ze_advanced_subscriber_timestamped_state_t *state) {
    state->_has_last_delivered = false;
    _z_timestamp_clear(&state->_last_delivered);
    state->_pending_queries = 0;
    _z_timestamp__z_sample_sortedmap_clear(&state->_pending_samples);
}

void _ze_advanced_subscriber_timestamped_state_copy(_ze_advanced_subscriber_timestamped_state_t *dst,
                                                    const _ze_advanced_subscriber_timestamped_state_t *src) {
    dst->_has_last_delivered = src->_has_last_delivered;
    dst->_last_delivered = src->_last_delivered;
    dst->_pending_queries = src->_pending_queries;
    _z_timestamp__z_sample_sortedmap_copy(&dst->_pending_samples, &src->_pending_samples);
}

_ze_advanced_subscriber_state_t _ze_advanced_subscriber_state_null(void) {
    _ze_advanced_subscriber_state_t state = {0};
    state._zn = _z_session_weak_null();
    z_internal_keyexpr_null(&state._key_expr);
    z_internal_keyexpr_null(&state._query_key_expr);
    z_internal_liveliness_token_null(&state._token);
    return state;
}

static z_result_t _ze_advanced_subscriber_state_init(_ze_advanced_subscriber_state_t *state, const _z_session_rc_t *zn,
                                                     z_moved_closure_sample_t *callback,
                                                     const z_loaned_keyexpr_t *keyexpr,
                                                     const z_loaned_keyexpr_t *query_keyexpr,
                                                     const ze_advanced_subscriber_options_t *options) {
    _Z_RETURN_IF_ERR(_z_mutex_init(&state->_mutex));
    state->_next_id = 0;
    state->_global_pending_queries = options->history.is_enabled ? 1 : 0;
    _z_entity_global_id__ze_advanced_subscriber_sequenced_state_hashmap_init(&state->_sequenced_states);
    _z_id__ze_advanced_subscriber_timestamped_state_hashmap_init(&state->_timestamped_states);
    state->_zn = _z_session_rc_clone_as_weak(zn);
    z_internal_keyexpr_null(&state->_key_expr);
    z_internal_keyexpr_null(&state->_query_key_expr);
    _Z_CLEAN_RETURN_IF_ERR(z_keyexpr_clone(&state->_key_expr, keyexpr), _z_mutex_drop(&state->_mutex));
    _Z_CLEAN_RETURN_IF_ERR(z_keyexpr_clone(&state->_query_key_expr, query_keyexpr),
                           z_keyexpr_drop(z_keyexpr_move(&state->_key_expr));
                           _z_mutex_drop(&state->_mutex));
    state->_retransmission = options->recovery.is_enabled;
    state->_has_period = options->recovery.last_sample_miss_detection.periodic_queries_period_ms != 0;
    // TODO: Set Period
    state->_history_depth = 0;
    state->_query_target = Z_QUERY_TARGET_DEFAULT;
    // Use default query timeout if not specified in options
    state->_query_timeout = (options->query_timeout_ms > 0) ? options->query_timeout_ms : Z_GET_TIMEOUT_DEFAULT;
    state->_callback = callback->_this._val.call;
    state->_dropper = callback->_this._val.drop;
    state->_ctx = callback->_this._val.context;
    z_internal_closure_sample_null(&callback->_this);
    // TODO: Init miss hander map
    state->_has_token = false;
    z_internal_liveliness_token_null(&state->_token);

    return _Z_RES_OK;
}

static z_result_t _ze_advanced_subscriber_state_new(_ze_advanced_subscriber_state_simple_rc_t *rc_state,
                                                    const _z_session_rc_t *zn, z_moved_closure_sample_t *callback,
                                                    const z_loaned_keyexpr_t *keyexpr,
                                                    const z_loaned_keyexpr_t *query_keyexpr,
                                                    const ze_advanced_subscriber_options_t *options) {
    _ze_advanced_subscriber_state_t state;
    _Z_RETURN_IF_ERR(_ze_advanced_subscriber_state_init(&state, zn, callback, keyexpr, query_keyexpr, options));

    *rc_state = _ze_advanced_subscriber_state_simple_rc_new_from_val(&state);
    if (_Z_SIMPLE_RC_IS_NULL(rc_state)) {
        _ze_advanced_subscriber_state_clear(&state);
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }

    return _Z_RES_OK;
}

void _ze_advanced_subscriber_state_clear(_ze_advanced_subscriber_state_t *state) {
    _z_entity_global_id__ze_advanced_subscriber_sequenced_state_hashmap_clear(&state->_sequenced_states);
    _z_id__ze_advanced_subscriber_timestamped_state_hashmap_clear(&state->_timestamped_states);
    z_keyexpr_drop(z_keyexpr_move(&state->_key_expr));
    z_keyexpr_drop(z_keyexpr_move(&state->_query_key_expr));
    if (state->_has_token) {
        z_liveliness_token_drop(z_liveliness_token_move(&state->_token));
    }
    _z_mutex_drop(&state->_mutex);
    _z_session_weak_drop(&state->_zn);

    *state = _ze_advanced_subscriber_state_null();
}

static bool _ze_advanced_subscriber_state_check(const _ze_advanced_subscriber_state_t *state) {
    return (!_Z_RC_IS_NULL(&state->_zn) && z_internal_keyexpr_check(&state->_key_expr) &&
            z_internal_keyexpr_check(&state->_query_key_expr) &&
            (!state->_has_token || z_internal_liveliness_token_check(&state->_token)));
}

bool _ze_advanced_subscriber_check(const _ze_advanced_subscriber_t *sub) {
    return (z_internal_subscriber_check(&sub->_subscriber) &&
            (!sub->_has_liveliness_subscriber || z_internal_subscriber_check(&sub->_liveliness_subscriber)) &&
            (!sub->_has_heartbeat_subscriber || z_internal_subscriber_check(&sub->_heartbeat_subscriber)) &&
            !_Z_SIMPLE_RC_IS_NULL(&sub->_state) &&
            _ze_advanced_subscriber_state_check(_ze_advanced_subscriber_state_simple_rc_value(&sub->_state)));
}

_ze_advanced_subscriber_t _ze_advanced_subscriber_null(void) {
    _ze_advanced_subscriber_t subscriber = {0};
    z_internal_subscriber_null(&subscriber._subscriber);
    z_internal_subscriber_null(&subscriber._liveliness_subscriber);
    z_internal_subscriber_null(&subscriber._heartbeat_subscriber);
    subscriber._state = _ze_advanced_subscriber_state_simple_rc_null();

    return subscriber;
}

z_result_t _ze_advanced_subscriber_clear(_ze_advanced_subscriber_t *sub) {
    _z_subscriber_clear(&sub->_subscriber._val);
    _z_subscriber_clear(&sub->_liveliness_subscriber._val);
    _z_subscriber_clear(&sub->_heartbeat_subscriber._val);
    _ze_advanced_subscriber_state_simple_rc_drop(&sub->_state);

    *sub = _ze_advanced_subscriber_null();
    return _Z_RES_OK;
}

z_result_t _ze_advanced_subscriber_drop(_ze_advanced_subscriber_t *sub) {
    if (sub == NULL || !_ze_advanced_subscriber_check(sub)) {
        return _Z_ERR_ENTITY_UNKNOWN;
    }

    z_result_t ret = z_undeclare_subscriber(z_subscriber_move(&sub->_subscriber));
    if (sub->_has_liveliness_subscriber) {
        z_undeclare_subscriber(z_subscriber_move(&sub->_liveliness_subscriber));
    }
    if (sub->_has_heartbeat_subscriber) {
        z_undeclare_subscriber(z_subscriber_move(&sub->_heartbeat_subscriber));
    }
    _ze_advanced_subscriber_state_simple_rc_drop(&sub->_state);

    *sub = _ze_advanced_subscriber_null();
    return ret;
}

_Z_OWNED_FUNCTIONS_VALUE_NO_COPY_NO_MOVE_IMPL_PREFIX(ze, _ze_advanced_subscriber_t, advanced_subscriber,
                                                     _ze_advanced_subscriber_check, _ze_advanced_subscriber_null,
                                                     _ze_advanced_subscriber_drop)

static bool _ze_advanced_subscriber_populate_query_params(char *buf, size_t buf_len, size_t max_samples,
                                                          uint64_t max_age_ms,
                                                          const _z_query_param_range_t *seq_range) {
    if (buf == NULL || buf_len == 0) {
        return false;
    }

    size_t pos = 0;
    // Allow responses for any key expression
    if (!_z_memcpy_checked(buf, buf_len, &pos, _Z_QUERY_PARAMS_KEY_ANYKE, _Z_QUERY_PARAMS_KEY_ANYKE_LEN)) {
        return false;  // Not enough space
    }

    if (max_samples > 0) {
        if (!_z_memcpy_checked(buf, buf_len, &pos, _Z_QUERY_PARAMS_LIST_SEPARATOR,
                               _Z_QUERY_PARAMS_LIST_SEPARATOR_LEN)) {
            return false;  // Not enough space
        }
        if (!_z_memcpy_checked(buf, buf_len, &pos, _Z_QUERY_PARAMS_KEY_MAX, _Z_QUERY_PARAMS_KEY_MAX_LEN)) {
            return false;  // Not enough space
        }
        if (!_z_memcpy_checked(buf, buf_len, &pos, _Z_QUERY_PARAMS_FIELD_SEPARATOR,
                               _Z_QUERY_PARAMS_FIELD_SEPARATOR_LEN)) {
            return false;  // Not enough space
        }

        int written = snprintf(&buf[pos], buf_len - pos, "%zu", max_samples);
        if (written < 0 || (size_t)written >= buf_len - pos) {
            return false;  // Overflow or encoding error
        }
        pos += (size_t)written;
    }
    if (max_age_ms > 0) {
        if (!_z_memcpy_checked(buf, buf_len, &pos, _Z_QUERY_PARAMS_LIST_SEPARATOR,
                               _Z_QUERY_PARAMS_LIST_SEPARATOR_LEN)) {
            return false;  // Not enough space
        }
        if (!_z_memcpy_checked(buf, buf_len, &pos, _Z_QUERY_PARAMS_KEY_TIME, _Z_QUERY_PARAMS_KEY_TIME_LEN)) {
            return false;  // Not enough space
        }
        if (!_z_memcpy_checked(buf, buf_len, &pos, _Z_QUERY_PARAMS_FIELD_SEPARATOR,
                               _Z_QUERY_PARAMS_FIELD_SEPARATOR_LEN)) {
            return false;  // Not enough space
        }

        double max_age_s = (double)max_age_ms * _Z_TIME_RANGE_MS_TO_SECS;
        _z_time_range_t range = {.start = {.bound = _Z_TIME_BOUND_INCLUSIVE, .now_offset = -max_age_s},
                                 .end = {.bound = _Z_TIME_BOUND_UNBOUNDED}};
        if (!_z_time_range_to_str(&range, &buf[pos], buf_len - pos)) {
            return false;  // Not enough space or invalid range
        }
        size_t used = strnlen(&buf[pos], buf_len - pos);
        if (used == buf_len - pos) {
            // Null terminator not found in remaining space
            return false;
        }
        pos += used;
    }
    if (seq_range != NULL) {
        if (!_z_memcpy_checked(buf, buf_len, &pos, _Z_QUERY_PARAMS_LIST_SEPARATOR,
                               _Z_QUERY_PARAMS_LIST_SEPARATOR_LEN)) {
            return false;  // Not enough space
        }
        if (!_z_memcpy_checked(buf, buf_len, &pos, _Z_QUERY_PARAMS_KEY_RANGE, _Z_QUERY_PARAMS_KEY_RANGE_LEN)) {
            return false;  // Not enough space
        }
        if (!_z_memcpy_checked(buf, buf_len, &pos, _Z_QUERY_PARAMS_FIELD_SEPARATOR,
                               _Z_QUERY_PARAMS_FIELD_SEPARATOR_LEN)) {
            return false;  // Not enough space
        }

        if (seq_range->_has_start) {
            int written = snprintf(&buf[pos], buf_len - pos, "%u", seq_range->_start);
            if (written < 0 || (size_t)written >= buf_len - pos) {
                return false;  // Overflow or encoding error
            }
            pos += (size_t)written;
        }

        if (buf_len - pos < 2) {
            return false;  // Not enough room for ".."
        }
        buf[pos++] = '.';
        buf[pos++] = '.';

        if (seq_range->_has_end) {
            int written = snprintf(&buf[pos], buf_len - pos, "%u", seq_range->_end);
            if (written < 0 || (size_t)written >= buf_len - pos) {
                return false;  // Overflow or encoding error
            }
            pos += (size_t)written;
        }
    }

    if (pos >= buf_len) {
        return false;  // Not enough space for '\0'
    }
    buf[pos] = '\0';
    return true;
}

// SAFETY: Must be called with _ze_advanced_subscriber_state_t mutex locked
static inline void __unsafe_ze_advanced_subscriber_deliver_and_flush(_z_sample_t *sample, uint32_t source_sn,
                                                                     _z_closure_sample_callback_t callback, void *ctx,
                                                                     _ze_advanced_subscriber_sequenced_state_t *state) {
    if (callback != NULL) {
        callback(sample, ctx);
    }
    state->_last_delivered = source_sn;
    state->_has_last_delivered = true;

    uint32_t next_source_sn = source_sn + 1;
    _z_sample_t *next_sample = _z_uint32__z_sample_sortedmap_get(&state->_pending_samples, &next_source_sn);
    while (next_sample != NULL) {
        if (callback != NULL) {
            callback(next_sample, ctx);
        }
        _z_uint32__z_sample_sortedmap_remove(&state->_pending_samples, &next_source_sn);
        state->_last_delivered = next_source_sn++;
        next_sample = _z_uint32__z_sample_sortedmap_get(&state->_pending_samples, &next_source_sn);
    }
}

// SAFETY: Must be called with _ze_advanced_subscriber_state_t mutex locked
static inline void __unsafe_ze_advanced_subscriber_flush_sequenced_source(
    _ze_advanced_subscriber_sequenced_state_t *state, _z_closure_sample_callback_t callback, void *ctx,
    const _z_entity_global_id_t *source_id) {
    (void)(source_id);

    if (state->_pending_queries == 0 && !_z_uint32__z_sample_sortedmap_is_empty(&state->_pending_samples)) {
        _z_uint32__z_sample_sortedmap_iterator_t it =
            _z_uint32__z_sample_sortedmap_iterator_make(&state->_pending_samples);

        while (_z_uint32__z_sample_sortedmap_iterator_next(&it)) {
            uint32_t *source_sn = _z_uint32__z_sample_sortedmap_iterator_key(&it);
            _z_sample_t *sample = _z_uint32__z_sample_sortedmap_iterator_value(&it);

            if (!state->_has_last_delivered) {
                state->_last_delivered = *source_sn;
                state->_has_last_delivered = true;
                if (callback != NULL) {
                    callback(sample, ctx);
                }
            } else {
                if (*source_sn == state->_last_delivered + 1) {
                    state->_last_delivered = *source_sn;
                    if (callback != NULL) {
                        callback(sample, ctx);
                    }
                } else if (*source_sn > state->_last_delivered + 1) {
                    // TODO: Call miss handlers
                    state->_last_delivered = *source_sn;
                    if (callback != NULL) {
                        callback(sample, ctx);
                    }
                }  // else duplicate sample
            }
        }
        _z_uint32__z_sample_sortedmap_clear(&state->_pending_samples);
    }
}

// SAFETY: Must be called with _ze_advanced_subscriber_state_t mutex locked
static inline void __unsafe_ze_advanced_subscriber_flush_timestamped_source(
    _ze_advanced_subscriber_timestamped_state_t *state, _z_closure_sample_callback_t callback, void *ctx) {
    if (state->_pending_queries == 0 && !_z_timestamp__z_sample_sortedmap_is_empty(&state->_pending_samples)) {
        _z_timestamp__z_sample_sortedmap_iterator_t it =
            _z_timestamp__z_sample_sortedmap_iterator_make(&state->_pending_samples);

        while (_z_timestamp__z_sample_sortedmap_iterator_next(&it)) {
            _z_timestamp_t *timestamp = _z_timestamp__z_sample_sortedmap_iterator_key(&it);
            _z_sample_t *sample = _z_timestamp__z_sample_sortedmap_iterator_value(&it);

            if (!state->_has_last_delivered || _z_timestamp_cmp(timestamp, &state->_last_delivered) > 0) {
                _z_timestamp_copy(&state->_last_delivered, timestamp);
                state->_has_last_delivered = true;
                if (callback != NULL) {
                    callback(sample, ctx);
                }
            }
        }
        _z_timestamp__z_sample_sortedmap_clear(&state->_pending_samples);
    }
}

// SAFETY: Must be called with _ze_advanced_subscriber_state_t mutex locked
// Returns true for new sequenced sources, false otherwise
static bool __unsafe_ze_advanced_subscriber_handle_sample(_ze_advanced_subscriber_state_t *states,
                                                          z_loaned_sample_t *sample) {
    const z_loaned_source_info_t *source_info = z_sample_source_info(sample);
    const z_timestamp_t *timestamp = z_sample_timestamp(sample);

    if (_z_source_info_check(source_info)) {
        bool new_source = false;
        z_entity_global_id_t id = z_source_info_id(source_info);
        uint32_t source_sn = z_source_info_sn(source_info);
        _ze_advanced_subscriber_sequenced_state_t *state =
            _z_entity_global_id__ze_advanced_subscriber_sequenced_state_hashmap_get(&states->_sequenced_states, &id);
        if (state == NULL) {
            new_source = true;

            z_entity_global_id_t *new_id = z_malloc(sizeof(z_entity_global_id_t));
            if (new_id == NULL) {
                _Z_ERROR("Failed to allocate memory for new sequenced state ID");
                return false;
            }
            *new_id = id;

            _ze_advanced_subscriber_sequenced_state_t *new_state =
                z_malloc(sizeof(_ze_advanced_subscriber_sequenced_state_t));
            if (new_state == NULL) {
                _Z_ERROR("Failed to allocate memory for new sequenced state");
                z_free(new_id);
                return false;
            }
            _ze_advanced_subscriber_sequenced_state_init(new_state);
            state = _z_entity_global_id__ze_advanced_subscriber_sequenced_state_hashmap_insert(
                &states->_sequenced_states, new_id, new_state);
            if (state == NULL) {
                _Z_ERROR("Failed to insert new sequenced state into hashmap");
                z_free(new_id);
                z_free(new_state);
                return false;
            }
        }
        if (!state->_has_last_delivered && states->_global_pending_queries != 0) {
            // Avoid going through the map if history_depth == 1
            if (states->_history_depth == 1) {
                state->_last_delivered = source_sn;
                state->_has_last_delivered = true;
                if (states->_callback != NULL) {
                    states->_callback(sample, states->_ctx);
                }
            } else {
                uint32_t *new_source_sn = z_malloc(sizeof(uint32_t));
                if (new_source_sn == NULL) {
                    _Z_ERROR("Failed to allocate memory for new source_sn");
                    return false;
                }
                *new_source_sn = source_sn;

                _z_sample_t *new_sample = z_malloc(sizeof(_z_sample_t));
                if (new_sample == NULL) {
                    _Z_ERROR("Failed to allocate memory for new sample");
                    z_free(new_source_sn);
                    return false;
                }
                if (_z_sample_copy(new_sample, sample) != _Z_RES_OK) {
                    _Z_ERROR("Failed to copy sample");
                    z_free(new_source_sn);
                    z_free(new_sample);
                    return false;
                }

                _z_sample_t *entry =
                    _z_uint32__z_sample_sortedmap_insert(&state->_pending_samples, new_source_sn, new_sample);
                if (entry == NULL) {
                    _Z_ERROR("Failed to insert sample into sequenced state");
                    z_free(new_source_sn);
                    z_free(new_sample);
                    return false;
                }

                if (_z_uint32__z_sample_sortedmap_len(&state->_pending_samples) >= states->_history_depth) {
                    _z_uint32__z_sample_sortedmap_entry_t *first_entry =
                        _z_uint32__z_sample_sortedmap_pop_first(&state->_pending_samples);
                    if (first_entry != NULL) {
                        uint32_t *first_source_sn = _z_uint32__z_sample_sortedmap_entry_key(first_entry);
                        _z_sample_t *first_sample = _z_uint32__z_sample_sortedmap_entry_val(first_entry);
                        __unsafe_ze_advanced_subscriber_deliver_and_flush(first_sample, *first_source_sn,
                                                                          states->_callback, states->_ctx, state);
                        _z_uint32__z_sample_sortedmap_entry_free(&first_entry);
                    }
                }
            }
        } else if (state->_has_last_delivered && source_sn != state->_last_delivered + 1) {
            if (source_sn > state->_last_delivered) {
                if (states->_retransmission) {
                    uint32_t *new_source_sn = z_malloc(sizeof(uint32_t));
                    if (new_source_sn == NULL) {
                        _Z_ERROR("Failed to allocate memory for new source_sn");
                        return false;
                    }
                    *new_source_sn = source_sn;

                    _z_sample_t *new_sample = z_malloc(sizeof(_z_sample_t));
                    if (new_sample == NULL) {
                        _Z_ERROR("Failed to allocate memory for new sample");
                        z_free(new_source_sn);
                        return false;
                    }
                    if (_z_sample_copy(new_sample, sample) != _Z_RES_OK) {
                        _Z_ERROR("Failed to copy sample");
                        z_free(new_source_sn);
                        z_free(new_sample);
                        return false;
                    }

                    _z_sample_t *entry =
                        _z_uint32__z_sample_sortedmap_insert(&state->_pending_samples, new_source_sn, new_sample);
                    if (entry == NULL) {
                        _Z_ERROR("Failed to insert sample into sequenced state");
                        z_free(new_source_sn);
                        z_free(new_sample);
                        return false;
                    }
                } else {
                    // TODO: Trigger missed sample callbacks
                    if (states->_callback != NULL) {
                        states->_callback(sample, states->_ctx);
                    }
                    state->_last_delivered = source_sn;
                }
            }
        } else {
            __unsafe_ze_advanced_subscriber_deliver_and_flush(sample, source_sn, states->_callback, states->_ctx,
                                                              state);
        }
        return new_source;
    } else if (timestamp != NULL) {
        z_id_t id = z_timestamp_id(timestamp);
        _ze_advanced_subscriber_timestamped_state_t *state =
            _z_id__ze_advanced_subscriber_timestamped_state_hashmap_get(&states->_timestamped_states, &id);
        if (state == NULL) {
            z_id_t *new_id = z_malloc(sizeof(z_id_t));
            if (new_id == NULL) {
                _Z_ERROR("Failed to allocate memory for new timestamped state ID");
                return false;
            }
            *new_id = id;
            _ze_advanced_subscriber_timestamped_state_t *new_state =
                z_malloc(sizeof(_ze_advanced_subscriber_timestamped_state_t));
            if (new_state == NULL) {
                _Z_ERROR("Failed to allocate memory for new timestamped state");
                z_free(new_id);
                return false;
            }
            _ze_advanced_subscriber_timestamped_state_init(new_state);
            state = _z_id__ze_advanced_subscriber_timestamped_state_hashmap_insert(&states->_timestamped_states, new_id,
                                                                                   new_state);
            if (state == NULL) {
                _Z_ERROR("Failed to insert new timestamped state into hashmap");
                z_free(new_id);
                z_free(new_state);
                return false;
            }
        }
        if (!state->_has_last_delivered || _z_timestamp_cmp(&state->_last_delivered, timestamp) < 0) {
            if ((states->_global_pending_queries == 0 && state->_pending_queries == 0) || states->_history_depth == 1) {
                state->_last_delivered = *timestamp;
                state->_has_last_delivered = true;
                if (states->_callback != NULL) {
                    states->_callback(sample, states->_ctx);
                }
            } else {
                _z_sample_t *entry = _z_timestamp__z_sample_sortedmap_get(&state->_pending_samples, timestamp);
                if (entry == NULL) {
                    z_timestamp_t *new_timestamp = z_malloc(sizeof(z_timestamp_t));
                    if (new_timestamp == NULL) {
                        _Z_ERROR("Failed to allocate memory for new timestamp");
                        return false;
                    }
                    _z_timestamp_copy(new_timestamp, timestamp);

                    _z_sample_t *new_sample = z_malloc(sizeof(_z_sample_t));
                    if (new_sample == NULL) {
                        _Z_ERROR("Failed to allocate memory for new sample");
                        z_free(new_timestamp);
                        return false;
                    }
                    if (_z_sample_copy(new_sample, sample) != _Z_RES_OK) {
                        _Z_ERROR("Failed to copy sample");
                        z_free(new_timestamp);
                        z_free(new_sample);
                        return false;
                    }

                    entry =
                        _z_timestamp__z_sample_sortedmap_insert(&state->_pending_samples, new_timestamp, new_sample);
                    if (entry == NULL) {
                        _Z_ERROR("Failed to insert sample into timestamped state");
                        z_free(new_timestamp);
                        z_free(new_sample);
                        return false;
                    }
                }
                if (_z_timestamp__z_sample_sortedmap_len(&state->_pending_samples) >= states->_history_depth) {
                    __unsafe_ze_advanced_subscriber_flush_timestamped_source(state, states->_callback, states->_ctx);
                }
            }
        }
        return false;
    } else {
        if (states->_callback != NULL) {
            states->_callback(sample, states->_ctx);
        }
        return false;
    }
}

typedef enum {
    _ZE_ADVANCED_SUBSCRIBER_QUERY_CTX_INITIAL,
    _ZE_ADVANCED_SUBSCRIBER_QUERY_CTX_SEQUENCED,
    _ZE_ADVANCED_SUBSCRIBER_QUERY_CTX_TIMESTAMPED
} _ze_advanced_subscriber_query_ctx_kind_t;

typedef struct {
    _ze_advanced_subscriber_state_simple_rc_t _statesref;
    _z_condvar_t _condvar;
    _ze_advanced_subscriber_query_ctx_kind_t _kind;

    union {
        z_entity_global_id_t _source_id;
        z_id_t _id;
    };
} _ze_advanced_subscriber_query_ctx_t;

static inline _ze_advanced_subscriber_query_ctx_t _ze_advanced_subscriber_query_ctx_null(void) {
    _ze_advanced_subscriber_query_ctx_t ctx = {0};
    ctx._statesref = _ze_advanced_subscriber_state_simple_rc_null();
    return ctx;
}

void _ze_advanced_subscriber_query_reply_handler(z_loaned_reply_t *reply, void *ctx) {
    if (!z_reply_is_ok(reply)) {
        const z_loaned_reply_err_t *err = z_reply_err(reply);
        z_owned_string_t errstr;
        z_bytes_to_string(z_reply_err_payload(err), &errstr);
        _Z_ERROR("Failed to query samples: %.*s", (int)z_string_len(z_string_loan(&errstr)),
                 z_string_data(z_string_loan(&errstr)));
        z_string_drop(z_string_move(&errstr));
        return;
    }

    _ze_advanced_subscriber_query_ctx_t *query_ctx = (_ze_advanced_subscriber_query_ctx_t *)ctx;

    if (!_Z_SIMPLE_RC_IS_NULL(&query_ctx->_statesref)) {
        _ze_advanced_subscriber_state_t *states = _ze_advanced_subscriber_state_simple_rc_value(&query_ctx->_statesref);

        const z_loaned_sample_t *sample = z_reply_ok(reply);
        const z_loaned_keyexpr_t *keyexpr = z_sample_keyexpr(sample);

        if (z_keyexpr_intersects(z_keyexpr_loan(&states->_key_expr), keyexpr)) {
            if (_z_mutex_lock(&states->_mutex) == _Z_RES_OK) {
                z_owned_sample_t sample_copy;
                if (z_sample_clone(&sample_copy, sample) == _Z_RES_OK) {
                    __unsafe_ze_advanced_subscriber_handle_sample(states, z_sample_loan_mut(&sample_copy));
                    z_sample_drop(z_sample_move(&sample_copy));
                } else {
                    _Z_ERROR("Failed to clone sample for query reply handling");
                }

                _z_mutex_unlock(&states->_mutex);
            } else {
                _Z_ERROR("Failed to lock mutex for query reply handling");
            }
        }
    }
}

// SAFETY: Must be called with _ze_advanced_subscriber_state_t mutex locked
void __unsafe_ze_advanced_subscriber_initial_query_drop_handler(_ze_advanced_subscriber_state_t *states) {
    states->_global_pending_queries = (states->_global_pending_queries > 0) ? (states->_global_pending_queries - 1) : 0;
    if (states->_global_pending_queries == 0) {
        for (_z_entity_global_id__ze_advanced_subscriber_sequenced_state_hashmap_iterator_t it =
                 _z_entity_global_id__ze_advanced_subscriber_sequenced_state_hashmap_iterator_make(
                     &states->_sequenced_states);
             _z_entity_global_id__ze_advanced_subscriber_sequenced_state_hashmap_iterator_next(&it);) {
            _z_entity_global_id_t *source_id =
                _z_entity_global_id__ze_advanced_subscriber_sequenced_state_hashmap_iterator_key(&it);
            _ze_advanced_subscriber_sequenced_state_t *sequenced_state =
                _z_entity_global_id__ze_advanced_subscriber_sequenced_state_hashmap_iterator_value(&it);

            __unsafe_ze_advanced_subscriber_flush_sequenced_source(sequenced_state, states->_callback, states->_ctx,
                                                                   source_id);
            // TODO: Spawn periodic queries
        }
        for (_z_id__ze_advanced_subscriber_timestamped_state_hashmap_iterator_t it =
                 _z_id__ze_advanced_subscriber_timestamped_state_hashmap_iterator_make(&states->_timestamped_states);
             _z_id__ze_advanced_subscriber_timestamped_state_hashmap_iterator_next(&it);) {
            _ze_advanced_subscriber_timestamped_state_t *timestamped_state =
                _z_id__ze_advanced_subscriber_timestamped_state_hashmap_iterator_value(&it);

            __unsafe_ze_advanced_subscriber_flush_timestamped_source(timestamped_state, states->_callback,
                                                                     states->_ctx);
        }
    }
}

// SAFETY: Must be called with _ze_advanced_subscriber_state_t mutex locked
void __unsafe_ze_advanced_subscriber_sequenced_query_drop_handler(_ze_advanced_subscriber_state_t *states,
                                                                  const z_entity_global_id_t *source_id) {
    _ze_advanced_subscriber_sequenced_state_t *state =
        _z_entity_global_id__ze_advanced_subscriber_sequenced_state_hashmap_get(&states->_sequenced_states, source_id);
    if (state != NULL) {
        state->_pending_queries = (state->_pending_queries > 0) ? (state->_pending_queries - 1) : 0;
        if (states->_global_pending_queries == 0) {
            __unsafe_ze_advanced_subscriber_flush_sequenced_source(state, states->_callback, states->_ctx, source_id);
        }
    }
}

// SAFETY: Must be called with _ze_advanced_subscriber_state_t mutex locked
void __unsafe_ze_advanced_subscriber_timestamped_query_drop_handler(_ze_advanced_subscriber_state_t *states,
                                                                    const z_id_t *id) {
    _ze_advanced_subscriber_timestamped_state_t *state =
        _z_id__ze_advanced_subscriber_timestamped_state_hashmap_get(&states->_timestamped_states, id);
    if (state != NULL) {
        state->_pending_queries = (state->_pending_queries > 0) ? (state->_pending_queries - 1) : 0;
        if (states->_global_pending_queries == 0) {
            __unsafe_ze_advanced_subscriber_flush_timestamped_source(state, states->_callback, states->_ctx);
        }
    }
}

void _ze_advanced_subscriber_query_drop_handler(void *ctx) {
    _ze_advanced_subscriber_query_ctx_t *query_ctx = (_ze_advanced_subscriber_query_ctx_t *)ctx;

    if (!_Z_SIMPLE_RC_IS_NULL(&query_ctx->_statesref)) {
        _ze_advanced_subscriber_state_t *states = _ze_advanced_subscriber_state_simple_rc_value(&query_ctx->_statesref);
        if (_z_mutex_lock(&states->_mutex) == _Z_RES_OK) {
            switch (query_ctx->_kind) {
                case _ZE_ADVANCED_SUBSCRIBER_QUERY_CTX_INITIAL:
                    __unsafe_ze_advanced_subscriber_initial_query_drop_handler(states);
                    break;
                case _ZE_ADVANCED_SUBSCRIBER_QUERY_CTX_SEQUENCED:
                    __unsafe_ze_advanced_subscriber_sequenced_query_drop_handler(states, &query_ctx->_source_id);
                    break;
                case _ZE_ADVANCED_SUBSCRIBER_QUERY_CTX_TIMESTAMPED:
                    __unsafe_ze_advanced_subscriber_timestamped_query_drop_handler(states, &query_ctx->_id);
                    break;
                default:
                    _Z_ERROR("Drop handler called for unknown query kind.");
            };
            _z_condvar_signal(&query_ctx->_condvar);
            _z_mutex_unlock(&states->_mutex);
        }
        _ze_advanced_subscriber_state_simple_rc_drop(&query_ctx->_statesref);
    }
}

static z_result_t _ze_advanced_subscriber_run_query(_ze_advanced_subscriber_query_ctx_t *ctx,
                                                    _ze_advanced_subscriber_state_simple_rc_t *rc_state,
                                                    const z_loaned_keyexpr_t *keyexpr, const char *params) {
    if (_Z_SIMPLE_RC_IS_NULL(rc_state)) {
        _Z_ERROR("Failed to run query - state is NULL");
        return _Z_ERR_GENERIC;
    }
    _ze_advanced_subscriber_state_t *state = _ze_advanced_subscriber_state_simple_rc_value(rc_state);
    _ze_advanced_subscriber_state_simple_rc_copy(&ctx->_statesref, rc_state);

    _Z_CLEAN_RETURN_IF_ERR(_z_condvar_init(&ctx->_condvar),
                           _ze_advanced_subscriber_state_simple_rc_drop(&ctx->_statesref));

    z_owned_closure_reply_t callback;
    z_closure_reply(&callback, _ze_advanced_subscriber_query_reply_handler, _ze_advanced_subscriber_query_drop_handler,
                    ctx);

    _Z_CLEAN_RETURN_IF_ERR(_z_mutex_lock(&state->_mutex),
                           _ze_advanced_subscriber_state_simple_rc_drop(&ctx->_statesref);
                           _z_condvar_drop(&ctx->_condvar));

    z_get_options_t get_opts;
    z_get_options_default(&get_opts);
    get_opts.consolidation.mode = Z_CONSOLIDATION_MODE_NONE;
    get_opts.target = Z_QUERY_TARGET_ALL;
    get_opts.timeout_ms = state->_query_timeout;

#if Z_FEATURE_SESSION_CHECK == 1
    _z_session_rc_t sess_rc = _z_session_weak_upgrade_if_open(&state->_zn);
#else
    _z_session_rc_t sess_rc = _z_session_weak_upgrade(&state->_zn);
#endif
    if (_Z_RC_IS_NULL(&sess_rc)) {
        _z_mutex_unlock(&state->_mutex);
        _ze_advanced_subscriber_state_simple_rc_drop(&ctx->_statesref);
        _z_condvar_drop(&ctx->_condvar);
        return _Z_ERR_SESSION_CLOSED;
    }
    z_result_t res = z_get(&sess_rc, keyexpr, params, z_closure_reply_move(&callback), &get_opts);
    _z_session_rc_drop(&sess_rc);
    if (res != _Z_RES_OK) {
        _z_mutex_unlock(&state->_mutex);
        _ze_advanced_subscriber_state_simple_rc_drop(&ctx->_statesref);
        _z_condvar_drop(&ctx->_condvar);
        return res;
    }

    res = _z_condvar_wait(&ctx->_condvar, &state->_mutex);
    _z_condvar_drop(&ctx->_condvar);
    _z_mutex_unlock(&state->_mutex);
    return res;
}

static inline z_result_t _ze_advanced_subscriber_initial_query(_ze_advanced_subscriber_state_simple_rc_t *state,
                                                               const z_loaned_keyexpr_t *keyexpr, const char *params) {
    _ze_advanced_subscriber_query_ctx_t ctx = _ze_advanced_subscriber_query_ctx_null();
    ctx._kind = _ZE_ADVANCED_SUBSCRIBER_QUERY_CTX_INITIAL;

    return _ze_advanced_subscriber_run_query(&ctx, state, keyexpr, params);
}

static inline z_result_t _ze_advanced_subscriber_sequenced_query(_ze_advanced_subscriber_state_simple_rc_t *state,
                                                                 const z_loaned_keyexpr_t *keyexpr, const char *params,
                                                                 const z_entity_global_id_t *source_id) {
    _ze_advanced_subscriber_query_ctx_t ctx = _ze_advanced_subscriber_query_ctx_null();
    ctx._kind = _ZE_ADVANCED_SUBSCRIBER_QUERY_CTX_SEQUENCED;
    ctx._source_id = *source_id;

    return _ze_advanced_subscriber_run_query(&ctx, state, keyexpr, params);
}

static inline z_result_t _ze_advanced_subscriber_timestamped_query(_ze_advanced_subscriber_state_simple_rc_t *state,
                                                                   const z_loaned_keyexpr_t *keyexpr,
                                                                   const char *params, const z_id_t *id) {
    _ze_advanced_subscriber_query_ctx_t ctx = _ze_advanced_subscriber_query_ctx_null();
    ctx._kind = _ZE_ADVANCED_SUBSCRIBER_QUERY_CTX_TIMESTAMPED;
    ctx._id = *id;

    return _ze_advanced_subscriber_run_query(&ctx, state, keyexpr, params);
}

void _ze_advanced_subscriber_subscriber_callback(z_loaned_sample_t *sample, void *ctx) {
    _ze_advanced_subscriber_state_simple_rc_t *rc_states = (_ze_advanced_subscriber_state_simple_rc_t *)ctx;
    if (!_Z_SIMPLE_RC_IS_NULL(rc_states)) {
        _ze_advanced_subscriber_state_t *states = _ze_advanced_subscriber_state_simple_rc_value(rc_states);
        if (_z_mutex_lock(&states->_mutex) != _Z_RES_OK) {
            _Z_ERROR("Failed to lock mutex when processing received sample");
            return;
        }

        bool new = __unsafe_ze_advanced_subscriber_handle_sample(states, sample);

        const z_loaned_source_info_t *source_info = z_sample_source_info(sample);
        if (_z_source_info_check(source_info)) {
            z_entity_global_id_t source_id = z_source_info_id(source_info);

            if (new) {
                // TODO: spawn periodic queries (states, source_id, rc_states.clone())
            }

            _ze_advanced_subscriber_sequenced_state_t *state =
                _z_entity_global_id__ze_advanced_subscriber_sequenced_state_hashmap_get(&states->_sequenced_states,
                                                                                        &source_id);
            if (state != NULL && states->_retransmission && state->_pending_queries == 0 &&
                !_z_uint32__z_sample_sortedmap_is_empty(&state->_pending_samples)) {
                state->_pending_queries++;

                char params[ZE_ADVANCED_SUBSCRIBER_QUERY_PARAM_BUF_SIZE];
                _z_query_param_range_t range = {._has_start = state->_has_last_delivered,
                                                ._start = state->_last_delivered,
                                                ._has_end = false,
                                                ._end = 0};

                _z_mutex_unlock(&states->_mutex);

                if (!_ze_advanced_subscriber_populate_query_params(params, sizeof(params), 0, 0, &range)) {
                    _Z_ERROR("Failed to prepare query for missing samples");
                } else if (_ze_advanced_subscriber_sequenced_query(rc_states, z_keyexpr_loan(&states->_query_key_expr),
                                                                   params, &source_id) != _Z_RES_OK) {
                    _Z_ERROR("Failed to query for missing samples");
                }
                return;
            }
        }

        _z_mutex_unlock(&states->_mutex);
    }
}

void _ze_advanced_subscriber_subscriber_drop_handler(void *ctx) {
    _ze_advanced_subscriber_state_simple_rc_t *rc_state = (_ze_advanced_subscriber_state_simple_rc_t *)ctx;
    if (!_Z_SIMPLE_RC_IS_NULL(rc_state)) {
        _ze_advanced_subscriber_state_t *state = _ze_advanced_subscriber_state_simple_rc_value(rc_state);
        if (state->_dropper != NULL) {
            state->_dropper(state->_ctx);
        }
        _ze_advanced_subscriber_state_simple_rc_drop(rc_state);
        z_free(ctx);
    }
}

// TODO
void _ze_advanced_subscriber_liveliness_callback(z_loaned_sample_t *sample, void *ctx) {
    (void)(ctx);
    (void)(sample);
}

void _ze_advanced_subscriber_liveliness_drop_handler(void *ctx) {
    _ze_advanced_subscriber_state_simple_rc_t *rc_state = (_ze_advanced_subscriber_state_simple_rc_t *)ctx;
    if (!_Z_SIMPLE_RC_IS_NULL(rc_state)) {
        _ze_advanced_subscriber_state_simple_rc_drop(rc_state);
        z_free(ctx);
    }
}

// TODO
void _ze_advanced_subscriber_heartbeat_callback(z_loaned_sample_t *sample, void *ctx) {
    (void)(ctx);
    (void)(sample);
}

void _ze_advanced_subscriber_heartbeat_drop_handler(void *ctx) {
    _ze_advanced_subscriber_state_simple_rc_t *rc_state = (_ze_advanced_subscriber_state_simple_rc_t *)ctx;
    if (!_Z_SIMPLE_RC_IS_NULL(rc_state)) {
        _ze_advanced_subscriber_state_simple_rc_drop(rc_state);
        z_free(ctx);
    }
}

// suffix = KE_ADV_PREFIX / KE_SUB / ZID / EID / [ metadata | KE_EMPTY ]
static z_result_t _ze_advanced_subscriber_ke_suffix(z_owned_keyexpr_t *suffix, const z_entity_global_id_t id,
                                                    const z_loaned_keyexpr_t *metadata) {
    z_internal_keyexpr_null(suffix);
    _Z_RETURN_IF_ERR(_Z_KEYEXPR_APPEND_STR_ARRAY(suffix, _Z_KEYEXPR_ADV_PREFIX, _Z_KEYEXPR_SUB));

    z_id_t zid = z_entity_global_id_zid(&id);
    z_owned_string_t zid_str;
    _Z_CLEAN_RETURN_IF_ERR(z_id_to_string(&zid, &zid_str), z_keyexpr_drop(z_keyexpr_move(suffix)));

    _Z_CLEAN_RETURN_IF_ERR(
        _z_keyexpr_append_substr(suffix, z_string_data(z_string_loan(&zid_str)), z_string_len(z_string_loan(&zid_str))),
        z_string_drop(z_string_move(&zid_str));
        z_keyexpr_drop(z_keyexpr_move(suffix)));
    z_string_drop(z_string_move(&zid_str));

    char buffer[21];
    uint32_t eid = z_entity_global_id_eid(&id);
    snprintf(buffer, sizeof(buffer), "%u", eid);
    _Z_CLEAN_RETURN_IF_ERR(_z_keyexpr_append_str(suffix, buffer), z_keyexpr_drop(z_keyexpr_move(suffix)));

    if (metadata != NULL) {
        _Z_CLEAN_RETURN_IF_ERR(_z_keyexpr_append_suffix(suffix, metadata), z_keyexpr_drop(z_keyexpr_move(suffix)));
    } else {
        _Z_CLEAN_RETURN_IF_ERR(_z_keyexpr_append_str(suffix, _Z_KEYEXPR_EMPTY), z_keyexpr_drop(z_keyexpr_move(suffix)));
    }

    return _Z_RES_OK;
}

z_result_t ze_declare_advanced_subscriber(const z_loaned_session_t *zs, ze_owned_advanced_subscriber_t *sub,
                                          const z_loaned_keyexpr_t *keyexpr, z_moved_closure_sample_t *callback,
                                          ze_advanced_subscriber_options_t *options) {
    sub->_val = _ze_advanced_subscriber_null();

    // Set options
    ze_advanced_subscriber_options_t opt;
    ze_advanced_subscriber_options_default(&opt);
    if (options != NULL) {
        opt = *options;
    }

    // Common keyexpr for subscribing to publisher liveliness and heartbeat and for querying history
    z_owned_keyexpr_t ke_pub;
    _Z_RETURN_IF_ERR(z_keyexpr_clone(&ke_pub, keyexpr));
    _Z_CLEAN_RETURN_IF_ERR(
        _Z_KEYEXPR_APPEND_STR_ARRAY(&ke_pub, _Z_KEYEXPR_ADV_PREFIX, _Z_KEYEXPR_PUB, _Z_KEYEXPR_STARSTAR),
        z_keyexpr_drop(z_keyexpr_move(&ke_pub)));

    // Create Advanced Subscriber state
    _Z_CLEAN_RETURN_IF_ERR(
        _ze_advanced_subscriber_state_new(&sub->_val._state, zs, callback, keyexpr, z_keyexpr_loan(&ke_pub), options),
        z_keyexpr_drop(z_keyexpr_move(&ke_pub)));

    // Declare subscriber
    _ze_advanced_subscriber_state_simple_rc_t *sub_state =
        _ze_advanced_subscriber_state_simple_rc_clone_as_ptr(&sub->_val._state);
    if (_Z_SIMPLE_RC_IS_NULL(sub_state)) {
        _ze_advanced_subscriber_state_simple_rc_drop(&sub->_val._state);
        z_keyexpr_drop(z_keyexpr_move(&ke_pub));
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }

    z_owned_closure_sample_t subcriber_callback;
    _Z_CLEAN_RETURN_IF_ERR(z_closure_sample(&subcriber_callback, _ze_advanced_subscriber_subscriber_callback,
                                            _ze_advanced_subscriber_subscriber_drop_handler, sub_state),
                           z_keyexpr_drop(z_keyexpr_move(&ke_pub));
                           _ze_advanced_subscriber_state_simple_rc_drop(sub_state); z_free(sub_state);
                           _ze_advanced_subscriber_state_simple_rc_drop(&sub->_val._state));
    _Z_CLEAN_RETURN_IF_ERR(z_declare_subscriber(zs, &sub->_val._subscriber, keyexpr,
                                                z_closure_sample_move(&subcriber_callback), &opt.subscriber_options),
                           z_keyexpr_drop(z_keyexpr_move(&ke_pub));
                           _ze_advanced_subscriber_state_simple_rc_drop(sub_state); z_free(sub_state);
                           _ze_advanced_subscriber_state_simple_rc_drop(&sub->_val._state));

    if (opt.history.is_enabled) {
        // Query initial history
        char params[ZE_ADVANCED_SUBSCRIBER_QUERY_PARAM_BUF_SIZE];
        if (!_ze_advanced_subscriber_populate_query_params(params, sizeof(params), opt.history.max_samples,
                                                           opt.history.max_age_ms, NULL)) {
            z_keyexpr_drop(z_keyexpr_move(&ke_pub));
            _ze_advanced_subscriber_drop(&sub->_val);
            return _Z_ERR_GENERIC;
        }

        _Z_CLEAN_RETURN_IF_ERR(
            _ze_advanced_subscriber_initial_query(&sub->_val._state, z_keyexpr_loan(&ke_pub), params),
            z_keyexpr_drop(z_keyexpr_move(&ke_pub));
            _ze_advanced_subscriber_drop(&sub->_val));

        // Declare liveliness subscriber on keyexpr / KE_ADV_PREFIX / KE_PUB / KE_STARSTAR
        if (opt.history.detect_late_publishers) {
            z_liveliness_subscriber_options_t liveliness_options;
            z_liveliness_subscriber_options_default(&liveliness_options);
            liveliness_options.history = true;

            _ze_advanced_subscriber_state_simple_rc_t *liveliness_sub_state =
                _ze_advanced_subscriber_state_simple_rc_clone_as_ptr(&sub->_val._state);
            if (_Z_SIMPLE_RC_IS_NULL(sub_state)) {
                z_keyexpr_drop(z_keyexpr_move(&ke_pub));
                _ze_advanced_subscriber_drop(&sub->_val);
                return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
            }

            z_owned_closure_sample_t liveliness_callback;
            _Z_CLEAN_RETURN_IF_ERR(
                z_closure_sample(&liveliness_callback, _ze_advanced_subscriber_liveliness_callback,
                                 _ze_advanced_subscriber_liveliness_drop_handler, liveliness_sub_state),
                _ze_advanced_subscriber_state_simple_rc_drop(liveliness_sub_state);
                z_free(liveliness_sub_state); z_keyexpr_drop(z_keyexpr_move(&ke_pub));
                _ze_advanced_subscriber_drop(&sub->_val));
            _Z_CLEAN_RETURN_IF_ERR(
                z_liveliness_declare_subscriber(zs, &sub->_val._liveliness_subscriber, z_keyexpr_loan(&ke_pub),
                                                z_closure_sample_move(&liveliness_callback), &liveliness_options),
                _ze_advanced_subscriber_state_simple_rc_drop(liveliness_sub_state);
                z_free(liveliness_sub_state); z_keyexpr_drop(z_keyexpr_move(&ke_pub));
                _ze_advanced_subscriber_drop(&sub->_val));
            sub->_val._has_liveliness_subscriber = true;
        }
    }

    // Heartbeat subscriber
    if (opt.recovery.is_enabled && opt.recovery.last_sample_miss_detection.is_enabled &&
        opt.recovery.last_sample_miss_detection.periodic_queries_period_ms == 0) {
        _ze_advanced_subscriber_state_simple_rc_t *heartbeat_sub_state =
            _ze_advanced_subscriber_state_simple_rc_clone_as_ptr(&sub->_val._state);
        if (_Z_SIMPLE_RC_IS_NULL(heartbeat_sub_state)) {
            z_keyexpr_drop(z_keyexpr_move(&ke_pub));
            _ze_advanced_subscriber_drop(&sub->_val);
            return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
        }

        z_owned_closure_sample_t heartbeat_callback;
        _Z_CLEAN_RETURN_IF_ERR(z_closure_sample(&heartbeat_callback, _ze_advanced_subscriber_heartbeat_callback,
                                                _ze_advanced_subscriber_heartbeat_drop_handler, heartbeat_sub_state),
                               _ze_advanced_subscriber_state_simple_rc_drop(heartbeat_sub_state);
                               z_free(heartbeat_sub_state); z_keyexpr_drop(z_keyexpr_move(&ke_pub));
                               _ze_advanced_subscriber_drop(&sub->_val));
        _Z_CLEAN_RETURN_IF_ERR(z_declare_subscriber(zs, &sub->_val._heartbeat_subscriber, z_keyexpr_loan(&ke_pub),
                                                    z_closure_sample_move(&heartbeat_callback), NULL),
                               _ze_advanced_subscriber_state_simple_rc_drop(heartbeat_sub_state);
                               z_free(heartbeat_sub_state); z_keyexpr_drop(z_keyexpr_move(&ke_pub));
                               _ze_advanced_subscriber_drop(&sub->_val));
        sub->_val._has_heartbeat_subscriber = true;
    }

    // Declare liveliness token on keyexpr/suffix
    if (opt.subscriber_detection) {
        _ze_advanced_subscriber_state_t *state = _ze_advanced_subscriber_state_simple_rc_value(&sub->_val._state);

        z_entity_global_id_t id = z_subscriber_id(z_subscriber_loan(&sub->_val._subscriber));
        z_owned_keyexpr_t suffix;
        _Z_CLEAN_RETURN_IF_ERR(_ze_advanced_subscriber_ke_suffix(&suffix, id, opt.subscriber_detection_metadata),
                               z_keyexpr_drop(z_keyexpr_move(&ke_pub));
                               _ze_advanced_subscriber_drop(&sub->_val));
        z_owned_keyexpr_t ke;
        _Z_CLEAN_RETURN_IF_ERR(z_keyexpr_join(&ke, keyexpr, z_keyexpr_loan(&suffix)),
                               z_keyexpr_drop(z_keyexpr_move(&ke_pub));
                               z_keyexpr_drop(z_keyexpr_move(&suffix)); _ze_advanced_subscriber_drop(&sub->_val));
        if (_z_mutex_lock(&state->_mutex) == _Z_RES_OK) {
            _Z_CLEAN_RETURN_IF_ERR(z_liveliness_declare_token(zs, &state->_token, z_keyexpr_loan(&ke), NULL),
                                   _z_mutex_unlock(&state->_mutex);
                                   z_keyexpr_drop(z_keyexpr_move(&ke_pub)); z_keyexpr_drop(z_keyexpr_move(&suffix));
                                   z_keyexpr_drop(z_keyexpr_move(&ke)); _ze_advanced_subscriber_drop(&sub->_val));
            state->_has_token = true;
            _z_mutex_unlock(&state->_mutex);
        }

        z_keyexpr_drop(z_keyexpr_move(&ke));
        z_keyexpr_drop(z_keyexpr_move(&suffix));
    }
    z_keyexpr_drop(z_keyexpr_move(&ke_pub));

    return _Z_RES_OK;
}

z_result_t ze_declare_background_advanced_subscriber(const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr,
                                                     z_moved_closure_sample_t *callback,
                                                     ze_advanced_subscriber_options_t *options) {
    ze_owned_advanced_subscriber_t sub;
    _Z_RETURN_IF_ERR(ze_declare_advanced_subscriber(zs, &sub, keyexpr, callback, options));
    _ze_advanced_subscriber_clear(&sub._val);
    return _Z_RES_OK;
}

z_result_t ze_undeclare_advanced_subscriber(ze_moved_advanced_subscriber_t *sub) {
    return _ze_advanced_subscriber_drop(&sub->_this._val);
}

const z_loaned_keyexpr_t *ze_advanced_subscriber_keyexpr(const ze_loaned_advanced_subscriber_t *sub) {
    return z_subscriber_keyexpr(z_subscriber_loan(&sub->_subscriber));
}

z_entity_global_id_t ze_advanced_subscriber_id(const ze_loaned_advanced_subscriber_t *sub) {
    return z_subscriber_id(z_subscriber_loan(&sub->_subscriber));
}

// TODO
z_result_t ze_advanced_subscriber_declare_sample_miss_listener(const ze_loaned_advanced_subscriber_t *subscriber,
                                                               ze_owned_sample_miss_listener_t *sample_miss_listener,
                                                               ze_moved_closure_miss_t *callback) {
    (void)(subscriber);
    (void)(sample_miss_listener);
    (void)(callback);
    return _Z_ERR_GENERIC;
}

// TODO
z_result_t ze_advanced_subscriber_declare_background_sample_miss_listener(
    const ze_loaned_advanced_subscriber_t *subscriber, ze_moved_closure_miss_t *callback) {
    (void)(subscriber);
    (void)(callback);
    return _Z_ERR_GENERIC;
}

// TODO
z_result_t ze_advanced_subscriber_detect_publishers(const ze_loaned_advanced_subscriber_t *subscriber,
                                                    z_owned_subscriber_t *liveliness_subscriber,
                                                    z_moved_closure_sample_t *callback,
                                                    z_liveliness_subscriber_options_t *options) {
    (void)(subscriber);
    (void)(liveliness_subscriber);
    (void)(callback);
    (void)(options);
    return _Z_ERR_GENERIC;
}

// TODO
z_result_t ze_advanced_subscriber_detect_publishers_background(const ze_loaned_advanced_subscriber_t *subscriber,
                                                               z_moved_closure_sample_t *callback,
                                                               z_liveliness_subscriber_options_t *options) {
    (void)(subscriber);
    (void)(callback);
    (void)(options);
    return _Z_ERR_GENERIC;
}

void ze_advanced_subscriber_history_options_default(ze_advanced_subscriber_history_options_t *options) {
    options->is_enabled = true;
    options->detect_late_publishers = false;
    options->max_samples = 0;
    options->max_age_ms = 0;
}

void ze_advanced_subscriber_last_sample_miss_detection_options_default(
    ze_advanced_subscriber_last_sample_miss_detection_options_t *options) {
    options->is_enabled = true;
    options->periodic_queries_period_ms = 0;
}

void ze_advanced_subscriber_recovery_options_default(ze_advanced_subscriber_recovery_options_t *options) {
    options->is_enabled = true;
    ze_advanced_subscriber_last_sample_miss_detection_options_default(&options->last_sample_miss_detection);
    options->last_sample_miss_detection.is_enabled = false;
}

void ze_advanced_subscriber_options_default(ze_advanced_subscriber_options_t *options) {
    z_subscriber_options_default(&options->subscriber_options);

    ze_advanced_subscriber_history_options_default(&options->history);
    options->history.is_enabled = false;

    ze_advanced_subscriber_recovery_options_default(&options->recovery);
    options->history.is_enabled = false;

    options->query_timeout_ms = 0;
    options->subscriber_detection = false;
    options->subscriber_detection_metadata = NULL;
}

#endif
