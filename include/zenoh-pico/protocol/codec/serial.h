//
// Copyright (c) 2024 ZettaScale Technology
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

#ifndef INCLUDE_ZENOH_PICO_PROTOCOL_CODEC_SERIAL_H
#define INCLUDE_ZENOH_PICO_PROTOCOL_CODEC_SERIAL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t _z_serial_msg_serialize(uint8_t *dest, size_t dest_len, const uint8_t *src, size_t src_len, uint8_t header,
                               uint8_t *tmp_buf, size_t tmp_buf_len);
size_t _z_serial_msg_deserialize(const uint8_t *src, size_t src_len, uint8_t *dst, size_t dst_len, uint8_t *header,
                                 uint8_t *tmp_buf, size_t tmp_buf_len);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_ZENOH_PICO_PROTOCOL_CODEC_SERIAL_H */
