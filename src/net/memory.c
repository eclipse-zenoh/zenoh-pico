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

#include "zenoh-pico/net/memory.h"

void _z_sample_clear(_z_sample_t *sample)
{
    _z_reskey_clear(&sample->_key);
    _z_bytes_clear(&sample->_value);
    _z_str_clear(sample->_encoding._suffix); // FIXME: call the z_encoding_clear
}

void _z_sample_free(_z_sample_t **sample)
{
    _z_sample_t *ptr = (_z_sample_t *)*sample;
    _z_sample_clear(ptr);

    free(ptr);
    *sample = NULL;
}

void _z_hello_clear(_z_hello_t *hello)
{
    if (hello->_pid.len > 0)
        _z_bytes_clear(&hello->_pid);
    if (_z_str_array_len(&hello->_locators) > 0)
        _z_str_array_clear(&hello->_locators);
}

void _z_reply_data_clear(_z_reply_data_t *reply_data)
{
    _z_sample_clear(&reply_data->_sample);
    _z_bytes_clear(&reply_data->_replier_id);
}

void _z_reply_data_free(_z_reply_data_t **reply_data)
{
    _z_reply_data_t *ptr = (_z_reply_data_t *)*reply_data;
    _z_reply_data_clear(ptr);

    free(ptr);
    *reply_data = NULL;
}
