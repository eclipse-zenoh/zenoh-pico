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

#ifndef _ZENOH_PICO_ENCODING_H
#define _ZENOH_PICO_ENCODING_H

#define Z_DATA_KIND_PUT 0
#define Z_DATA_KIND_PATCH 1
#define Z_DATA_KIND_DELETE 2
#define Z_DATA_KIND_DEFAULT Z_DATA_KIND_PUT

#define Z_ENCODING_APP_OCTET_STREAM 0
#define Z_ENCODING_NONE Z_ENCODING_APP_OCTET_STREAM
#define Z_ENCODING_APP_CUSTOM 1
#define Z_ENCODING_TEXT_PLAIN 2
#define Z_ENCODING_STRING Z_ENCODING_TEXT_PLAIN
#define Z_ENCODING_APP_PROPERTIES 3
#define Z_ENCODING_APP_JSON 4
#define Z_ENCODING_APP_SQL 5
#define Z_ENCODING_APP_INTEGER 6
#define Z_ENCODING_APP_FLOAT 7
#define Z_ENCODING_APP_XML 8
#define Z_ENCODING_APP_XHTML_XML 9
#define Z_ENCODING_APP_X_WWW_FORM_URLENCODED 10
#define Z_ENCODING_TEXT_JSON 11
#define Z_ENCODING_TEXT_HTML 12
#define Z_ENCODING_TEXT_XML 13
#define Z_ENCODING_TEXT_CSS 14
#define Z_ENCODING_TEXT_CSV 15
#define Z_ENCODING_TEXT_JAVASCRIPT 16
#define Z_ENCODING_IMG_JPG 17
#define Z_ENCODING_IMG_PNG 18
#define Z_ENCODING_IMG_GIF 19
#define Z_ENCODING_DEFAULT Z_ENCODING_APP_OCTET_STREAM

#endif /* _ZENOH_PICO_ENCODING_H */
