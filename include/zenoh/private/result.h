/*
 * Copyright (c) 2017, 2020 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#ifndef _ZENOH_PICO_RESULT_H
#define _ZENOH_PICO_RESULT_H

/*------------------ Result Enums ------------------*/
typedef enum _z_err_t
{
    _z_err_t_PARSE_ZINT,
    _z_err_t_PARSE_BYTES,
    _z_err_t_PARSE_STRING
} _z_err_t;

typedef enum _z_res_t
{
    _z_res_t_OK = 0,
    _z_res_t_ERR = -1
} _z_res_t;

/*------------------ Internal Result Macros ------------------*/
#define _RESULT_DECLARE(type, name, prefix) \
    typedef struct                          \
    {                                       \
        enum _z_res_t tag;                  \
        union                               \
        {                                   \
            type name;                      \
            int error;                      \
        } value;                            \
    } prefix##_##name##_result_t;

#define _P_RESULT_DECLARE(type, name, prefix)                                           \
    typedef struct                                                                      \
    {                                                                                   \
        enum _z_res_t tag;                                                              \
        union                                                                           \
        {                                                                               \
            type *name;                                                                 \
            int error;                                                                  \
        } value;                                                                        \
    } prefix##_##name##_p_result_t;                                                     \
                                                                                        \
    inline static void prefix##_##name##_p_result_init(prefix##_##name##_p_result_t *r) \
    {                                                                                   \
        r->value.name = (type *)malloc(sizeof(type));                                   \
    }                                                                                   \
                                                                                        \
    inline static void prefix##_##name##_p_result_free(prefix##_##name##_p_result_t *r) \
    {                                                                                   \
        free(r->value.name);                                                            \
        r->value.name = NULL;                                                           \
    }

#define _ASSURE_RESULT(in_r, out_r, e) \
    if (in_r.tag == _z_res_t_ERR)      \
    {                                  \
        out_r.tag = _z_res_t_ERR;      \
        out_r.value.error = e;         \
        return out_r;                  \
    }

#define _ASSURE_P_RESULT(in_r, out_r, e) \
    if (in_r.tag == _z_res_t_ERR)        \
    {                                    \
        out_r->tag = _z_res_t_ERR;       \
        out_r->value.error = e;          \
        return;                          \
    }

#define _ASSURE_FREE_P_RESULT(in_r, out_r, e, name) \
    if (in_r.tag == _z_res_t_ERR)                   \
    {                                               \
        free(out_r->value.name);                    \
        out_r->tag = _z_res_t_ERR;                  \
        out_r->value.error = e;                     \
        return;                                     \
    }

#define _ASSERT_RESULT(r, msg) \
    if (r.tag == _z_res_t_ERR) \
    {                          \
        printf(msg);           \
        printf("\n");          \
        exit(r.value.error);   \
    }

#define _ASSERT_P_RESULT(r, msg) \
    if (r.tag == _z_res_t_ERR)   \
    {                            \
        printf(msg);             \
        printf("\n");            \
        exit(r.value.error);     \
    }

/*------------------ Internal Zenoh Results ------------------*/
#define _Z_RESULT_DECLARE(type, name) _RESULT_DECLARE(type, name, _z)
#define _Z_P_RESULT_DECLARE(type, name) _P_RESULT_DECLARE(type, name, _z)

#endif /* _ZENOH_PICO_RESULT_H */
