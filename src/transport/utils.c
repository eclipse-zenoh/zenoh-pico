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

#include "zenoh-pico/transport/utils.h"

_Bool _z_sn_precedes(const _z_zint_t seq_num_res_half, const _z_zint_t sn_left, const _z_zint_t sn_right) {
    _Bool ret = false;

    if (sn_right > sn_left) {
        ret = ((sn_right - sn_left) <= seq_num_res_half);
    } else {
        ret = ((sn_left - sn_right) > seq_num_res_half);
    }

    return ret;
}

_z_zint_t _z_sn_increment(const _z_zint_t seq_num_res, const _z_zint_t sn) { return (sn + 1) % seq_num_res; }

_z_zint_t _z_sn_decrement(const _z_zint_t seq_num_res, const _z_zint_t sn) {
    _z_zint_t ret = 0;

    if (sn == 0) {
        ret = seq_num_res - 1;
    } else {
        ret = sn - 1;
    }

    return ret;
}

void _z_conduit_sn_list_copy(_z_conduit_sn_list_t *dst, const _z_conduit_sn_list_t *src) {
    dst->_is_qos = src->_is_qos;
    if (dst->_is_qos == false) {
        dst->_val._plain._best_effort = src->_val._plain._best_effort;
        dst->_val._plain._reliable = src->_val._plain._reliable;
    } else {
        for (uint8_t i = 0; i < Z_PRIORITIES_NUM; i++) {
            dst->_val._qos[i]._best_effort = src->_val._qos[i]._best_effort;
            dst->_val._qos[i]._reliable = src->_val._qos[i]._reliable;
        }
    }
}

void _z_conduit_sn_list_decrement(const _z_zint_t seq_num_res, _z_conduit_sn_list_t *sns) {
    if (sns->_is_qos == false) {
        sns->_val._plain._best_effort = _z_sn_decrement(seq_num_res, sns->_val._plain._best_effort);
        sns->_val._plain._reliable = _z_sn_decrement(seq_num_res, sns->_val._plain._reliable);
    } else {
        for (uint8_t i = 0; i < Z_PRIORITIES_NUM; i++) {
            sns->_val._qos[i]._best_effort = _z_sn_decrement(seq_num_res, sns->_val._qos[i]._best_effort);
            sns->_val._qos[i]._best_effort = _z_sn_decrement(seq_num_res, sns->_val._qos[i]._reliable);
        }
    }
}