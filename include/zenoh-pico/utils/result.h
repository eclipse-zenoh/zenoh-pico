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

#ifndef ZENOH_PICO_UTILS_RESULT_H
#define ZENOH_PICO_UTILS_RESULT_H

#include <stddef.h>
#include <stdlib.h>

/*------------------ Result Enums ------------------*/
typedef enum {
    _Z_RES_OK = 0,
    _Z_ERR_PARSE_UINT8 = -1,
    _Z_ERR_PARSE_ZINT = -2,
    _Z_ERR_PARSE_BYTES = -3,
    _Z_ERR_PARSE_STRING = -4,
    _Z_ERR_PARSE_CONSOLIDATION = -5,
    _Z_ERR_PARSE_DECLARATION = -6,
    _Z_ERR_PARSE_PAYLOAD = -7,
    _Z_ERR_PARSE_PERIOD = -8,
    _Z_ERR_PARSE_PROPERTIES = -9,
    _Z_ERR_PARSE_PROPERTY = -10,
    _Z_ERR_PARSE_RESKEY = -11,
    _Z_ERR_PARSE_SUBMODE = -12,
    _Z_ERR_PARSE_TIMESTAMP = -13,
    _Z_ERR_PARSE_ZENOH_MESSAGE = -14,
    _Z_ERR_PARSE_TRANSPORT_MESSAGE = -15,

    _Z_ERR_SERIALIZING_TRANSPORT_MESSAGE = -16,
    _Z_ERR_DESERIALIZING_TRANSPORT_MESSAGE = -17,
    _Z_ERR_SERIALIZING_ZENOH_MESSAGE = -18,
    _Z_ERR_DESERIALIZING_ZENOH_MESSAGE = -19,

    _Z_ERR_DECLARE_KEYEXPR = -20,
    _Z_ERR_DECLARE_PUBLISHER = -21,
    _Z_ERR_DECLARE_SUBSCRIBER = -22,
    _Z_ERR_DECLARE_QUERYABLE = -23,
    _Z_ERR_REGISTER_QUERY = -24,
    _Z_ERR_KEYEXPR_UNKNOWN = -25,
    _Z_ERR_SUBSCRIPTION_UNKNOWN = -26,
    _Z_ERR_PUBLICATION_UNKNOWN = -27,
    _Z_ERR_QUERYABLE_UNKNOWN = -28,
    _Z_ERR_QUERY_UNKNOWN = -29,
    _Z_ERR_KEYEXPR_NOT_MATCH = -30,
    _Z_ERR_QUERY_NOT_MATCH = -31,
    _Z_ERR_QUERY_OLDER_DATA = -32,

    _Z_ERR_LOCATOR_UNKNOWN_SCHEMA = -33,
    _Z_ERR_LOCATOR_INVALID = -34,

    _Z_ERR_TRANSPORT_NOT_AVAILABLE = -35,
    _Z_ERR_TRANSPORT_OPEN_FAILED = -36,
    _Z_ERR_TRANSPORT_OPEN_SN_RESOLUTION = -37,
    _Z_ERR_TRANSPORT_TX_FAILED = -38,
    _Z_ERR_TRANSPORT_RX_FAILED = -39,

    _Z_ERR_IOBUF_NO_SPACE = -40,
    _Z_ERR_IOBUFFER_FAILED_WRITE = -41,

    _Z_ERR_MESSAGE_UNKNOWN = -42,
    _Z_ERR_MESSAGE_UNEXPECTED = -43,
    _Z_ERR_MESSAGE_FLAG_UNEXPECTED = -44,
    _Z_ERR_MESSAGE_DECLARATION_UNEXPECTED = -45,

    _Z_ERR_CONFIG_FAILED_INSERT = -46,
    _Z_ERR_CONFIG_UNSUPPORTED_CLIENT_MULTICAST = -47,
    _Z_ERR_CONFIG_UNSUPPORTED_PEER_UNICAST = -48,

    _Z_ERR_TASK_START_FAILED = -49,

    _Z_ERR_IO_GENERIC = -126,
    _Z_ERR_GENERIC = -127
} _z_res_t;

/*------------------ Internal Result Macros ------------------*/
#define _RESULT_DECLARE(type, name, prefix) \
    typedef struct {                        \
        _z_res_t _tag;                      \
        type _value;                        \
    } prefix##_##name##_result_t;

#define _P_RESULT_DECLARE(type, name, prefix)                                             \
    typedef struct {                                                                      \
        _z_res_t _tag;                                                                    \
        type *_value;                                                                     \
    } prefix##_##name##_p_result_t;                                                       \
                                                                                          \
    inline static void prefix##_##name##_p_result_init(prefix##_##name##_p_result_t *r) { \
        r->_value = (type *)z_malloc(sizeof(type));                                       \
    }                                                                                     \
                                                                                          \
    inline static void prefix##_##name##_p_result_free(prefix##_##name##_p_result_t *r) { \
        z_free(r->_value);                                                                \
        r->_value = NULL;                                                                 \
    }

#define _ASSURE_RESULT(in_r, out_r, e) \
    if (in_r._tag != _Z_RES_OK) {      \
        out_r._tag = _Z_ERR_GENERIC;   \
        return out_r;                  \
    }

#define _ASSURE_P_RESULT(in_r, out_r, e) \
    if (in_r._tag != _Z_RES_OK) {        \
        out_r->_tag = _Z_ERR_GENERIC;    \
        return;                          \
    }

#define _ASSURE_FREE_P_RESULT(in_r, out_r, e, name) \
    if (in_r._tag != _Z_RES_OK) {                   \
        z_free(out_r->_value);                      \
        out_r->_tag = _Z_ERR_GENERIC;               \
        return;                                     \
    }

#define _ASSERT_RESULT(r, msg) \
    if (r._tag != _Z_RES_OK) { \
        _Z_ERROR(msg);         \
        _Z_ERROR("\n");        \
        exit(r._tag);          \
    }

#define _ASSERT_P_RESULT(r, msg) \
    if (r._tag != _Z_RES_OK) {   \
        _Z_ERROR(msg);           \
        _Z_ERROR("\n");          \
        exit(r._tag);            \
    }

/*------------------ Internal Zenoh Results ------------------*/
#define _Z_RESULT_DECLARE(type, name) _RESULT_DECLARE(type, name, _z)
#define _Z_P_RESULT_DECLARE(type, name) _P_RESULT_DECLARE(type, name, _z)

/*------------------ Internal Zenoh Results ------------------*/
#define Z_RESULT_DECLARE(type, name) _RESULT_DECLARE(type, name, z)
#define Z_P_RESULT_DECLARE(type, name) _P_RESULT_DECLARE(type, name, z)

#endif /* ZENOH_PICO_UTILS_RESULT_H */
