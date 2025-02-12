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

#include "zenoh-pico/api/advanced_publisher.h"

#include <stdio.h>

#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/utils/result.h"

#if Z_FEATURE_ADVANCED_PUBLICATION == 1

/**************** Advanced Publisher ****************/

static const char *KE_ADV_PREFIX = "@adv";
static const char *KE_PUB = "pub";
static const char *KE_UHLC = "uhlc";
static const char *KE_EMPTY = "_";

bool _ze_advanced_publisher_check(const _ze_advanced_publisher_t *pub) {
    return z_internal_publisher_check(&pub->_publisher) && z_internal_liveliness_token_check(&pub->_liveliness);
}

_ze_advanced_publisher_t _ze_advanced_publisher_null(void) {
    _ze_advanced_publisher_t publisher = {0};
    z_internal_publisher_null(&publisher._publisher);
    z_internal_liveliness_token_null(&publisher._liveliness);
    publisher._cache = NULL;

    return publisher;
}

z_result_t _ze_undeclare_advanced_clear(_ze_advanced_publisher_t *pub) {
    z_result_t ret = z_undeclare_publisher(z_publisher_move(&pub->_publisher));
    z_liveliness_token_drop(z_liveliness_token_move(&pub->_liveliness));
    if (pub->_cache != NULL) {
        _ze_advanced_cache_free(&pub->_cache);
    }
    *pub = _ze_advanced_publisher_null();
    return ret;
}

_Z_OWNED_FUNCTIONS_VALUE_NO_COPY_NO_MOVE_IMPL_PREFIX(ze, _ze_advanced_publisher_t, advanced_publisher,
                                                     _ze_advanced_publisher_check, _ze_advanced_publisher_null,
                                                     _ze_undeclare_advanced_clear)

void ze_advanced_publisher_cache_options_default(ze_advanced_publisher_cache_options_t *options) {
    options->is_enabled = true;
    options->max_samples = 1;
    options->congestion_control = z_internal_congestion_control_default_push();
    options->priority = z_priority_default();
    options->is_express = false;
    options->_liveness = false;
}

void ze_advanced_publisher_sample_miss_detection_options_default(
    ze_advanced_publisher_sample_miss_detection_options_t *options) {
    options->is_enabled = true;
    options->heartbeat_mode = ZE_ADVANCED_PUBLISHER_HEARTBEAT_MODE_NONE;
    options->heartbeat_period_ms = 0;
}

void ze_advanced_publisher_options_default(ze_advanced_publisher_options_t *options) {
    z_publisher_options_default(&options->publisher_options);
    ze_advanced_publisher_cache_options_default(&options->cache);
    options->cache.is_enabled = false;
    ze_advanced_publisher_sample_miss_detection_options_default(&options->sample_miss_detection);
    options->sample_miss_detection.is_enabled = false;
    options->publisher_detection = false;
    options->publisher_detection_metadata = NULL;
}

void ze_advanced_publisher_put_options_default(ze_advanced_publisher_put_options_t *options) {
    z_publisher_put_options_default(&options->put_options);
}

void ze_advanced_publisher_delete_options_default(ze_advanced_publisher_delete_options_t *options) {
    z_publisher_delete_options_default(&options->delete_options);
}

static z_result_t _ze_advanced_publisher_ke_append_ke(z_owned_keyexpr_t *prefix, const z_loaned_keyexpr_t *right) {
    z_owned_keyexpr_t tmp;
    z_result_t res = z_keyexpr_join(&tmp, z_keyexpr_loan(prefix), right);
    if (res == _Z_RES_OK) {
        z_keyexpr_drop(z_keyexpr_move(prefix));
        z_keyexpr_take(prefix, z_keyexpr_move(&tmp));
    }
    return res;
}

static z_result_t _ze_advanced_publisher_ke_append_substr(z_owned_keyexpr_t *prefix, const char *right, size_t len) {
    z_view_keyexpr_t ke_right;
    z_view_keyexpr_from_substr_unchecked(&ke_right, right, len);
    return _ze_advanced_publisher_ke_append_ke(prefix, z_view_keyexpr_loan(&ke_right));
}

static inline z_result_t _ze_advanced_publisher_ke_append_str(z_owned_keyexpr_t *prefix, const char *right) {
    return _ze_advanced_publisher_ke_append_substr(prefix, right, strlen(right));
}

// suffix = KE_ADV_PREFIX / KE_PUB / ZID / [ EID | KE_UHLC ] / [ meta | KE_EMPTY]
static z_result_t _ze_advanced_publisher_ke_suffix(z_owned_keyexpr_t *suffix, const z_entity_global_id_t id,
                                                   const _ze_advanced_publisher_sequencing_t sequencing,
                                                   const z_loaned_keyexpr_t *metadata) {
    z_internal_keyexpr_null(suffix);
    _Z_RETURN_IF_ERR(z_keyexpr_from_str(suffix, KE_ADV_PREFIX));
    _Z_CLEAN_RETURN_IF_ERR(_ze_advanced_publisher_ke_append_str(suffix, KE_PUB),
                           z_keyexpr_drop(z_keyexpr_move(suffix)));

    z_id_t zid = z_entity_global_id_zid(&id);
    z_owned_string_t zid_str;
    _Z_CLEAN_RETURN_IF_ERR(z_id_to_string(&zid, &zid_str), z_keyexpr_drop(z_keyexpr_move(suffix)));

    z_result_t res = _ze_advanced_publisher_ke_append_substr(suffix, z_string_data(z_string_loan(&zid_str)),
                                                             z_string_len(z_string_loan(&zid_str)));
    z_string_drop(z_string_move(&zid_str));
    if (res != _Z_RES_OK) {
        z_keyexpr_drop(z_keyexpr_move(suffix));
        return res;
    }

    if (sequencing == _ZE_ADVANCED_PUBLISHER_SEQUENCING_SEQUENCE_NUMBER) {
        char buffer[21];
        uint32_t eid = z_entity_global_id_eid(&id);
        snprintf(buffer, sizeof(buffer), "%u", eid);
        _Z_CLEAN_RETURN_IF_ERR(_ze_advanced_publisher_ke_append_str(suffix, buffer),
                               z_keyexpr_drop(z_keyexpr_move(suffix)));
    } else {
        _Z_CLEAN_RETURN_IF_ERR(_ze_advanced_publisher_ke_append_str(suffix, KE_UHLC),
                               z_keyexpr_drop(z_keyexpr_move(suffix)));
    }

    if (metadata != NULL) {
        _Z_CLEAN_RETURN_IF_ERR(_ze_advanced_publisher_ke_append_ke(suffix, metadata),
                               z_keyexpr_drop(z_keyexpr_move(suffix)));
    } else {
        _Z_CLEAN_RETURN_IF_ERR(_ze_advanced_publisher_ke_append_str(suffix, KE_EMPTY),
                               z_keyexpr_drop(z_keyexpr_move(suffix)));
    }
    return _Z_RES_OK;
}

z_result_t ze_declare_advanced_publisher(const z_loaned_session_t *zs, ze_owned_advanced_publisher_t *pub,
                                         const z_loaned_keyexpr_t *keyexpr,
                                         const ze_advanced_publisher_options_t *options) {
    pub->_val = _ze_advanced_publisher_null();

    // Set options
    ze_advanced_publisher_options_t opt;
    ze_advanced_publisher_options_default(&opt);
    if (options != NULL) {
        opt = *options;
    }

    z_result_t res = z_declare_publisher(zs, &pub->_val._publisher, keyexpr, &opt.publisher_options);
    if (res != _Z_RES_OK) {
        return res;
    }

    z_entity_global_id_t id = z_publisher_id(z_publisher_loan(&pub->_val._publisher));

    if (opt.sample_miss_detection.is_enabled) {
        pub->_val._sequencing = _ZE_ADVANCED_PUBLISHER_SEQUENCING_SEQUENCE_NUMBER;
        _z_seqnumber_init(&pub->_val._seqnumber);
    } else if (opt.cache.is_enabled) {
        pub->_val._sequencing = _ZE_ADVANCED_PUBLISHER_SEQUENCING_TIMESTAMP;
    }

    z_owned_keyexpr_t suffix;

    _Z_CLEAN_RETURN_IF_ERR(
        _ze_advanced_publisher_ke_suffix(&suffix, id, pub->_val._sequencing, opt.publisher_detection_metadata),
        z_publisher_drop(z_publisher_move(&pub->_val._publisher)));

    if (opt.cache.is_enabled) {
        _ze_advanced_cache_t *cache = _ze_advanced_cache_new(zs, keyexpr, z_keyexpr_loan(&suffix), opt.cache);

        if (cache == NULL) {
            z_publisher_drop(z_publisher_move(&pub->_val._publisher));
            z_keyexpr_drop(z_keyexpr_move(&suffix));
            return _Z_ERR_GENERIC;
        }
        pub->_val._cache = cache;
    }

    // Declare liveliness token on key_expr/suffix
    if (opt.publisher_detection) {
        z_owned_keyexpr_t ke;
        _Z_CLEAN_RETURN_IF_ERR(z_keyexpr_join(&ke, keyexpr, z_keyexpr_loan(&suffix)),
                               z_publisher_drop(z_publisher_move(&pub->_val._publisher));
                               z_keyexpr_drop(z_keyexpr_move(&suffix)); _ze_advanced_cache_free(&pub->_val._cache));

        _Z_CLEAN_RETURN_IF_ERR(z_liveliness_declare_token(zs, &pub->_val._liveliness, z_keyexpr_loan(&ke), NULL),
                               z_keyexpr_drop(z_keyexpr_move(&ke));
                               z_publisher_drop(z_publisher_move(&pub->_val._publisher));
                               z_keyexpr_drop(z_keyexpr_move(&suffix)); _ze_advanced_cache_free(&pub->_val._cache));
        z_keyexpr_drop(z_keyexpr_move(&ke));
    }

    // TODO: State publisher

    return _Z_RES_OK;
}

static z_result_t _ze_advanced_publisher_sequencing_options(const ze_loaned_advanced_publisher_t *pub,
                                                            z_owned_source_info_t *source_info,
                                                            z_timestamp_t *timestamp) {
    if (source_info == NULL || timestamp == NULL) {
        return _Z_ERR_INVALID;
    }

    const z_loaned_publisher_t *publisher = z_publisher_loan(&pub->_publisher);

    // Set sequence number is required
    if (pub->_sequencing == _ZE_ADVANCED_PUBLISHER_SEQUENCING_SEQUENCE_NUMBER) {
        z_entity_global_id_t publisher_id = z_publisher_id(publisher);
        uint32_t seqnumber = 0;
        _Z_RETURN_IF_ERR(_z_seqnumber_fetch_and_increment((_z_seqnumber_t *)&pub->_seqnumber, &seqnumber));
        (void)z_source_info_new(source_info, &publisher_id, seqnumber);
    }

// Set timestamp
#if Z_FEATURE_SESSION_CHECK == 1
    _z_session_rc_t sess_rc = _z_session_weak_upgrade_if_open(&publisher->_zn);
#else
    _z_session_rc_t sess_rc = _z_session_weak_upgrade(&publisher->_zn);
#endif
    if (!_Z_RC_IS_NULL(&sess_rc)) {
        _Z_CLEAN_RETURN_IF_ERR(z_timestamp_new(timestamp, &sess_rc), _z_session_rc_drop(&sess_rc));
        _z_session_rc_drop(&sess_rc);
    } else {
        return _Z_ERR_SESSION_CLOSED;
    }
    return _Z_RES_OK;
}

z_result_t ze_undeclare_advanced_publisher(ze_moved_advanced_publisher_t *pub) {
    return _ze_undeclare_advanced_clear(&pub->_this._val);
}

z_result_t ze_advanced_publisher_put(const ze_loaned_advanced_publisher_t *pub, z_moved_bytes_t *payload,
                                     const ze_advanced_publisher_put_options_t *options) {
    ze_advanced_publisher_put_options_t opt;
    ze_advanced_publisher_put_options_default(&opt);
    if (options != NULL) {
        opt = *options;
    }

    z_timestamp_t timestamp = _z_timestamp_null();
    z_owned_source_info_t si;
    z_internal_source_info_null(&si);
    _Z_RETURN_IF_ERR(_ze_advanced_publisher_sequencing_options(pub, &si, &timestamp));
    opt.put_options.timestamp = &timestamp;
    opt.put_options.source_info = z_source_info_move(&si);
    return _z_publisher_put_impl(z_publisher_loan(&pub->_publisher), payload, &opt.put_options, pub->_cache);
}

z_result_t ze_advanced_publisher_delete(const ze_loaned_advanced_publisher_t *pub,
                                        const ze_advanced_publisher_delete_options_t *options) {
    ze_advanced_publisher_delete_options_t opt;
    ze_advanced_publisher_delete_options_default(&opt);
    if (options != NULL) {
        opt = *options;
    }

    z_timestamp_t timestamp = _z_timestamp_null();
    z_owned_source_info_t si;
    z_internal_source_info_null(&si);
    _Z_RETURN_IF_ERR(_ze_advanced_publisher_sequencing_options(pub, &si, &timestamp));
    opt.delete_options.timestamp = &timestamp;
    opt.delete_options.source_info = z_source_info_move(&si);
    return _z_publisher_delete_impl(z_publisher_loan(&pub->_publisher), &opt.delete_options, pub->_cache);
}

const z_loaned_keyexpr_t *ze_advanced_publisher_keyexpr(const ze_loaned_advanced_publisher_t *pub) {
    return z_publisher_keyexpr(z_publisher_loan(&pub->_publisher));
}

z_entity_global_id_t ze_advanced_publisher_id(const ze_loaned_advanced_publisher_t *pub) {
    return z_publisher_id(z_publisher_loan(&pub->_publisher));
}

z_result_t ze_advanced_publisher_get_matching_status(const ze_loaned_advanced_publisher_t *pub,
                                                     z_matching_status_t *matching_status) {
    return z_publisher_get_matching_status(z_publisher_loan(&pub->_publisher), matching_status);
}

z_result_t ze_advanced_publisher_declare_matching_listener(const ze_loaned_advanced_publisher_t *publisher,
                                                           z_owned_matching_listener_t *matching_listener,
                                                           z_moved_closure_matching_status_t *callback) {
    return z_publisher_declare_matching_listener(z_publisher_loan(&publisher->_publisher), matching_listener, callback);
}

z_result_t ze_advanced_publisher_declare_background_matching_listener(const ze_loaned_advanced_publisher_t *publisher,
                                                                      z_moved_closure_matching_status_t *callback) {
    return z_publisher_declare_background_matching_listener(z_publisher_loan(&publisher->_publisher), callback);
}

#endif
