//
// Copyright (c) 2023 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   hyojun.an <anhj2473@ivis.ai>

#ifndef LWIP_ARCH_CC_H
#define LWIP_ARCH_CC_H

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Define platform endianness */
#ifndef BYTE_ORDER
#define BYTE_ORDER LITTLE_ENDIAN
#endif

/* Define basic types used by lwIP */
typedef uint8_t u8_t;
typedef int8_t s8_t;
typedef uint16_t u16_t;
typedef int16_t s16_t;
typedef uint32_t u32_t;
typedef int32_t s32_t;
typedef uintptr_t mem_ptr_t;

/* Define (sn)printf formatters for these lwIP types */
#define U16_F "hu"
#define S16_F "hd"
#define X16_F "hx"
#define U32_F "u"
#define S32_F "d"
#define X32_F "x"
#define SZT_F "lu"

/* Compiler hints for packing structures */
#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_STRUCT __attribute__((packed))
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END

/* Platform specific diagnostic output */
#define LWIP_PLATFORM_DIAG(x) \
    do {                      \
        printf x;             \
    } while (0)

#define LWIP_PLATFORM_ASSERT(x)                                                      \
    do {                                                                             \
        printf("Assertion \"%s\" failed at line %d in %s\n", x, __LINE__, __FILE__); \
        fflush(NULL);                                                                \
        abort();                                                                     \
    } while (0)

/* Random number generator */
#define LWIP_RAND() ((u32_t)rand())

#endif /* LWIP_ARCH_CC_H */
