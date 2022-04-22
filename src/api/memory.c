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

#include "zenoh-pico/api/memory.h"

void zn_sample_free(zn_sample_t sample)
{
    if (sample.key.val)
        _z_string_clear(&sample.key);
    if (sample.value.val)
        _z_bytes_clear(&sample.value);
}

void zn_hello_array_free(zn_hello_array_t hellos)
{
    zn_hello_t *h = (zn_hello_t *)hellos.val;
    if (h)
    {
        for (unsigned int i = 0; i < hellos.len; i++)
        {
            if (h[i].pid.len > 0)
                _z_bytes_clear(&h[i].pid);
            if (h[i].locators.len > 0)
                _z_str_array_free(&h[i].locators);
        }

        free(h);
    }
}

void zn_reply_data_array_free(zn_reply_data_array_t replies)
{
    for (unsigned int i = 0; i < replies.len; i++)
    {
        if (replies.val[i].replier_id.val)
            _z_bytes_clear((z_bytes_t *)&replies.val[i].replier_id);
        if (replies.val[i].data.value.val)
            _z_bytes_clear((z_bytes_t *)&replies.val[i].data.value);
        if (replies.val[i].data.key.val)
            _z_string_clear((z_string_t *)&replies.val[i].data.key);
    }
    free((zn_reply_data_t *)replies.val);
}
