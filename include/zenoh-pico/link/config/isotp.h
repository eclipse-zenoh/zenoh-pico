//
// Copyright (c) 2026 ZettaScale Technology
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

#ifndef ZENOH_PICO_LINK_CONFIG_ISOTP_H
#define ZENOH_PICO_LINK_CONFIG_ISOTP_H

#include "zenoh-pico/collections/intmap.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/result.h"

#ifdef __cplusplus
extern "C" {
#endif

#if Z_FEATURE_LINK_ISOTP == 1

// Endpoint: isotp/<device>#tx_id=<n>;rx_id=<n>;eff=<0|1>
//
//   device  the CAN interface name ("can0", "vcan0", or on Zephyr a devicetree
//           node label)
//   tx_id   the identifier THIS peer transmits its PDUs on
//   rx_id   the identifier it receives on, and on which it sends flow control
//   eff     1 for 29-bit extended identifiers; default 0, meaning 11-bit
//   stmin   separation time this peer asks the other to leave between the
//           ConsecutiveFrames it sends us. 0x00..0x7F is milliseconds,
//           0xF1..0xF9 is 100us..900us. Default 0, meaning "no wait".
//   bs      how many ConsecutiveFrames the other side may send before it must
//           wait for another flow control frame from us. Default 0, meaning
//           "send the whole message without pausing".
//
// stmin and bs are what a node with little memory or a slow loop uses to keep
// a faster peer from overrunning it. They travel in the flow control frame
// THIS peer sends, so they constrain the other side, not this one.
//
// The two identifiers are a DIRECTED PAIR and must differ: one identifier for
// both directions would mean a peer receiving its own PDUs and answering its
// own flow control.
//
// Identifier value is bus priority -- a LOWER identifier wins arbitration -- so
// these are real-time decisions rather than names. The peer that must not be
// delayed needs the lower identifier.

#define ISOTP_CONFIG_ARGC 5

#define ISOTP_CONFIG_TX_ID_KEY 0x01
#define ISOTP_CONFIG_TX_ID_STR "tx_id"

#define ISOTP_CONFIG_RX_ID_KEY 0x02
#define ISOTP_CONFIG_RX_ID_STR "rx_id"

#define ISOTP_CONFIG_EFF_KEY 0x03
#define ISOTP_CONFIG_EFF_STR "eff"

#define ISOTP_CONFIG_STMIN_KEY 0x04
#define ISOTP_CONFIG_STMIN_STR "stmin"

#define ISOTP_CONFIG_BS_KEY 0x05
#define ISOTP_CONFIG_BS_STR "bs"

#define ISOTP_CONFIG_MAPPING_BUILD                 \
    _z_str_intmapping_t args[ISOTP_CONFIG_ARGC];   \
    args[0]._key = ISOTP_CONFIG_TX_ID_KEY;         \
    args[0]._str = (char *)ISOTP_CONFIG_TX_ID_STR; \
    args[1]._key = ISOTP_CONFIG_RX_ID_KEY;         \
    args[1]._str = (char *)ISOTP_CONFIG_RX_ID_STR; \
    args[2]._key = ISOTP_CONFIG_EFF_KEY;           \
    args[2]._str = (char *)ISOTP_CONFIG_EFF_STR;   \
    args[3]._key = ISOTP_CONFIG_STMIN_KEY;         \
    args[3]._str = (char *)ISOTP_CONFIG_STMIN_STR; \
    args[4]._key = ISOTP_CONFIG_BS_KEY;            \
    args[4]._str = (char *)ISOTP_CONFIG_BS_STR;

size_t _z_isotp_config_strlen(const _z_str_intmap_t *s);
void _z_isotp_config_onto_str(char *dst, size_t dst_len, const _z_str_intmap_t *s);
char *_z_isotp_config_to_str(const _z_str_intmap_t *s);
z_result_t _z_isotp_config_from_str(_z_str_intmap_t *strint, const char *s);
z_result_t _z_isotp_config_from_strn(_z_str_intmap_t *strint, const char *s, size_t n);

#endif

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_LINK_CONFIG_ISOTP_H */
