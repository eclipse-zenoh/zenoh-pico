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
#include "zenoh-pico/collections/advanced_cache.h"

#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"
#include "zenoh-pico/utils/query_params.h"
#include "zenoh-pico/utils/time_range.h"

#define _ZE_ADVANCED_CACHE_QUERY_PARAMETERS_RANGE_UNBOUNDED -1
#define _ZE_ADVANCED_CACHE_QUERY_PARAMETERS_MAX_UNBOUNDED SIZE_MAX

#if Z_FEATURE_ADVANCED_PUBLICATION == 1

typedef struct {
    int64_t start;
    int64_t end;
} _ze_advanced_cache_range_t;

typedef struct {
    _ze_advanced_cache_range_t range;
    size_t max;
    _z_time_range_t time;
} _ze_advanced_cache_query_parameters_t;

static bool _ze_advanced_cache_query_match_key(const char *key_start, size_t key_len, const char *expected_key,
                                               size_t expected_key_len) {
    return (key_len == expected_key_len && strncmp(key_start, expected_key, key_len) == 0);
}

static void _ze_advanced_cache_query_parse_max(const _z_str_se_t *str, size_t *max) {
    *max = _ZE_ADVANCED_CACHE_QUERY_PARAMETERS_MAX_UNBOUNDED;
    if (_z_ptr_char_diff(str->end, str->start) > 0) {
        uint32_t value;
        if (_z_str_se_atoui(str, &value)) {
            *max = value;
        }
    }
}

static void _ze_advanced_cache_query_parse_range(const _z_str_se_t *str, _ze_advanced_cache_range_t *range) {
    range->start = _ZE_ADVANCED_CACHE_QUERY_PARAMETERS_RANGE_UNBOUNDED;
    range->end = _ZE_ADVANCED_CACHE_QUERY_PARAMETERS_RANGE_UNBOUNDED;

    if (_z_ptr_char_diff(str->end, str->start) > 0) {
        _z_splitstr_t ss = {.s = *str, .delimiter = ".."};

        _z_str_se_t token = _z_splitstr_next(&ss);
        uint32_t value;
        if (_z_str_se_atoui(&token, &value)) {
            range->start = value;
        }
        token = _z_splitstr_next(&ss);
        if (_z_str_se_atoui(&token, &value)) {
            range->end = value;
        }
    }
}

static void _ze_advanced_cache_query_parse_time(const _z_str_se_t *str, _z_time_range_t *time) {
    _z_time_range_t range;
    if (_z_time_range_from_str(str->start, _z_ptr_char_diff(str->end, str->start), &range)) {
        *time = range;
    } else {
        time->start.bound = _Z_TIME_BOUND_UNBOUNDED;
        time->end.bound = _Z_TIME_BOUND_UNBOUNDED;
    }
}

static void _ze_advanced_cache_query_parse_parameters(_ze_advanced_cache_query_parameters_t *params,
                                                      const z_loaned_string_t *raw_params) {
    params->range.start = _ZE_ADVANCED_CACHE_QUERY_PARAMETERS_RANGE_UNBOUNDED;
    params->range.end = _ZE_ADVANCED_CACHE_QUERY_PARAMETERS_RANGE_UNBOUNDED;
    params->max = _ZE_ADVANCED_CACHE_QUERY_PARAMETERS_MAX_UNBOUNDED;
    params->time.start.bound = _Z_TIME_BOUND_UNBOUNDED;
    params->time.end.bound = _Z_TIME_BOUND_UNBOUNDED;

    _z_str_se_t str;
    str.start = z_string_data(raw_params);
    str.end = str.start + z_string_len(raw_params);
    while (str.start != NULL) {
        _z_query_param_t param = _z_query_params_next(&str);
        if (param.key.start != NULL) {
            size_t key_len = _z_ptr_char_diff(param.key.end, param.key.start) + 1;
            if (_ze_advanced_cache_query_match_key(param.key.start, key_len, _Z_QUERY_PARAMS_KEY_RANGE,
                                                   _Z_QUERY_PARAMS_KEY_RANGE_LEN)) {
                _ze_advanced_cache_query_parse_range(&param.value, &params->range);
            } else if (_ze_advanced_cache_query_match_key(param.key.start, key_len, _Z_QUERY_PARAMS_KEY_MAX,
                                                          _Z_QUERY_PARAMS_KEY_MAX_LEN)) {
                _ze_advanced_cache_query_parse_max(&param.value, &params->max);
            } else if (_ze_advanced_cache_query_match_key(param.key.start, key_len, _Z_QUERY_PARAMS_KEY_TIME,
                                                          _Z_QUERY_PARAMS_KEY_TIME_LEN)) {
                _ze_advanced_cache_query_parse_time(&param.value, &params->time);
            }
        }
    }
}

static bool _ze_advanced_cache_range_contains(const _ze_advanced_cache_range_t *range, uint32_t sn) {
    return ((range->start == _ZE_ADVANCED_CACHE_QUERY_PARAMETERS_RANGE_UNBOUNDED || range->start <= sn) &&
            (range->end == _ZE_ADVANCED_CACHE_QUERY_PARAMETERS_RANGE_UNBOUNDED || sn <= range->end));
}

static void _ze_advanced_cache_query_handler(z_loaned_query_t *query, void *ctx) {
    _ze_advanced_cache_t *cache = (_ze_advanced_cache_t *)ctx;

    z_view_string_t keystr;
    z_keyexpr_as_view_string(z_query_keyexpr(query), &keystr);
    z_view_string_t param_str;
    z_query_parameters(query, &param_str);

    _ze_advanced_cache_query_parameters_t params;
    _ze_advanced_cache_query_parse_parameters(&params, z_view_string_loan(&param_str));

    z_result_t res = _Z_RES_OK;
#if Z_FEATURE_MULTI_THREAD == 1
    res = _z_mutex_lock(&cache->_mutex);
    if (res != _Z_RES_OK) {
        _Z_ERROR("_z_mutex_lock failed: %i", res);
        return;
    }
#endif
    size_t max = (params.max != _ZE_ADVANCED_CACHE_QUERY_PARAMETERS_MAX_UNBOUNDED)
                     ? params.max
                     : _z_sample_ring_capacity(&cache->_cache);
    _z_sample_ring_reverse_iterator_t iter = _z_sample_ring_reverse_iterator_make(&cache->_cache);

    _z_time_since_epoch now;
    res = _z_get_time_since_epoch(&now);
    if (res != _Z_RES_OK) {
        _Z_ERROR("_z_get_time_since_epoch failed: %i", res);
        return;
    }
    _z_ntp64_t now_ntp64 = _z_timestamp_ntp64_from_time(now.secs, now.nanos);

    size_t to_send = 0;
    while (max > 0 && _z_sample_ring_reverse_iterator_next(&iter)) {
        _z_sample_t *sample = _z_sample_ring_reverse_iterator_value(&iter);
        if ((params.range.start == _ZE_ADVANCED_CACHE_QUERY_PARAMETERS_RANGE_UNBOUNDED &&
             params.range.end == _ZE_ADVANCED_CACHE_QUERY_PARAMETERS_RANGE_UNBOUNDED) ||
            (_z_source_info_check(&sample->source_info) &&
             _ze_advanced_cache_range_contains(&params.range, sample->source_info._source_sn))) {
            if ((params.time.start.bound != _Z_TIME_BOUND_UNBOUNDED ||
                 params.time.end.bound != _Z_TIME_BOUND_UNBOUNDED) &&
                _z_timestamp_check(&sample->timestamp) &&
                !_z_time_range_contains_at_time(&params.time, sample->timestamp.time, now_ntp64)) {
                continue;
            }

            cache->_query_reply_buffer[to_send++] = sample;
            max--;
        }
    }

    // Send samples in order
    while (to_send > 0) {
        to_send--;
        _z_sample_t *sample = cache->_query_reply_buffer[to_send];

        z_query_reply_options_t opt;
        z_query_reply_options_default(&opt);
        opt.congestion_control = cache->_congestion_control;
        opt.priority = cache->_priority;
        opt.is_express = cache->_is_express;
        res = _z_query_reply_sample(query, sample, &opt);
        if (res != _Z_RES_OK) {
            _Z_ERROR("_z_query_reply_sample failed: %i", res);
            continue;
        }
    }

#if Z_FEATURE_MULTI_THREAD == 1
    res = _z_mutex_unlock(&cache->_mutex);
    if (res != _Z_RES_OK) {
        _Z_ERROR("_z_mutex_unlock failed: %i", res);
    }
#endif
}

z_result_t _ze_advanced_cache_init(_ze_advanced_cache_t *cache, const z_loaned_session_t *zs,
                                   const z_loaned_keyexpr_t *keyexpr, const z_loaned_keyexpr_t *suffix,
                                   const ze_advanced_publisher_cache_options_t options) {
    _Z_RETURN_IF_ERR(_z_sample_ring_init(&cache->_cache, options.max_samples));
    cache->_query_reply_buffer = (_z_sample_t **)z_malloc(sizeof(_z_sample_t *) * options.max_samples);

#if Z_FEATURE_MULTI_THREAD == 1
    _Z_CLEAN_RETURN_IF_ERR(_z_mutex_init(&cache->_mutex), _z_sample_ring_clear(&cache->_cache));
#endif

    cache->_congestion_control = options.congestion_control;
    cache->_priority = options.priority;
    cache->_is_express = options.is_express;

    z_owned_keyexpr_t ke;
    z_internal_keyexpr_null(&ke);
    if (suffix != NULL) {
        _Z_CLEAN_RETURN_IF_ERR(z_keyexpr_join(&ke, keyexpr, suffix), _z_sample_ring_clear(&cache->_cache));
    } else {
        _Z_CLEAN_RETURN_IF_ERR(z_keyexpr_clone(&ke, keyexpr), _z_sample_ring_clear(&cache->_cache));
    }

    z_internal_queryable_null(&cache->_queryable);
    z_owned_closure_query_t callback;
    _Z_CLEAN_RETURN_IF_ERR(z_closure_query(&callback, _ze_advanced_cache_query_handler, NULL, cache),
                           z_keyexpr_drop(z_keyexpr_move(&ke));
                           _z_sample_ring_clear(&cache->_cache));
    _Z_CLEAN_RETURN_IF_ERR(
        z_declare_queryable(zs, &cache->_queryable, z_keyexpr_loan(&ke), z_closure_query_move(&callback), NULL),
        z_keyexpr_drop(z_keyexpr_move(&ke));
        _z_sample_ring_clear(&cache->_cache));

    z_internal_liveliness_token_null(&cache->_liveliness);
    if (options._liveness) {
        _Z_CLEAN_RETURN_IF_ERR(z_liveliness_declare_token(zs, &cache->_liveliness, z_keyexpr_loan(&ke), NULL),
                               z_queryable_drop(z_queryable_move(&cache->_queryable));
                               z_keyexpr_drop(z_keyexpr_move(&ke)); _z_sample_ring_clear(&cache->_cache));
    }

    z_keyexpr_drop(z_keyexpr_move(&ke));
    return _Z_RES_OK;
}

_ze_advanced_cache_t *_ze_advanced_cache_new(const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr,
                                             const z_loaned_keyexpr_t *suffix,
                                             const ze_advanced_publisher_cache_options_t options) {
    _ze_advanced_cache_t *cache = (_ze_advanced_cache_t *)z_malloc(sizeof(_ze_advanced_cache_t));
    if (cache == NULL) {
        _Z_ERROR("z_malloc failed");
        return NULL;
    }

    z_result_t ret = _ze_advanced_cache_init(cache, zs, keyexpr, suffix, options);
    if (ret != _Z_RES_OK) {
        _Z_ERROR("_ze_advanced_cache_init failed: %i", ret);
        z_free(cache);
        return NULL;
    }

    return cache;
}

z_result_t _ze_advanced_cache_add(_ze_advanced_cache_t *cache, _z_sample_t *sample) {
    if (cache == NULL || sample == NULL) {
        _Z_ERROR("Invalid cache or sample");
        return _Z_ERR_INVALID;
    }
    _z_sample_t *s = (_z_sample_t *)z_malloc(sizeof(_z_sample_t));
    if (s == NULL) {
        _Z_ERROR("Out of memory");
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }

    z_result_t res = _z_sample_move(s, sample);
    if (res != _Z_RES_OK) {
        z_free((void *)s);
        return res;
    }

#if Z_FEATURE_MULTI_THREAD == 1
    res = _z_mutex_lock(&cache->_mutex);
    if (res != _Z_RES_OK) {
        z_free((void *)s);
        return res;
    }
#endif
    _z_sample_ring_push_force_drop(&cache->_cache, s);
#if Z_FEATURE_MULTI_THREAD == 1
    _Z_RETURN_IF_ERR(_z_mutex_unlock(&cache->_mutex))
#endif

    return _Z_RES_OK;
}

void _ze_advanced_cache_free(_ze_advanced_cache_t **cache) {
    _ze_advanced_cache_t *ptr = (_ze_advanced_cache_t *)*cache;
    if (ptr != NULL) {
        z_liveliness_token_drop(z_liveliness_token_move(&ptr->_liveliness));
        z_queryable_drop(z_queryable_move(&ptr->_queryable));
#if Z_FEATURE_MULTI_THREAD == 1
        _z_mutex_drop(&ptr->_mutex);
#endif
        _z_sample_ring_clear(&ptr->_cache);
        z_free(ptr->_query_reply_buffer);
        z_free(ptr);
        *cache = NULL;
    }
}

#endif  // Z_FEATURE_ADVANCED_PUBLICATION == 1
