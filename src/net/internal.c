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

#include <assert.h>
#include "zenoh/rname.h"
#include "zenoh/net/private/internal.h"
#include "zenoh/private/logging.h"

z_vle_t _zn_get_entity_id(zn_session_t *z) {
  return z->eid++;
}

z_vle_t _zn_get_resource_id(zn_session_t *z, const char *rname) {
  _zn_res_decl_t *rd_brn = _zn_get_res_decl_by_rname(z, rname);
  if (rd_brn == 0) {
    z_vle_t rid = z->rid++;
    while (_zn_get_res_decl_by_rid(z, rid) != 0) {
      rid++;
    }
    z->rid = rid;
    return rid;
  }
  else return rd_brn->rid;
}

int _zn_register_res_decl(zn_session_t *z, z_vle_t rid, const char *rname) {
  _Z_DEBUG_VA(">>> Allocating res decl for (%zu,%s)\n", rid, rname);
  _zn_res_decl_t *rd_bid = _zn_get_res_decl_by_rid(z, rid);
  _zn_res_decl_t *rd_brn = _zn_get_res_decl_by_rname(z, rname);

  if (rd_bid == 0 && rd_brn == 0) {
    _zn_res_decl_t *rdecl = (_zn_res_decl_t *) malloc(sizeof(_zn_res_decl_t));
    rdecl->rid = rid;
    rdecl->r_name = strdup(rname);
    z->declarations = z_list_cons(z->declarations, rdecl);
    return 0;
  }
  else if (rd_bid == rd_brn)
    return 0;
  else return 1;
}

_zn_res_decl_t *_zn_get_res_decl_by_rid(zn_session_t *z, z_vle_t rid) {
  if (z->declarations == 0) {
    return 0;
  }
  else {
    _zn_res_decl_t *decl = (_zn_res_decl_t *)z_list_head(z->declarations);
    z_list_t *decls = z_list_tail(z->declarations);
    while (decls != 0 && decl->rid != rid) {
      decl = z_list_head(decls);
      decls = z_list_tail(decls);
    }
    if (decl->rid == rid) return decl;
    else return 0;
  }
}

_zn_res_decl_t *_zn_get_res_decl_by_rname(zn_session_t *z, const char *rname) {
  if (z->declarations == 0) {
    return 0;
  } else {
    _zn_res_decl_t *decl = (_zn_res_decl_t *)z_list_head(z->declarations);
    z_list_t *decls = z_list_tail(z->declarations);

    while (decls != 0 && strcmp(decl->r_name, rname) != 0) {
      decl = z_list_head(decls);
      decls = z_list_tail(decls);
    }
    if (strcmp(decl->r_name, rname) == 0) return decl;
    else return 0;
  }
}

void _zn_register_subscription(zn_session_t *z, z_vle_t rid, z_vle_t id, zn_data_handler_t data_handler, void *arg) {
  _zn_sub_t *sub = (_zn_sub_t *) malloc(sizeof(_zn_sub_t));
  sub->rid = rid;
  sub->id = id;
  _zn_res_decl_t *decl = _zn_get_res_decl_by_rid(z, rid);
  assert(decl != 0);
  sub->rname = strdup(decl->r_name);
  sub->data_handler = data_handler;
  sub->arg = arg;
  z->subscriptions = z_list_cons(z->subscriptions, sub);
}

int sub_predicate(void *elem, void *arg) {
  zn_sub_t *s = (zn_sub_t *)arg;
  _zn_sub_t *sub = (_zn_sub_t *)elem;
  if(sub->id == s->id) {
    return 1;
  } else {
    return 0;
  }
}

void _zn_unregister_subscription(zn_sub_t *s) {
  s->z->subscriptions = z_list_remove(s->z->subscriptions, sub_predicate, s);
}

const char *_zn_get_resource_name(zn_session_t *z, z_vle_t rid) {
  z_list_t *ds = z->declarations;
  _zn_res_decl_t *d;
  while (ds != z_list_empty) {
    d = z_list_head(ds);
    if (d->rid == rid) {
      return d->r_name;
    }
    ds = z_list_tail(ds);
  }
  return 0;
}

z_list_t *
_zn_get_subscriptions_by_rid(zn_session_t *z, z_vle_t rid) {
  z_list_t *subs = z_list_empty;
  if (z->subscriptions == 0) {
    return subs;
  }
  else {
    _zn_sub_t *sub = 0;
    z_list_t *subs = z->subscriptions;
    z_list_t *xs = z_list_empty;
    do {
      sub = (_zn_sub_t *)z_list_head(subs);
      subs = z_list_tail(subs);
      if (sub->rid == rid) {
        xs = z_list_cons(xs, sub);
      }
    } while (subs != 0);
    return xs;
  }
}

z_list_t *
_zn_get_subscriptions_by_rname(zn_session_t *z, const char *rname) {
  z_list_t *subs = z_list_empty;
  if (z->subscriptions == 0) {
    return subs;
  }
  else {
    _zn_sub_t *sub = 0;
    z_list_t *subs = z->subscriptions;
    z_list_t *xs = z_list_empty;
    do {
      sub = (_zn_sub_t *)z_list_head(subs);
      subs = z_list_tail(subs);
      if (zn_rname_intersect(sub->rname, (char *)rname)) {
        xs = z_list_cons(xs, sub);
      }
    } while (subs != 0);
    return xs;
  }
}

void _zn_register_storage(zn_session_t *z, z_vle_t rid, z_vle_t id, zn_data_handler_t data_handler, zn_query_handler_t query_handler, void *arg) {
  _zn_sto_t *sto = (_zn_sto_t *) malloc(sizeof(_zn_sto_t));
  sto->rid = rid;
  sto->id = id;
  _zn_res_decl_t *decl = _zn_get_res_decl_by_rid(z, rid);
  assert(decl != 0);
  sto->rname = strdup(decl->r_name);
  sto->data_handler = data_handler;
  sto->query_handler = query_handler;
  sto->arg = arg;
  z->storages = z_list_cons(z->storages, sto);
}

int sto_predicate(void *elem, void *arg) {
  zn_sto_t *s = (zn_sto_t *)arg;
  _zn_sto_t *sto = (_zn_sto_t *)elem;
  if(sto->id == s->id) {
    return 1;
  } else {
    return 0;
  }
}

void _zn_unregister_storage(zn_sto_t *s) {
  s->z->storages = z_list_remove(s->z->storages, sto_predicate, s);
}

z_list_t *
_zn_get_storages_by_rid(zn_session_t *z, z_vle_t rid) {
  z_list_t *stos = z_list_empty;
  if (z->storages == 0) {
    return stos;
  }
  else {
    _zn_sto_t *sto = 0;
    z_list_t *stos = z->storages;
    z_list_t *xs = z_list_empty;
    do {
      sto = (_zn_sto_t *)z_list_head(stos);
      stos = z_list_tail(stos);
      if (sto->rid == rid) {
        xs = z_list_cons(xs, sto);
      }
    } while (stos != 0);
    return xs;
  }
}

z_list_t *
_zn_get_storages_by_rname(zn_session_t *z, const char *rname) {
  z_list_t *stos = z_list_empty;
  if (z->storages == 0) {
    return stos;
  }
  else {
    _zn_sto_t *sto = 0;
    z_list_t *stos = z->storages;
    z_list_t *xs = z_list_empty;
    do {
      sto = (_zn_sto_t *)z_list_head(stos);
      stos = z_list_tail(stos);
      if (zn_rname_intersect(sto->rname, (char *)rname)) {
        xs = z_list_cons(xs, sto);
      }
    } while (stos != 0);
    return xs;
  }
}

void _zn_register_eval(zn_session_t *z, z_vle_t rid, z_vle_t id, zn_query_handler_t query_handler, void *arg) {
  _zn_eva_t *eval = (_zn_eva_t *) malloc(sizeof(_zn_eva_t));
  eval->rid = rid;
  eval->id = id;
  _zn_res_decl_t *decl = _zn_get_res_decl_by_rid(z, rid);
  assert(decl != 0);
  eval->rname = strdup(decl->r_name);
  eval->query_handler = query_handler;
  eval->arg = arg;
  z->evals = z_list_cons(z->evals, eval);
}

int eva_predicate(void *elem, void *arg) {
  zn_eva_t *e = (zn_eva_t *)arg;
  _zn_eva_t *eval = (_zn_eva_t *)elem;
  if(eval->id == e->id) {
    return 1;
  } else {
    return 0;
  }
}

void _zn_unregister_eval(zn_eva_t *e) {
  e->z->evals = z_list_remove(e->z->evals, sto_predicate, e);
}

z_list_t *
_zn_get_evals_by_rid(zn_session_t *z, z_vle_t rid) {
  z_list_t *evals = z_list_empty;
  if (z->evals == 0) {
    return evals;
  }
  else {
    _zn_eva_t *eval = 0;
    z_list_t *evals = z->evals;
    z_list_t *xs = z_list_empty;
    do {
      eval = (_zn_eva_t *)z_list_head(evals);
      evals = z_list_tail(evals);
      if (eval->rid == rid) {
        xs = z_list_cons(xs, eval);
      }
    } while (evals != 0);
    return xs;
  }
}

z_list_t *
_zn_get_evals_by_rname(zn_session_t *z, const char *rname) {
  z_list_t *evals = z_list_empty;
  if (z->evals == 0) {
    return evals;
  }
  else {
    _zn_eva_t *eval = 0;
    z_list_t *evals = z->evals;
    z_list_t *xs = z_list_empty;
    do {
      eval = (_zn_eva_t *)z_list_head(evals);
      evals = z_list_tail(evals);
      if (zn_rname_intersect(eval->rname, (char *)rname)) {
        xs = z_list_cons(xs, eval);
      }
    } while (evals != 0);
    return xs;
  }
}

int _zn_matching_remote_sub(zn_session_t *z, z_vle_t rid) {
  return z_i_map_get(z->remote_subs, rid) != 0 ? 1 : 0;
}

void _zn_register_query(zn_session_t *z, z_vle_t qid, zn_reply_handler_t reply_handler, void *arg) {
  _zn_replywaiter_t *rw = (_zn_replywaiter_t *) malloc(sizeof(_zn_replywaiter_t));
  rw->qid = qid;
  rw->reply_handler = reply_handler;
  rw->arg = arg;
  z->replywaiters = z_list_cons(z->replywaiters, rw);
}

_zn_replywaiter_t *_zn_get_query(zn_session_t *z, z_vle_t qid) {
  if (z->replywaiters == 0) {
    return 0;
  }
  else {
    _zn_replywaiter_t *rw = (_zn_replywaiter_t *)z_list_head(z->replywaiters);
    z_list_t *rws = z_list_tail(z->replywaiters);
    while (rws != 0 && rw->qid != qid) {
      rw = z_list_head(rws);
      rws = z_list_tail(rws);
    }
    if (rw->qid == qid) return rw;
    else return 0;
  }
}
