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

#include <stdio.h>
#include "zenoh/private/logging.h"
#include "zenoh/codec.h"
#include "zenoh/net/codec.h"
#include "zenoh/net/property.h"

void zn_property_encode(z_iobuf_t* buf, const zn_property_t* m) {
  z_vle_encode(buf, m->id);
  z_uint8_array_encode(buf, &m->value);
}

void zn_property_decode_na(z_iobuf_t* buf, zn_property_result_t *r) {
  z_vle_result_t r_vle;
  z_uint8_array_result_t r_a8;
  r->tag = Z_OK_TAG;
  r_vle = z_vle_decode(buf);
  ASSURE_P_RESULT(r_vle, r, Z_VLE_PARSE_ERROR);
  r_a8 = z_uint8_array_decode(buf);
  ASSURE_P_RESULT(r_a8, r, Z_ARRAY_PARSE_ERROR);
  r->value.property.id = r_vle.value.vle;
  r->value.property.value = r_a8.value.uint8_array;
}
zn_property_result_t zn_property_decode(z_iobuf_t* buf) {
  zn_property_result_t r;
  zn_property_decode_na(buf, &r);
  return r;
}

void zn_properties_encode(z_iobuf_t *buf, const z_vec_t *ps) {
  zn_property_t *p;
  z_vle_t l = z_vec_length(ps);
  z_vle_encode(buf, l);
  for (unsigned int i = 0; i < l; ++i) {
    p = (zn_property_t *)z_vec_get(ps, i);
    zn_property_encode(buf, p);
  }
}

void
zn_temporal_property_encode(z_iobuf_t* buf, const zn_temporal_property_t* tp) {
  z_vle_encode(buf, tp->origin);
  z_vle_encode(buf, tp->period);
  z_vle_encode(buf, tp->duration);
}

void
zn_temporal_property_decode_na(z_iobuf_t* buf, zn_temporal_property_result_t *r) {
  r->tag = Z_OK_TAG;
  z_vle_result_t r_origin = z_vle_decode(buf);
  ASSURE_P_RESULT(r_origin, r, Z_VLE_PARSE_ERROR)
  z_vle_result_t r_period = z_vle_decode(buf);
  ASSURE_P_RESULT(r_period, r, Z_VLE_PARSE_ERROR)
  z_vle_result_t r_duration = z_vle_decode(buf);
  ASSURE_P_RESULT(r_duration, r, Z_VLE_PARSE_ERROR)

  r->value.temporal_property.origin = r_origin.value.vle;
  r->value.temporal_property.period = r_period.value.vle;
  r->value.temporal_property.duration = r_duration.value.vle;
}

zn_temporal_property_result_t
zn_temporal_property_decode(z_iobuf_t* buf) {
  zn_temporal_property_result_t r;
  zn_temporal_property_decode_na(buf, &r);
  return r;
}
