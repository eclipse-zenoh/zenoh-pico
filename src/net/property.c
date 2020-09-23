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

#include "zenoh/net/property.h"
#include <string.h>

zn_property_t* zn_property_make(z_vle_t id, z_uint8_array_t value) {
  zn_property_t* p = (zn_property_t*)malloc(sizeof(zn_property_t));
  p->id = id;
  p->value = value;
  return p;
}

zn_property_t* zn_property_make_from_str(z_vle_t id, char *str) {
  zn_property_t* p = (zn_property_t*)malloc(sizeof(zn_property_t));
  p->id = id;
  p->value.elem = (uint8_t *)str;
  p->value.length = strlen(str);
  return p;
}

void zn_property_free(zn_property_t** p) {
  free((*p));
  *p = 0;
  }
