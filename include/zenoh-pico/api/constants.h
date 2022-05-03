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

#ifndef ZENOH_PICO_API_CONSTANTS_H
#define ZENOH_PICO_API_CONSTANTS_H

typedef enum
{
    Z_ENCODING_EMPTY = 0,
    Z_ENCODING_APP_OCTETSTREAM = 1,
    Z_ENCODING_APP_CUSTOM = 2,
    Z_ENCODING_TEXTPLAIN = 3,
    Z_ENCODING_APP_PROPERTIES = 4,
    Z_ENCODING_APP_JSON = 5,
    Z_ENCODING_APP_SQL = 6,
    Z_ENCODING_APP_INTEGER = 7,
    Z_ENCODING_APP_FLOAT = 8,
    Z_ENCODING_APP_XML = 9,
    Z_ENCODING_APP_XHTMLXML = 10,
    Z_ENCODING_APP_XWWWFORMURLENCODED = 11,
    Z_ENCODING_TEXT_JSON = 12,
    Z_ENCODING_TEXT_HTML = 13,
    Z_ENCODING_TEXT_XML = 14,
    Z_ENCODING_TEXT_CSS = 15,
    Z_ENCODING_TEXT_CSV = 16,
    Z_ENCODING_TEXT_JAVASCRIPT = 17,
    Z_ENCODING_IMAGE_JPEG = 18,
    Z_ENCODING_IMAGE_PNG = 19,
    Z_ENCODING_IMAGE_GIF = 20,
} z_encoding_type_t;

/**
 * The kind of consolidation that should be applied on replies to a :c:func:`z_query`.
 *
 *     - **Z_CONSOLIDATION_MODE_FULL**: Guaranties unicity of replies. Optimizes bandwidth.
 *     - **Z_CONSOLIDATION_MODE_LAZY**: Does not garanty unicity. Optimizes latency.
 *     - **Z_CONSOLIDATION_MODE_NONE**: No consolidation.
 */
typedef enum
{
    Z_CONSOLIDATION_MODE_NONE = 0,
    Z_CONSOLIDATION_MODE_LAZY = 1,
    Z_CONSOLIDATION_MODE_FULL = 2,
} z_consolidation_mode_t;

/**
 * The subscription reliability.
 *
 *     - **Z_RELIABILITY_BEST_EFFORT**
 *     - **Z_RELIABILITY_RELIABLE**
 */
typedef enum
{
    Z_RELIABILITY_BEST_EFFORT = 0,
    Z_RELIABILITY_RELIABLE = 1,
} z_reliability_t;

typedef enum
{
    Z_QUERY_CONSOLIDATION_AUTO = 0,
    Z_QUERY_CONSOLIDATION_MANUAL = 1,
} z_query_consolidation_tag_t;

/**
 * The possible values of :c:member:`_z_reply_t.tag`
 *
 *     - **Z_REPLY_TAG_DATA**: The reply contains some data.
 *     - **Z_REPLY_TAG_FINAL**: The reply does not contain any data and indicates that there will be no more replies for this query.
 */
typedef enum
{
    Z_REPLY_TAG_DATA = 0,
    Z_REPLY_TAG_FINAL = 1,
} z_reply_tag_t;

#define Z_PERIOD_NONE (z_period_t){.origin = 0, .period = 0, .duration = 0}

typedef enum
{
    Z_PRIORITY_CONTROL = 0,
    Z_PRIORITY_REALTIME = 1,
    Z_PRIORITY_INTERACTIVE_HIGH = 2,
    Z_PRIORITY_INTERACTIVE_LOW = 3,
    Z_PRIORITY_DATA_HIGH = 4,
    Z_PRIORITY_DATA = 5,
    Z_PRIORITY_DATA_LOW = 6,
    Z_PRIORITY_BACKGROUND = 7,
} z_priority_t;

#endif /* ZENOH_PICO_API_CONSTANTS_H */
