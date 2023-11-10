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

#ifndef UNICAST_H
#define UNICAST_H

#include "zenoh-pico/api/types.h"

bool _zp_is_unicast_here(void);

void _zp_unicast_fetch_zid(const _z_transport_t *zt, z_owned_closure_zid_t *callback);
void _zp_unicast_info_session(const _z_transport_t *zt, _z_config_t *ps);

int _zp_unicast_start_read_task(_z_transport_t *zt, _z_task_attr_t *attr, _z_task_t *task);
int _zp_unicast_start_lease_task(_z_transport_t *zt, _z_task_attr_t *attr, _z_task_t *task);
int _zp_unicast_stop_read_task(_z_transport_t *zt);
int _zp_unicast_stop_lease_task(_z_transport_t *zt);

int8_t _z_unicast_transport_create(_z_transport_t *zt, _z_link_t *zl, _z_transport_unicast_establish_param_t *param);
int8_t _z_unicast_open_client(_z_transport_unicast_establish_param_t *param, const _z_link_t *zl,
                              const _z_id_t *local_zid);
int8_t _z_unicast_open_peer(_z_transport_unicast_establish_param_t *param, const _z_link_t *zl,
                            const _z_id_t *local_zid);
int8_t _z_unicast_send_close(_z_transport_unicast_t *ztu, uint8_t reason, _Bool link_only);
int8_t _z_unicast_transport_close(_z_transport_unicast_t *ztu, uint8_t reason);
void _z_unicast_transport_clear(_z_transport_t *zt);
#endif /* UNICAST_H */