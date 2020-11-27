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

#include <string.h>
#include "zenoh-pico/utils.h"

z_string_t z_string_make(const char *value)
{
    z_string_t s;
    s.val = strdup(value);
    s.len = strlen(value);
    return s;
}

void z_string_free(z_string_t *s)
{
    free((char *)s->val);
}
