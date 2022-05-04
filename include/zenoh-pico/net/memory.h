/*
 * Copyright (c) 2017, 2021 ADLINK Technology Inc.
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

#ifndef ZENOH_PICO_MEMORY_NETAPI_H
#define ZENOH_PICO_MEMORY_NETAPI_H

#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/session/session.h"

/**
 * Free a :c:type:`_z_sample_t`, including its internal fields.
 *
 * Parameters:
 *     sample: The :c:type:`_z_sample_t` to free.
 */
void _z_sample_move(_z_sample_t *dst, _z_sample_t *src);
void _z_sample_clear(_z_sample_t *sample);
void _z_sample_free(_z_sample_t **sample);

/**
 * Free an array of :c:type:`_z_t_msg_hello_t` messages and it's contained :c:type:`_z_t_msg_hello_t` messages recursively.
 *
 * Parameters:
 *     strs: The array of :c:type:`_z_t_msg_hello_t` messages to free.
 */
void _z_hello_array_clear(_z_hello_array_t *hellos);
void _z_hello_array_free(_z_hello_array_t **hellos);

/**
 * Free a :c:type:`_z_reply_data_t`, including its internal fields.
 *
 * Parameters:
 *     replies: The :c:type:`_z_reply_data_t` to free.
 */
void _z_reply_data_clear(_z_reply_data_t *reply_data);
void _z_reply_data_free(_z_reply_data_t **reply_data);

/**
 * Free a :c:type:`_z_reply_data_array_t` and it's contained replies.
 *
 * Parameters:
 *     replies: The :c:type:`_z_reply_data_array_t` to free.
 */
void _z_reply_data_array_clear(_z_reply_data_array_t *replies);
void _z_reply_data_array_free(_z_reply_data_array_t **replies);

#endif /* ZENOH_PICO_MEMORY_NETAPI_H */
