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

#ifndef ZENOH_PICO_PROTOCOL_ENCODING_H
#define ZENOH_PICO_PROTOCOL_ENCODING_H

#define Z_DATA_KIND_PUT 0
#define Z_DATA_KIND_PATCH 1
#define Z_DATA_KIND_DELETE 2
#define Z_DATA_KIND_DEFAULT Z_DATA_KIND_PUT

#define Z_ENCODING_NONE 0
#define Z_ENCODING_APP_OCTET_STREAM 1
#define Z_ENCODING_APP_CUSTOM 2
#define Z_ENCODING_TEXT_PLAIN 3
#define Z_ENCODING_APP_PROPERTIES 4
#define Z_ENCODING_APP_JSON 5
#define Z_ENCODING_APP_SQL 6
#define Z_ENCODING_APP_INTEGER 7
#define Z_ENCODING_APP_FLOAT 8
#define Z_ENCODING_APP_XML 9
#define Z_ENCODING_APP_XHTML_XML 10
#define Z_ENCODING_APP_X_WWW_FORM_URLENCODED 11
#define Z_ENCODING_TEXT_JSON 12
#define Z_ENCODING_TEXT_HTML 13
#define Z_ENCODING_TEXT_XML 14
#define Z_ENCODING_TEXT_CSS 15
#define Z_ENCODING_TEXT_CSV 16
#define Z_ENCODING_TEXT_JAVASCRIPT 17
#define Z_ENCODING_IMG_JPG 18
#define Z_ENCODING_IMG_PNG 19
#define Z_ENCODING_IMG_GIF 20

#define Z_ENCODING_DEFAULT Z_ENCODING_APP_OCTET_STREAM

#endif /* ZENOH_PICO_PROTOCOL_ENCODING_H */
