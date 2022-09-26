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
    _Z_ERR_PARSE_UINT8,
    _Z_ERR_PARSE_ZINT,
    _Z_ERR_PARSE_BYTES,
    _Z_ERR_PARSE_STRING,
    _Z_ERR_IO_GENERIC,
    _Z_ERR_IOBUF_NO_SPACE,
    _Z_ERR_INVALID_LOCATOR,
    _Z_ERR_OPEN_TRANSPORT_FAILED,
    _Z_ERR_PARSE_CONSOLIDATION,
    _Z_ERR_PARSE_DECLARATION,
    _Z_ERR_PARSE_PAYLOAD,
    _Z_ERR_PARSE_PERIOD,
    _Z_ERR_PARSE_PROPERTIES,
    _Z_ERR_PARSE_PROPERTY,
    _Z_ERR_PARSE_RESKEY,
    _Z_ERR_PARSE_TRANSPORT_MESSAGE,
    _Z_ERR_PARSE_SUBMODE,
    _Z_ERR_PARSE_TIMESTAMP,
    _Z_ERR_PARSE_ZENOH_MESSAGE,
    _Z_ERR_RESOURCE_DECLARATION,
    _Z_ERR_TX_CONNECTION,
    _Z_ERR_UNEXPECTED_MESSAGE
} _z_err_t;

typedef enum { _Z_RES_OK = 0, _Z_RES_ERR = -1 } _z_res_t;

/*------------------ Internal Result Macros ------------------*/
#define _RESULT_DECLARE(type, name, prefix) \
    typedef struct {                        \
        _z_res_t _tag;                      \
        union {                             \
            type _##name;                   \
            int _error;                     \
        } _value;                           \
    } prefix##_##name##_result_t;

#define _P_RESULT_DECLARE(type, name, prefix)                                             \
    typedef struct {                                                                      \
        _z_res_t _tag;                                                                    \
        union {                                                                           \
            type *_##name;                                                                \
            int _error;                                                                   \
        } _value;                                                                         \
    } prefix##_##name##_p_result_t;                                                       \
                                                                                          \
    inline static void prefix##_##name##_p_result_init(prefix##_##name##_p_result_t *r) { \
        r->_value._##name = (type *)z_malloc(sizeof(type));                               \
    }                                                                                     \
                                                                                          \
    inline static void prefix##_##name##_p_result_free(prefix##_##name##_p_result_t *r) { \
        z_free(r->_value._##name);                                                        \
        r->_value._##name = NULL;                                                         \
    }

#define _ASSURE_RESULT(in_r, out_r, e) \
    if (in_r._tag == _Z_RES_ERR) {     \
        out_r._tag = _Z_RES_ERR;       \
        out_r._value._error = e;       \
        return out_r;                  \
    }

#define _ASSURE_P_RESULT(in_r, out_r, e) \
    if (in_r._tag == _Z_RES_ERR) {       \
        out_r->_tag = _Z_RES_ERR;        \
        out_r->_value._error = e;        \
        return;                          \
    }

#define _ASSURE_FREE_P_RESULT(in_r, out_r, e, name) \
    if (in_r._tag == _Z_RES_ERR) {                  \
        z_free(out_r->_value._##name);              \
        out_r->_tag = _Z_RES_ERR;                   \
        out_r->_value._error = e;                   \
        return;                                     \
    }

#define _ASSERT_RESULT(r, msg)  \
    if (r._tag == _Z_RES_ERR) { \
        _Z_ERROR(msg);          \
        _Z_ERROR("\n");         \
        exit(r._value._error);  \
    }

#define _ASSERT_P_RESULT(r, msg) \
    if (r._tag == _Z_RES_ERR) {  \
        _Z_ERROR(msg);           \
        _Z_ERROR("\n");          \
        exit(r._value._error);   \
    }

/*------------------ Internal Zenoh Results ------------------*/
#define _Z_RESULT_DECLARE(type, name) _RESULT_DECLARE(type, name, _z)
#define _Z_P_RESULT_DECLARE(type, name) _P_RESULT_DECLARE(type, name, _z)

/*------------------ Internal Zenoh Results ------------------*/
#define Z_RESULT_DECLARE(type, name) _RESULT_DECLARE(type, name, z)
#define Z_P_RESULT_DECLARE(type, name) _P_RESULT_DECLARE(type, name, z)

#endif /* ZENOH_PICO_UTILS_RESULT_H */
