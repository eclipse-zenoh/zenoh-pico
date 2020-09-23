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

#ifndef ZENOH_C_NET_RESULT_H
#define ZENOH_C_NET_RESULT_H

#define ZN_PROPERTY_PARSE_ERROR 0x81
#define ZN_PROPERTIES_PARSE_ERROR 0x82
#define ZN_MESSAGE_PARSE_ERROR 0x83
#define ZN_INSUFFICIENT_IOBUF_SIZE 0x84
#define ZN_IO_ERROR 0x85
#define ZN_RESOURCE_DECL_ERROR 0x86
#define ZN_PAYLOAD_HEADER_PARSE_ERROR 0x87
#define ZN_TX_CONNECTION_ERROR 0x89
#define ZN_INVALID_ADDRESS_ERROR 0x8a
#define ZN_FAILED_TO_OPEN_SESSION 0x8b
#define ZN_UNEXPECTED_MESSAGE 0x8c

#define ZN_RESULT_DECLARE(type, name) RESULT_DECLARE(type, name, zn)

#define _ZN_RESULT_DECLARE(type, name) RESULT_DECLARE(type, name, _zn)

#define ZN_P_RESULT_DECLARE(type, name) P_RESULT_DECLARE(type, name, zn)

#define _ZN_P_RESULT_DECLARE(type, name) P_RESULT_DECLARE(type, name, _zn)

#endif /* ZENOH_C_NET_RESULT_H */
