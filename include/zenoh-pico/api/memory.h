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

#ifndef ZENOH_PICO_MEMORY_API_H
#define ZENOH_PICO_MEMORY_API_H

#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/session/session.h"

/**
 * Free a :c:type:`zn_sample_t` contained key and value.
 *
 * Parameters:
 *     sample: The :c:type:`zn_sample_t` to free.
 */
void zn_sample_free(zn_sample_t sample);

/**
 * Free an array of :c:struct:`zn_hello_t` messages and it's contained :c:struct:`zn_hello_t` messages recursively.
 *
 * Parameters:
 *     strs: The array of :c:struct:`zn_hello_t` messages to free.
 */
void zn_hello_array_free(zn_hello_array_t hellos);

/**
 * Free a :c:type:`zn_reply_data_array_t` and it's contained replies.
 *
 * Parameters:
 *     replies: The :c:type:`zn_reply_data_array_t` to free.
 */
void zn_reply_data_array_free(zn_reply_data_array_t replies);

#endif /* ZENOH_PICO_MEMORY_API_H */
