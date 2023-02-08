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
#include <string.h>

void *ddsrt_calloc(size_t nitems, size_t size) {
    return calloc(nitems, size);
}

void *dds_realloc(void *ptr, size_t size) {
    return realloc(ptr, size);
}

void *ddsrt_realloc(void *ptr, size_t size) {
    return realloc(ptr, size);
}

void *ddsrt_malloc(size_t size) {
    return malloc(size);
}

void *ddsrt_malloc_s(size_t size) {
  return malloc(size ? size : 1);
}

void *ddsrt_memdup(const void *src, size_t n) {
  void *dest = NULL;

  if (n != 0 && (dest = ddsrt_malloc_s(n)) != NULL) {
    memcpy(dest, src, n);
  }

  return dest;
}


void ddsrt_free(void *ptr) {
    free(ptr);
}

void dds_free(void *ptr) {
    free(ptr);
}

#endif
