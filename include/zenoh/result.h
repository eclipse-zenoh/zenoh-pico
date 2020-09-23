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

#ifndef ZENOH_C_RESULT_H
#define ZENOH_C_RESULT_H

#define Z_VLE_PARSE_ERROR 0x01
#define Z_ARRAY_PARSE_ERROR 0x02
#define Z_STRING_PARSE_ERROR 0x03

#define RESULT_DECLARE(type, name, prefix) \
typedef struct { \
  enum result_kind tag; \
  union { \
    type name; \
    int error; \
  } value;\
} prefix ## _ ## name ## _result_t;
#define Z_RESULT_DECLARE(type, name) RESULT_DECLARE(type, name, z)
#define _Z_RESULT_DECLARE(type, name) RESULT_DECLARE(type, name, _z)

#define P_RESULT_DECLARE(type, name, prefix) \
typedef struct { \
  enum result_kind tag; \
  union { \
    type * name; \
    int error; \
  } value;\
} prefix ## _ ## name ## _p_result_t; \
inline static void prefix ## _ ## name ## _p_result_init(prefix ## _ ## name ## _p_result_t *r) { \
     r->value.name = (type *)malloc(sizeof(type)); \
} \
inline static void prefix ## _ ## name ## _p_result_free(prefix ## _ ## name ## _p_result_t *r) { \
    free(r->value.name); \
    r->value.name = 0; \
}
#define Z_P_RESULT_DECLARE(type, name) P_RESULT_DECLARE(type, name, z)
#define _Z_P_RESULT_DECLARE(type, name) P_RESULT_DECLARE(type, name, _z)

#define ASSURE_RESULT(in_r, out_r, e) \
  if (in_r.tag == Z_ERROR_TAG) { \
    out_r.tag = Z_ERROR_TAG; \
    out_r.value.error = e; \
    return out_r; \
  }

#define ASSURE_P_RESULT(in_r, out_r, e) \
  if (in_r.tag == Z_ERROR_TAG) { \
    out_r->tag = Z_ERROR_TAG; \
    out_r->value.error = e; \
    return; \
  }
#define ASSERT_RESULT(r, msg) \
  if (r.tag == Z_ERROR_TAG) { \
    printf(msg); \
    printf("\n"); \
    exit(r.value.error); \
  }

#define ASSERT_P_RESULT(r, msg) \
  if (r.tag == Z_ERROR_TAG) { \
    printf(msg); \
    printf("\n"); \
    exit(r.value.error); \
  }

enum result_kind {
  Z_OK_TAG = 0,
  Z_ERROR_TAG = 1
};

#endif /* ZENOH_C_RESULT_H */
