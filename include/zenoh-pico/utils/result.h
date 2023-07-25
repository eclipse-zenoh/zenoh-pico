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

#ifndef ZENOH_PICO_UTILS_RESULT_H
#define ZENOH_PICO_UTILS_RESULT_H

#define _Z_ERR_MESSAGE_MASK 0x88
#define _Z_ERR_ENTITY_MASK 0x90
#define _Z_ERR_TRANSPORT_MASK 0x98
#define _Z_ERR_CONFIG_MASK 0xa0
#define _Z_ERR_SCOUT_MASK 0xa8
#define _Z_ERR_SYSTEM_MASK 0xb0
#define _Z_ERR_GENERIC_MASK 0xb8

/*------------------ Result Enums ------------------*/
typedef enum {
    _Z_RES_OK = 0,

    _Z_ERR_MESSAGE_DESERIALIZATION_FAILED = -119,
    _Z_ERR_MESSAGE_SERIALIZATION_FAILED = -118,
    _Z_ERR_MESSAGE_UNEXPECTED = -117,
    _Z_ERR_MESSAGE_FLAG_UNEXPECTED = -116,
    _Z_ERR_MESSAGE_ZENOH_DECLARATION_UNKNOWN = -115,
    _Z_ERR_MESSAGE_ZENOH_UNKNOWN = -114,
    _Z_ERR_MESSAGE_TRANSPORT_UNKNOWN = -113,

    _Z_ERR_ENTITY_DECLARATION_FAILED = -111,
    _Z_ERR_ENTITY_UNKNOWN = -110,
    _Z_ERR_KEYEXPR_UNKNOWN = -109,
    _Z_ERR_KEYEXPR_NOT_MATCH = -108,
    _Z_ERR_QUERY_NOT_MATCH = -107,

    _Z_ERR_TRANSPORT_NOT_AVAILABLE = -103,
    _Z_ERR_TRANSPORT_OPEN_FAILED = -102,
    _Z_ERR_TRANSPORT_OPEN_SN_RESOLUTION = -101,
    _Z_ERR_TRANSPORT_TX_FAILED = -100,
    _Z_ERR_TRANSPORT_RX_FAILED = -99,
    _Z_ERR_TRANSPORT_NO_SPACE = -98,
    _Z_ERR_TRANSPORT_NOT_ENOUGH_BYTES = -97,

    _Z_ERR_CONFIG_FAILED_INSERT = -95,
    _Z_ERR_CONFIG_UNSUPPORTED_CLIENT_MULTICAST = -94,
    _Z_ERR_CONFIG_UNSUPPORTED_PEER_UNICAST = -93,
    _Z_ERR_CONFIG_LOCATOR_SCHEMA_UNKNOWN = -92,
    _Z_ERR_CONFIG_LOCATOR_INVALID = -91,
    _Z_ERR_CONFIG_INVALID_MODE = -90,

    _Z_ERR_SCOUT_NO_RESULTS = -87,

    _Z_ERR_SYSTEM_TASK_FAILED = -79,
    _Z_ERR_SYSTEM_OUT_OF_MEMORY = -78,

    _Z_ERR_GENERIC = -128
} _z_res_t;

#endif /* ZENOH_PICO_UTILS_RESULT_H */
