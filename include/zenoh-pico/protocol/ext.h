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

#ifndef ZENOH_PICO_PROTOCOL_EXT_H
#define ZENOH_PICO_PROTOCOL_EXT_H

#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/iobuf.h"

/*=============================*/
/*        Extension IDs        */
/*=============================*/
#define _ZN_EID_SCOUT 0x11

/*=============================*/
/*       Extension flags       */
/*=============================*/
// Extension flags:
//      F Forward          If F==1 then the extension needs to be forwarded. (*)
//      Z More             If Z==1 then another extension will follow.
//      X None             Non-attributed flags are set to zero
//
//      (*) If the zenoh extension is not understood, then it SHOULD NOT be dropped and it
//          SHOULD be forwarded to the next hops.

#define _ZN_FLAG_EXT_F          0x40  // 1 << 6
#define _ZN_FLAG_EXT_Z          0x80  // 1 << 7

#define _ZN_FLAG_EXT_X          0x00  // 0

/*=============================*/
/*      Extension header       */
/*=============================*/
#define _ZN_EID_MASK 0x1f
#define _ZN_EFLAGS_MASK 0xe0

/*=============================*/
/*      Extension helpers      */
/*=============================*/
#define _ZN_EID(h) (_ZN_EID_MASK & h)
#define _ZN_EFLAGS(h) (_ZN_EFLAGS_MASK & h)
#define _ZN_HAS_EFLAG(h, f) ((h & f) != 0)
#define _ZN_SET_EFLAG(h, f) (h |= f)

/*------------------ Zenoh Extension ------------------*/
// A zenoh extension is encoded as TLV (Type, Length, Value).
// Zenoh extensions with unknown IDs (i.e., type) can be skipped by reading the length and
// not decoding the body (i.e. value). In case the zenoh extension is unknown, it is
// still possible to forward it to the next hops, which in turn may be able to understand it.
// This results in the capability of introducing new extensions in an already running system
// without requiring the redeployment of the totatly of infrastructure nodes.
//
// The zenoh extension wire format is the following:
//
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |Z|F|X|   EID   |
// +-+-+-+---------+
// ~      len      ~
// +---------------+
// ~      [u8]     ~
// +---------------+
//
typedef union
{
} _zn_ext_body_t;

typedef struct
{
    _zn_ext_body_t body;
    uint8_t header;
} _zn_transport_extension_t;
void _zn_t_ext_clear(_zn_transport_extension_t *m);
_Z_ELEM_DEFINE(_zn_extension, _zn_transport_extension_t, _zn_noop_size, _zn_t_ext_clear, _zn_noop_copy)
_Z_VEC_DEFINE(_zn_extension, _zn_transport_extension_t)

/*------------------ Builders ------------------*/
// TODO

/*-------------------- Copy --------------------*/
void _zn_t_ext_copy(_zn_transport_extension_t *clone, _zn_transport_extension_t *ext);

/*------------------- Helpers ------------------*/
void _zn_t_ext_skip(_z_zbuf_t *zbf);

#endif /* ZENOH_PICO_PROTOCOL_EXT_H */
