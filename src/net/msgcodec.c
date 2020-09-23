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

#include <stdio.h>
#include "zenoh/private/logging.h"
#include "zenoh/net/private/msgcodec.h"

void _zn_payload_encode(z_iobuf_t *buf, const z_iobuf_t *bs) {
  z_iobuf_write_slice(buf, bs->buf, bs->r_pos, bs->w_pos);
}

z_iobuf_t _zn_payload_decode(z_iobuf_t *buf) {
  z_vle_t len = z_iobuf_readable(buf);
  uint8_t *bs = z_iobuf_read_n(buf, len);
  z_iobuf_t iob = z_iobuf_wrap_wo(bs, len, 0, len);
  return iob;
}

void
_zn_scout_encode(z_iobuf_t *buf, const _zn_scout_t *m) {
  z_iobuf_write(buf, _ZN_SCOUT);
  z_vle_encode(buf, m->mask);
}

void
z_scout_decode_na(z_iobuf_t *buf, _zn_scout_result_t *r) {
  r->tag = Z_OK_TAG;
  z_vle_result_t r_vle = z_vle_decode(buf);
  ASSURE_P_RESULT(r_vle, r, Z_VLE_PARSE_ERROR)
  r->value.scout.mask = r_vle.value.vle;
}

_zn_scout_result_t
z_scout_decode(z_iobuf_t *buf) {
  _zn_scout_result_t r;
  z_scout_decode_na(buf, &r);
  return r;
}

void
_zn_hello_encode(z_iobuf_t *buf, const _zn_hello_t *msg) {
  z_iobuf_write(buf, _ZN_HELLO);
  z_iobuf_write(buf, msg->mask);
  unsigned int len = z_vec_length(&msg->locators);
  z_vle_encode(buf, len);
  for (unsigned int i = 0; i < len; ++i)
    z_string_encode(buf, z_vec_get(&msg->locators, i));

}
void
_zn_hello_decode_na(z_iobuf_t *buf, _zn_hello_result_t *r) {
  z_vle_result_t r_mask;
  z_vle_result_t r_n;
  z_string_result_t r_l;
  r->tag = Z_OK_TAG;
  r->value.hello.locators.elem_ = 0;
  r->value.hello.mask = 0;
  r_mask = z_vle_decode(buf);
  ASSURE_P_RESULT(r_mask, r, Z_VLE_PARSE_ERROR)
  r->value.hello.mask = r_mask.value.vle;
  r_n = z_vle_decode(buf);
  ASSURE_P_RESULT(r_n, r, Z_VLE_PARSE_ERROR)
  int len = r_n.value.vle;
  r->value.hello.locators = z_vec_make(len);
  for (int i = 0; i < len; ++i) {
    r_l = z_string_decode(buf);
    ASSURE_P_RESULT(r_l, r, Z_STRING_PARSE_ERROR)
    z_vec_append(&r->value.hello.locators, r_l.value.string);
  }
}
_zn_hello_result_t
z_hello_decode(z_iobuf_t *buf) {
  _zn_hello_result_t r;
  _zn_hello_decode_na(buf, &r);
  return r;
}

void
_zn_open_encode(z_iobuf_t* buf, const _zn_open_t* m) {
  z_iobuf_write(buf, m->version);
  z_uint8_array_encode(buf, &(m->pid));
  z_vle_encode(buf, m->lease);
  z_vle_encode(buf, 0); // no locators
  // TODO: Encode properties if present
}

void
_zn_accept_decode_na(z_iobuf_t* buf, _zn_accept_result_t *r) {

  r->tag = Z_OK_TAG;

  z_uint8_array_result_t r_cpid = z_uint8_array_decode(buf);
  ASSURE_P_RESULT(r_cpid, r, Z_ARRAY_PARSE_ERROR)

  z_uint8_array_result_t r_bpid = z_uint8_array_decode(buf);
  ASSURE_P_RESULT(r_bpid, r, Z_ARRAY_PARSE_ERROR)

  z_vle_result_t r_vle = z_vle_decode(buf);
  ASSURE_P_RESULT(r_vle, r, Z_VLE_PARSE_ERROR)

  // TODO: Decode Properties

  r->value.accept.client_pid = r_cpid.value.uint8_array;
  r->value.accept.broker_pid = r_bpid.value.uint8_array;
  r->value.accept.lease = r_vle.value.vle;
}

_zn_accept_result_t
_zn_accept_decode(z_iobuf_t* buf) {
  _zn_accept_result_t r;
  _zn_accept_decode_na(buf, &r);
  return r;
}

void
_zn_close_encode(z_iobuf_t* buf, const _zn_close_t* m) {
  z_uint8_array_encode(buf, &(m->pid));
  z_iobuf_write(buf, m->reason);
}

void
_zn_close_decode_na(z_iobuf_t* buf, _zn_close_result_t *r) {
  r->tag = Z_OK_TAG;
  z_uint8_array_result_t ar =  z_uint8_array_decode(buf);
  ASSURE_P_RESULT(ar, r, Z_ARRAY_PARSE_ERROR)
  r->value.close.pid = ar.value.uint8_array;
  r->value.close.reason = z_iobuf_read(buf);
}

_zn_close_result_t
_zn_close_decode(z_iobuf_t* buf) {
  _zn_close_result_t r;
  _zn_close_decode_na(buf, &r);
  return r;
}

void
_zn_sub_mode_encode(z_iobuf_t* buf, const zn_sub_mode_t* m) {
  z_iobuf_write(buf, m->kind);
  switch (m->kind) {
    case ZN_PERIODIC_PULL_MODE:
    case ZN_PERIODIC_PUSH_MODE:
      zn_temporal_property_encode(buf, &m->tprop);
      break;
    default:
      break;
  }
}

void
_zn_sub_mode_decode_na(z_iobuf_t* buf, zn_sub_mode_result_t *r) {
  zn_temporal_property_result_t r_tp;
  r->tag = Z_OK_TAG;
  r->value.sub_mode.kind = z_iobuf_read(buf);
  switch (r->value.sub_mode.kind) {
    case ZN_PERIODIC_PULL_MODE:
    case ZN_PERIODIC_PUSH_MODE:
      r_tp = zn_temporal_property_decode(buf);
      ASSURE_P_RESULT(r_tp, r, ZN_MESSAGE_PARSE_ERROR)
      r->value.sub_mode.tprop = r_tp.value.temporal_property;
      break;
    default:
      break;
  }
}

zn_sub_mode_result_t
_zn_sub_mode_decode(z_iobuf_t* buf) {
  zn_sub_mode_result_t r;
  _zn_sub_mode_decode_na(buf, &r);
  return r;
}

void
_zn_declaration_encode(z_iobuf_t *buf, _zn_declaration_t *d) {
  z_iobuf_write(buf, d->header);
  switch (_ZN_MID(d->header)) {
    case _ZN_RESOURCE_DECL:
      z_vle_encode(buf, d->payload.resource.rid);
      z_string_encode(buf, d->payload.resource.r_name);
      break;
    case _ZN_PUBLISHER_DECL:
      z_vle_encode(buf, d->payload.pub.rid);
      break;
    case _ZN_STORAGE_DECL:
      z_vle_encode(buf, d->payload.storage.rid);
      break;
    case _ZN_EVAL_DECL:
      z_vle_encode(buf, d->payload.eval.rid);
      break;
    case _ZN_SUBSCRIBER_DECL:
      z_vle_encode(buf, d->payload.sub.rid);
      _zn_sub_mode_encode(buf, &d->payload.sub.sub_mode);
      break;
    case _ZN_FORGET_PUBLISHER_DECL:
      z_vle_encode(buf, d->payload.forget_pub.rid);
      break;
    case _ZN_FORGET_STORAGE_DECL:
      z_vle_encode(buf, d->payload.forget_sto.rid);
      break;
    case _ZN_FORGET_EVAL_DECL:
      z_vle_encode(buf, d->payload.forget_eval.rid);
      break;
    case _ZN_FORGET_SUBSCRIBER_DECL:
      z_vle_encode(buf, d->payload.forget_sub.rid);
      break;
    case _ZN_RESULT_DECL:
      z_iobuf_write(buf, d->payload.result.cid);
      z_iobuf_write(buf, d->payload.result.status);
      break;
    case _ZN_COMMIT_DECL:
      z_iobuf_write(buf, d->payload.commit.cid);
      break;
    default:
      break;
  }
}

void
_zn_declaration_decode_na(z_iobuf_t *buf, _zn_declaration_result_t *r) {
  z_vle_result_t r_vle;
  z_string_result_t r_str;
  zn_sub_mode_result_t r_sm;
  r->tag = Z_OK_TAG;
  r->value.declaration.header = z_iobuf_read(buf);
  switch (_ZN_MID(r->value.declaration.header)) {
    case _ZN_RESOURCE_DECL:
      r_vle = z_vle_decode(buf);
      ASSURE_P_RESULT(r_vle, r, Z_VLE_PARSE_ERROR)
      r_str = z_string_decode(buf);
      ASSURE_P_RESULT(r_str, r, Z_STRING_PARSE_ERROR)
      r->value.declaration.payload.resource.rid = r_vle.value.vle;
      r->value.declaration.payload.resource.r_name = r_str.value.string;
      break;
    case _ZN_PUBLISHER_DECL:
      r_vle = z_vle_decode(buf);
      ASSURE_P_RESULT(r_vle, r, Z_VLE_PARSE_ERROR)
      r->value.declaration.payload.pub.rid = r_vle.value.vle;
      break;
    case _ZN_STORAGE_DECL:
      r_vle = z_vle_decode(buf);
      ASSURE_P_RESULT(r_vle, r, Z_VLE_PARSE_ERROR)
      r->value.declaration.payload.storage.rid = r_vle.value.vle;
      break;
    case _ZN_EVAL_DECL:
      r_vle = z_vle_decode(buf);
      ASSURE_P_RESULT(r_vle, r, Z_VLE_PARSE_ERROR)
      r->value.declaration.payload.eval.rid = r_vle.value.vle;
      break;
    case _ZN_SUBSCRIBER_DECL:
      r_vle = z_vle_decode(buf);
      ASSURE_P_RESULT(r_vle, r, Z_VLE_PARSE_ERROR)
      r_sm = _zn_sub_mode_decode(buf);
      ASSURE_P_RESULT(r_sm, r, ZN_MESSAGE_PARSE_ERROR)
      r->value.declaration.payload.sub.rid = r_vle.value.vle;
      r->value.declaration.payload.sub.sub_mode = r_sm.value.sub_mode;
      break;
    case _ZN_FORGET_PUBLISHER_DECL:
      r_vle = z_vle_decode(buf);
      ASSURE_P_RESULT(r_vle, r, Z_VLE_PARSE_ERROR)
      r->value.declaration.payload.forget_pub.rid = r_vle.value.vle;
      break;
    case _ZN_FORGET_STORAGE_DECL:
      r_vle = z_vle_decode(buf);
      ASSURE_P_RESULT(r_vle, r, Z_VLE_PARSE_ERROR)
      r->value.declaration.payload.forget_sto.rid = r_vle.value.vle;
      break;
    case _ZN_FORGET_EVAL_DECL:
      r_vle = z_vle_decode(buf);
      ASSURE_P_RESULT(r_vle, r, Z_VLE_PARSE_ERROR)
      r->value.declaration.payload.forget_eval.rid = r_vle.value.vle;
      break;
    case _ZN_FORGET_SUBSCRIBER_DECL:
      r_vle = z_vle_decode(buf);
      ASSURE_P_RESULT(r_vle, r, Z_VLE_PARSE_ERROR)
      r->value.declaration.payload.forget_sub.rid = r_vle.value.vle;
      break;
    case _ZN_RESULT_DECL:
      r->value.declaration.payload.result.cid = z_iobuf_read(buf);
      r->value.declaration.payload.result.status = z_iobuf_read(buf);
      break;
    case _ZN_COMMIT_DECL:
      r->value.declaration.payload.commit.cid = z_iobuf_read(buf);
      break;
    default:
      r->tag = Z_ERROR_TAG;
      r->value.error = ZN_MESSAGE_PARSE_ERROR;
      return;
  }
}
_zn_declaration_result_t
_zn_declaration_decode(z_iobuf_t *buf) {
  _zn_declaration_result_t r;
  _zn_declaration_decode_na(buf, &r);
  return r;
}
void
_zn_declare_encode(z_iobuf_t* buf, const _zn_declare_t* m) {
  z_vle_encode(buf, m->sn);
  unsigned int len = m->declarations.length;
  z_vle_encode(buf, len);
  for (unsigned int i = 0; i < len; ++i) {
    _zn_declaration_encode(buf, &m->declarations.elem[i]);
  }
}

void
_zn_declare_decode_na(z_iobuf_t* buf, _zn_declare_result_t *r) {
  _zn_declaration_result_t *r_decl;
  z_vle_result_t r_sn = z_vle_decode(buf);
  r->tag = Z_OK_TAG;
  ASSURE_P_RESULT(r_sn, r, Z_VLE_PARSE_ERROR)

  z_vle_result_t r_dlen = z_vle_decode(buf);
  ASSURE_P_RESULT(r_dlen, r, Z_VLE_PARSE_ERROR)
  size_t len = r_dlen.value.vle;
  r->value.declare.declarations.length = len;
  r->value.declare.declarations.elem = (_zn_declaration_t*)malloc(sizeof(_zn_declaration_t)*len);

  r_decl = (_zn_declaration_result_t*)malloc(sizeof(_zn_declaration_result_t));
  for (size_t i = 0; i < len; ++i) {
    _zn_declaration_decode_na(buf, r_decl);
    if (r_decl->tag != Z_ERROR_TAG)
      r->value.declare.declarations.elem[i] = r_decl->value.declaration;
    else {
      r->value.declare.declarations.length = 0;
      free(r->value.declare.declarations.elem);
      free(r_decl);
      r->tag = Z_ERROR_TAG;
      r->value.error = ZN_MESSAGE_PARSE_ERROR;
      return;
    }
  }
  free(r_decl);
}

_zn_declare_result_t
_zn_declare_decode(z_iobuf_t* buf) {
  _zn_declare_result_t r;
  _zn_declare_decode_na(buf, &r);
  return r;
}

void
_zn_compact_data_encode(z_iobuf_t* buf, const _zn_compact_data_t* m) {
  z_vle_encode(buf, m->sn);
  z_vle_encode(buf, m->rid);
  _zn_payload_encode(buf, &m->payload);
}

void
_zn_compact_data_decode_na(z_iobuf_t* buf, _zn_compact_data_result_t *r) {
  r->tag = Z_OK_TAG;
  z_vle_result_t r_vle = z_vle_decode(buf);
  ASSURE_P_RESULT(r_vle, r, Z_VLE_PARSE_ERROR)

  r->value.compact_data.sn = r_vle.value.vle;
  r_vle = z_vle_decode(buf);
  ASSURE_P_RESULT(r_vle, r, Z_VLE_PARSE_ERROR)
  r->value.compact_data.rid = r_vle.value.vle;
  r->value.compact_data.payload = _zn_payload_decode(buf);
}

_zn_compact_data_result_t
_zn_compact_data_decode(z_iobuf_t* buf) {
  _zn_compact_data_result_t r;
  _zn_compact_data_decode_na(buf, &r);
  return r;
}

void _zn_payload_header_encode(z_iobuf_t *buf, const _zn_payload_header_t *ph) {
  uint8_t flags = ph->flags;
  _Z_DEBUG_VA("z_payload_header_encode flags = 0x%x\n", flags);
  z_iobuf_write(buf, flags);
  if (flags & _ZN_SRC_ID) {
    _Z_DEBUG("Encoding _Z_SRC_ID\n");
    z_iobuf_write_slice(buf, (uint8_t*)ph->src_id, 0, 16);
  }
  if (flags & _ZN_SRC_SN) {
    _Z_DEBUG("Encoding _Z_SRC_SN\n");
    z_vle_encode(buf, ph->src_sn);
  }
  if (flags & _ZN_BRK_ID) {
    _Z_DEBUG("Encoding _Z_BRK_ID\n");
    z_iobuf_write_slice(buf, (uint8_t*)ph->brk_id, 0, 16);
  }
  if (flags & _ZN_BRK_SN) {
    _Z_DEBUG("Encoding _Z_BRK_SN\n");
    z_vle_encode(buf, ph->brk_sn);
  }
  if (flags & _ZN_KIND) {
    _Z_DEBUG("Encoding _Z_KIND\n");
    z_vle_encode(buf, ph->kind);
  }
  if (flags & _ZN_ENCODING) {
    _Z_DEBUG("Encoding _Z_ENCODING\n");
    z_vle_encode(buf, ph->encoding);
  }

  _zn_payload_encode(buf, &ph->payload);
}

void
_zn_payload_header_decode_na(z_iobuf_t *buf, _zn_payload_header_result_t *r) {
  z_vle_result_t r_vle;
  uint8_t flags = z_iobuf_read(buf);
  _Z_DEBUG_VA("Payload header flags: 0x%x\n", flags);

  if (flags & _ZN_SRC_ID) {
    _Z_DEBUG("Decoding _Z_SRC_ID\n");
    z_iobuf_read_to_n(buf, r->value.payload_header.src_id, 16);
  }

  if (flags & _ZN_T_STAMP) {
    _Z_DEBUG("Decoding _Z_T_STAMP\n");
    r_vle = z_vle_decode(buf);
    ASSURE_P_RESULT(r_vle, r, Z_VLE_PARSE_ERROR)
    r->value.payload_header.tstamp.time = r_vle.value.vle;
    memcpy(r->value.payload_header.tstamp.clock_id, buf->buf + buf->r_pos, 16);
    buf->r_pos += 16;
  }

  if (flags & _ZN_SRC_SN) {
    _Z_DEBUG("Decoding _Z_SRC_SN\n");
    r_vle = z_vle_decode(buf);
    ASSURE_P_RESULT(r_vle, r, Z_VLE_PARSE_ERROR)
    r->value.payload_header.src_sn = r_vle.value.vle;
  }

  if (flags & _ZN_BRK_ID) {
    _Z_DEBUG("Decoding _Z_BRK_ID\n");
    z_iobuf_read_to_n(buf, r->value.payload_header.brk_id, 16);
  }

  if (flags & _ZN_BRK_SN) {
    _Z_DEBUG("Decoding _Z_BRK_SN\n");
    r_vle = z_vle_decode(buf);
    ASSURE_P_RESULT(r_vle, r, Z_VLE_PARSE_ERROR)
    r->value.payload_header.brk_sn = r_vle.value.vle;
  }

  if (flags & _ZN_KIND) {
    _Z_DEBUG("Decoding _Z_KIND\n");
    r_vle = z_vle_decode(buf);
    ASSURE_P_RESULT(r_vle, r, Z_VLE_PARSE_ERROR)
    r->value.payload_header.kind = r_vle.value.vle;
  }

  if (flags & _ZN_ENCODING) {
    _Z_DEBUG("Decoding _Z_ENCODING\n");
    r_vle = z_vle_decode(buf);
    ASSURE_P_RESULT(r_vle, r, Z_VLE_PARSE_ERROR)
    r->value.payload_header.encoding = r_vle.value.vle;
    _Z_DEBUG("Done Decoding _Z_ENCODING\n");
  }

  _Z_DEBUG("Decoding payload\n");
  r->value.payload_header.flags = flags;
  r->value.payload_header.payload = _zn_payload_decode(buf);
  r->tag = Z_OK_TAG;
}

_zn_payload_header_result_t
_zn_payload_header_decode(z_iobuf_t *buf) {
  _zn_payload_header_result_t r;
  _zn_payload_header_decode_na(buf, &r);
  return r;
}

void
_zn_stream_data_encode(z_iobuf_t *buf, const _zn_stream_data_t* m) {
  z_vle_encode(buf, m->sn);
  z_vle_encode(buf, m->rid);
  z_vle_t len = z_iobuf_readable(&m->payload_header);
  z_vle_encode(buf, len);
  z_iobuf_write_slice(buf, m->payload_header.buf, m->payload_header.r_pos, m->payload_header.w_pos);
}

void _zn_stream_data_decode_na(z_iobuf_t *buf, _zn_stream_data_result_t *r) {
  r->tag = Z_OK_TAG;
  z_vle_result_t r_vle = z_vle_decode(buf);
  ASSURE_P_RESULT(r_vle, r, Z_VLE_PARSE_ERROR)
  r->value.stream_data.sn = r_vle.value.vle;

  r_vle = z_vle_decode(buf);
  ASSURE_P_RESULT(r_vle, r, Z_VLE_PARSE_ERROR)
  r->value.stream_data.rid = r_vle.value.vle;

  r_vle = z_vle_decode(buf);
  ASSURE_P_RESULT(r_vle, r, Z_VLE_PARSE_ERROR)
  uint8_t *ph = z_iobuf_read_n(buf, r_vle.value.vle);
  r->value.stream_data.payload_header = z_iobuf_wrap_wo(ph, r_vle.value.vle, 0, r_vle.value.vle);
  r->value.stream_data.payload_header.w_pos = r_vle.value.vle;
}

_zn_stream_data_result_t
_zn_stream_data_decode(z_iobuf_t *buf) {
  _zn_stream_data_result_t r;
  _zn_stream_data_decode_na(buf, &r);
  return r;
}

void
_zn_write_data_encode(z_iobuf_t *buf, const _zn_write_data_t* m) {
  z_vle_encode(buf, m->sn);
  z_string_encode(buf, m->rname);
  z_vle_t len = z_iobuf_readable(&m->payload_header);
  z_vle_encode(buf, len);
  z_iobuf_write_slice(buf, m->payload_header.buf, m->payload_header.r_pos, m->payload_header.w_pos);
}


void _zn_write_data_decode_na(z_iobuf_t *buf, _zn_write_data_result_t *r) {
  r->tag = Z_OK_TAG;
  z_string_result_t r_str;
  z_vle_result_t r_vle = z_vle_decode(buf);
  ASSURE_P_RESULT(r_vle, r, Z_VLE_PARSE_ERROR)
  r->value.write_data.sn = r_vle.value.vle;

  r_str = z_string_decode(buf);
  ASSURE_P_RESULT(r_str, r, Z_STRING_PARSE_ERROR)
  r->value.write_data.rname = r_str.value.string;
  _Z_DEBUG_VA("Decoding write data for resource %s\n", r_str.value.string);
  r_vle = z_vle_decode(buf);
  ASSURE_P_RESULT(r_vle, r, Z_VLE_PARSE_ERROR)
  uint8_t *ph = z_iobuf_read_n(buf, r_vle.value.vle);
  r->value.write_data.payload_header = z_iobuf_wrap_wo(ph, r_vle.value.vle, 0, r_vle.value.vle);
  r->value.write_data.payload_header.w_pos = r_vle.value.vle;
}

_zn_write_data_result_t
_zn_write_data_decode(z_iobuf_t *buf) {
  _zn_write_data_result_t r;
  _zn_write_data_decode_na(buf, &r);
  return r;
}

void
_zn_pull_encode(z_iobuf_t *buf, const _zn_pull_t* m) {
  z_vle_encode(buf, m->sn);
  z_vle_encode(buf, m->id);
}

void
_zn_query_encode(z_iobuf_t *buf, const _zn_query_t* m) {
  z_uint8_array_encode(buf, &(m->pid));
  z_vle_encode(buf, m->qid);
  z_string_encode(buf, m->rname);
  z_string_encode(buf, m->predicate);
}

void _zn_query_decode_na(z_iobuf_t *buf, _zn_query_result_t *r) {
  r->tag = Z_OK_TAG;

  z_uint8_array_result_t r_pid = z_uint8_array_decode(buf);
  ASSURE_P_RESULT(r_pid, r, Z_ARRAY_PARSE_ERROR)
  r->value.query.pid = r_pid.value.uint8_array;

  z_vle_result_t r_qid = z_vle_decode(buf);
  ASSURE_P_RESULT(r_qid, r, Z_VLE_PARSE_ERROR)
  r->value.query.qid = r_qid.value.vle;

  z_string_result_t r_str = z_string_decode(buf);
  ASSURE_P_RESULT(r_str, r, Z_STRING_PARSE_ERROR)
  r->value.query.rname = r_str.value.string;

  r_str = z_string_decode(buf);
  ASSURE_P_RESULT(r_str, r, Z_STRING_PARSE_ERROR)
  r->value.query.predicate = r_str.value.string;
}

_zn_query_result_t
_zn_query_decode(z_iobuf_t *buf) {
  _zn_query_result_t r;
  _zn_query_decode_na(buf, &r);
  return r;
}

void
_zn_reply_encode(z_iobuf_t *buf, const _zn_reply_t* m, uint8_t header) {
  z_uint8_array_encode(buf, &(m->qpid));
  z_vle_encode(buf, m->qid);

  if(header & _ZN_F_FLAG) {
    z_uint8_array_encode(buf, &(m->srcid));
    z_vle_encode(buf, m->rsn);
    z_string_encode(buf, m->rname);
    z_vle_t len = z_iobuf_readable(&m->payload_header);
    z_vle_encode(buf, len);
    z_iobuf_write_slice(buf, m->payload_header.buf, m->payload_header.r_pos, m->payload_header.w_pos);
  }
}

void _zn_reply_decode_na(z_iobuf_t *buf, uint8_t header, _zn_reply_result_t *r) {
  r->tag = Z_OK_TAG;

  z_uint8_array_result_t r_qpid = z_uint8_array_decode(buf);
  ASSURE_P_RESULT(r_qpid, r, Z_ARRAY_PARSE_ERROR)
  r->value.reply.qpid = r_qpid.value.uint8_array;

  z_vle_result_t r_qid = z_vle_decode(buf);
  ASSURE_P_RESULT(r_qid, r, Z_VLE_PARSE_ERROR)
  r->value.reply.qid = r_qid.value.vle;

  if (header & _ZN_F_FLAG)
  {
    z_uint8_array_result_t r_srcid = z_uint8_array_decode(buf);
    ASSURE_P_RESULT(r_srcid, r, Z_ARRAY_PARSE_ERROR)
    r->value.reply.srcid = r_srcid.value.uint8_array;

    z_vle_result_t r_vle = z_vle_decode(buf);
    ASSURE_P_RESULT(r_vle, r, Z_VLE_PARSE_ERROR)
    r->value.reply.rsn = r_vle.value.vle;

    z_string_result_t r_str = z_string_decode(buf);
    ASSURE_P_RESULT(r_str, r, Z_STRING_PARSE_ERROR)
    r->value.reply.rname = r_str.value.string;

    r_vle = z_vle_decode(buf);
    ASSURE_P_RESULT(r_vle, r, Z_VLE_PARSE_ERROR)
    uint8_t *ph = z_iobuf_read_n(buf, r_vle.value.vle);
    r->value.reply.payload_header = z_iobuf_wrap_wo(ph, r_vle.value.vle, 0, r_vle.value.vle);
    r->value.reply.payload_header.w_pos = r_vle.value.vle;
  }
}

_zn_reply_result_t
_zn_reply_decode(z_iobuf_t *buf, uint8_t header) {
  _zn_reply_result_t r;
  _zn_reply_decode_na(buf, header, &r);
  return r;
}

void
_zn_message_encode(z_iobuf_t* buf, const _zn_message_t* m) {
  z_iobuf_write(buf, m->header);
  uint8_t mid = _ZN_MID(m->header);
  switch (mid) {
    case _ZN_COMPACT_DATA:
      _zn_compact_data_encode(buf, &m->payload.compact_data);
      break;
    case _ZN_STREAM_DATA:
      _zn_stream_data_encode(buf, &m->payload.stream_data);
      break;
    case _ZN_WRITE_DATA:
      _zn_write_data_encode(buf, &m->payload.write_data);
      break;
    case _ZN_PULL:
      _zn_pull_encode(buf, &m->payload.pull);
      break;
    case _ZN_QUERY:
      _zn_query_encode(buf, &m->payload.query);
      if (m->header & _ZN_P_FLAG ) {
        zn_properties_encode(buf, m->properties);
      }
      break;
    case _ZN_REPLY:
      _zn_reply_encode(buf, &m->payload.reply, m->header);
      break;
    case _ZN_OPEN:
      _zn_open_encode(buf, &m->payload.open);
      if (m->header & _ZN_P_FLAG ) {
        zn_properties_encode(buf, m->properties);
      }
      break;
    case _ZN_CLOSE:
      _zn_close_encode(buf, &m->payload.close);
      break;
    case _ZN_DECLARE:
      _zn_declare_encode(buf, &m->payload.declare);
      break;
    default:
      _Z_ERROR("WARNING: Trying to encode message with unknown ID(%d)\n", mid);
      return;
  }
}

void
_zn_message_decode_na(z_iobuf_t* buf, _zn_message_p_result_t* r) {
  _zn_compact_data_result_t r_cd;
  _zn_stream_data_result_t r_sd;
  _zn_write_data_result_t r_wd;
  _zn_query_result_t r_q;
  _zn_reply_result_t r_r;
  _zn_accept_result_t r_a;
  _zn_close_result_t r_c;
  _zn_declare_result_t r_d;
  _zn_hello_result_t r_h;

  uint8_t h = z_iobuf_read(buf);
  r->tag = Z_OK_TAG;
  r->value.message->header = h;

  uint8_t mid = _ZN_MID(h);
  switch (mid) {
    case _ZN_COMPACT_DATA:
      r->tag = Z_OK_TAG;
      r_cd = _zn_compact_data_decode(buf);
      ASSURE_P_RESULT(r_cd, r, ZN_MESSAGE_PARSE_ERROR)
      r->value.message->payload.compact_data = r_cd.value.compact_data;
      break;
    case _ZN_STREAM_DATA:
      r->tag = Z_OK_TAG;
      r_sd = _zn_stream_data_decode(buf);
      ASSURE_P_RESULT(r_sd, r, ZN_MESSAGE_PARSE_ERROR)
      r->value.message->payload.stream_data = r_sd.value.stream_data;
      break;
    case _ZN_WRITE_DATA:
      r->tag = Z_OK_TAG;
      r_wd = _zn_write_data_decode(buf);
      ASSURE_P_RESULT(r_wd, r, ZN_MESSAGE_PARSE_ERROR)
      r->value.message->payload.write_data = r_wd.value.write_data;
      break;
    case _ZN_QUERY:
      r->tag = Z_OK_TAG;
      r_q = _zn_query_decode(buf);
      ASSURE_P_RESULT(r_q, r, ZN_MESSAGE_PARSE_ERROR)
      r->value.message->payload.query = r_q.value.query;
      break;
    case _ZN_REPLY:
      r->tag = Z_OK_TAG;
      r_r = _zn_reply_decode(buf, h);
      ASSURE_P_RESULT(r_r, r, ZN_MESSAGE_PARSE_ERROR)
      r->value.message->payload.reply = r_r.value.reply;
      break;
    case _ZN_ACCEPT:
      r->tag = Z_OK_TAG;
      r_a = _zn_accept_decode(buf);
      ASSURE_P_RESULT(r_a, r, ZN_MESSAGE_PARSE_ERROR)
      r->value.message->payload.accept = r_a.value.accept;
      break;
    case _ZN_CLOSE:
      r->tag = Z_OK_TAG;
      r_c = _zn_close_decode(buf);
      ASSURE_P_RESULT(r_c, r, ZN_MESSAGE_PARSE_ERROR)
      r->value.message->payload.close = r_c.value.close;
      break;
    case _ZN_DECLARE:
      r->tag = Z_OK_TAG;
      r_d = _zn_declare_decode(buf);
      ASSURE_P_RESULT(r_d, r, ZN_MESSAGE_PARSE_ERROR)
      r->value.message->payload.declare = r_d.value.declare;
      break;
    case _ZN_HELLO:
      r->tag = Z_OK_TAG;
      r_h = z_hello_decode(buf);
      ASSURE_P_RESULT(r_h, r, ZN_MESSAGE_PARSE_ERROR)
      r->value.message->payload.hello = r_h.value.hello;
      break;
    default:
      r->tag = Z_ERROR_TAG;
      r->value.error = ZN_MESSAGE_PARSE_ERROR;
      _Z_ERROR("WARNING: Trying to decode message with unknown ID(%d)\n", mid);

  }
}

_zn_message_p_result_t
_zn_message_decode(z_iobuf_t* buf) {
  _zn_message_p_result_t r;
  _zn_message_p_result_init(&r);
  _zn_message_decode_na(buf, &r);
  return r;
}
