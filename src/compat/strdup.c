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

#include <stdlib.h>
#include <string.h>

#include "zenoh-pico/system/private/compat/strdup.h"

char *strdup(const char *s)
{
    char *result = malloc(strlen(s) + 1);

    if (result)
    {
        strcpy(result, s);
    }

    return result;
}
