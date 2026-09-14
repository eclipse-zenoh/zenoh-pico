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

#ifndef ZENOH_PICO_LINK_CONFIG_CAN_H
#define ZENOH_PICO_LINK_CONFIG_CAN_H

#include "zenoh-pico/collections/intmap.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/result.h"

#ifdef __cplusplus
extern "C" {
#endif

#if Z_FEATURE_LINK_CAN == 1

// Endpoint: can/<device>#bitrate=<n>;dbitrate=<n>;id=<n>;match=<n>;mask=<n>
//
//   device   the CAN interface name ("can0", "vcan0", or on Zephyr the
//            devicetree node label)
//   bitrate  arbitration-phase bit rate; also the sole rate for classic CAN
//   dbitrate CAN FD data-phase bit rate. 0 selects classic CAN (8-byte frames)
//   id       THIS peer's identifier. It transmits on this, and every other
//            peer sees it as this peer's address.
//   match    accept frames whose (id & mask) == match
//   mask     0 (the default) accepts every identifier on the bus
//
// A CAN bus is a broadcast medium, so the link is MULTICAST: peers do not pair
// off, they all listen and filter. `match`/`mask` exist so zenoh can share a
// bus with ordinary vehicle traffic by claiming an identifier band rather than
// the whole bus.
//
// Identifier value IS bus priority on CAN -- a LOWER identifier wins arbitration
// -- so `id` is a real-time decision, not a name.
//
// The peer that must not be delayed needs the lower identifier. On a safety
// island that is the node publishing the stop command, NOT the one publishing
// bulk telemetry: if the telemetry peer holds the lower id, its multi-frame
// bursts outrank every urgent message on the bus.
//
// The defaults below are a STARTING POINT, not an allocation. Two peers that
// both accept them differ only by whoever was configured first, which is a
// priority ordering nobody chose. Allocate them deliberately.
//
// Note also that priority here is per PEER, not per message: one identifier
// carries all of a peer's traffic, and zenoh-pico batches a link FIFO with no
// per-priority split, so a peer's own urgent message cannot overtake its own
// bulk one.

#define CAN_CONFIG_ARGC 5

#define CAN_CONFIG_BITRATE_KEY 0x01
#define CAN_CONFIG_BITRATE_STR "bitrate"

#define CAN_CONFIG_DBITRATE_KEY 0x02
#define CAN_CONFIG_DBITRATE_STR "dbitrate"

#define CAN_CONFIG_ID_KEY 0x03
#define CAN_CONFIG_ID_STR "id"

#define CAN_CONFIG_MATCH_KEY 0x04
#define CAN_CONFIG_MATCH_STR "match"

#define CAN_CONFIG_MASK_KEY 0x05
#define CAN_CONFIG_MASK_STR "mask"

#define CAN_CONFIG_MAPPING_BUILD                    \
    _z_str_intmapping_t args[CAN_CONFIG_ARGC];      \
    args[0]._key = CAN_CONFIG_BITRATE_KEY;          \
    args[0]._str = (char *)CAN_CONFIG_BITRATE_STR;  \
    args[1]._key = CAN_CONFIG_DBITRATE_KEY;         \
    args[1]._str = (char *)CAN_CONFIG_DBITRATE_STR; \
    args[2]._key = CAN_CONFIG_ID_KEY;               \
    args[2]._str = (char *)CAN_CONFIG_ID_STR;       \
    args[3]._key = CAN_CONFIG_MATCH_KEY;            \
    args[3]._str = (char *)CAN_CONFIG_MATCH_STR;    \
    args[4]._key = CAN_CONFIG_MASK_KEY;             \
    args[4]._str = (char *)CAN_CONFIG_MASK_STR;

// Defaults when a key is absent from the endpoint.
#define CAN_CONFIG_DEFAULT_BITRATE 500000u
#define CAN_CONFIG_DEFAULT_DBITRATE 2000000u
#define CAN_CONFIG_DEFAULT_ID 0x100u
#define CAN_CONFIG_DEFAULT_MATCH 0u
#define CAN_CONFIG_DEFAULT_MASK 0u

size_t _z_can_config_strlen(const _z_str_intmap_t *s);
void _z_can_config_onto_str(char *dst, size_t dst_len, const _z_str_intmap_t *s);
char *_z_can_config_to_str(const _z_str_intmap_t *s);
z_result_t _z_can_config_from_str(_z_str_intmap_t *strint, const char *s);
z_result_t _z_can_config_from_strn(_z_str_intmap_t *strint, const char *s, size_t n);

#endif

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_LINK_CONFIG_CAN_H */
