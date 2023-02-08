//
// Copyright (c) 2022 NXP
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   Peter van der Perk, <peter.vanderperk@nxp.com>
//

#ifdef ZENOH_LINUX

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

void dds_log (uint32_t cat, const char *file, uint32_t line, const char *func, const char *fmt, ...)
{
#ifdef DEBUG
  va_list ap;
  va_start (ap, fmt);
  printf(fmt, ap);
  va_end (ap);
#endif
}

#endif