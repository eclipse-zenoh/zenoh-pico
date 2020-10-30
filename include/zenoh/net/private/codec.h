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

#ifndef ZENOH_C_NET_CODEC_H
#define ZENOH_C_NET_CODEC_H

#include <stdint.h>
#include "zenoh/net/property.h"
#include "zenoh/net/private/result.h"
#include "zenoh/private/iobuf.h"

/*------------------ Internal Zenoh-net Encoding/Decoding ------------------*/
int _zn_property_encode(_z_wbuf_t *wbf, const zn_property_t *m);
_zn_property_result_t _zn_property_decode(_z_rbuf_t *rbf);
void _zn_property_decode_na(_z_rbuf_t *rbf, _zn_property_result_t *r);

int _zn_properties_encode(_z_wbuf_t *wbf, const zn_properties_t *m);
_zn_properties_result_t _zn_properties_decode(_z_rbuf_t *rbf);
void _zn_properties_decode_na(_z_rbuf_t *rbf, _zn_properties_result_t *r);

int _zn_period_encode(_z_wbuf_t *wbf, const zn_period_t *m);
_zn_period_result_t _zn_period_decode(_z_rbuf_t *rbf);
void _zn_period_decode_na(_z_rbuf_t *rbf, _zn_period_result_t *r);

#endif /* ZENOH_C_NET_CODEC_H */
