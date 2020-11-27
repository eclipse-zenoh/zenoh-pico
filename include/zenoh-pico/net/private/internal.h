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

#ifndef _ZENOH_NET_PICO_INTERNAL_H
#define _ZENOH_NET_PICO_INTERNAL_H

#include "zenoh-pico/net/types.h"
#include "zenoh-pico/net/private/msg.h"
#include "zenoh-pico/net/private/msgcodec.h"

/*------------------ Session ------------------*/
zn_session_t *_zn_session_init(void);
int _zn_session_close(zn_session_t *zn, uint8_t reason);
void _zn_session_free(zn_session_t *zn);

int _zn_handle_session_message(zn_session_t *zn, _zn_session_message_t *s_msg);
int _zn_handle_zenoh_message(zn_session_t *zn, _zn_zenoh_message_t *z_msg);

/*------------------ Clone/Copy/Free helpers ------------------*/
zn_reskey_t _zn_reskey_clone(const zn_reskey_t *resky);

/*------------------ Message helper ------------------*/
_zn_session_message_t _zn_session_message_init(uint8_t header);
_zn_zenoh_message_t _zn_zenoh_message_init(uint8_t header);
_zn_reply_context_t *_zn_reply_context_init(void);
_zn_attachment_t *_zn_attachment_init(void);

/*------------------ SN helpers ------------------*/
int _zn_sn_precedes(z_zint_t sn_resolution_half, z_zint_t sn_left, z_zint_t sn_right);

/*------------------ Transmission and Reception helpers ------------------*/
int _zn_send_s_msg(zn_session_t *zn, _zn_session_message_t *m);
int _zn_send_z_msg(zn_session_t *zn, _zn_zenoh_message_t *m, zn_reliability_t reliability, zn_congestion_control_t cong_ctrl);

_zn_session_message_p_result_t _zn_recv_s_msg(zn_session_t *zn);
void _zn_recv_s_msg_na(zn_session_t *zn, _zn_session_message_p_result_t *r);

/*------------------ Entity ------------------*/
z_zint_t _zn_get_entity_id(zn_session_t *zn);

/*------------------ Resource ------------------*/
z_zint_t _zn_get_resource_id(zn_session_t *zn);
_zn_resource_t *_zn_get_resource_by_id(zn_session_t *zn, int is_local, z_zint_t rid);
_zn_resource_t *_zn_get_resource_by_key(zn_session_t *zn, int is_local, const zn_reskey_t *reskey);
z_str_t _zn_get_resource_name_from_key(zn_session_t *zn, int is_local, const zn_reskey_t *reskey);
int _zn_register_resource(zn_session_t *zn, int is_local, _zn_resource_t *res);
void _zn_unregister_resource(zn_session_t *zn, int is_local, _zn_resource_t *res);
void _zn_flush_resources(zn_session_t *zn);

/*------------------ Subscription ------------------*/
_z_list_t *_zn_get_subscriptions_from_remote_key(zn_session_t *zn, const zn_reskey_t *reskey);
_zn_subscriber_t *_zn_get_subscription_by_id(zn_session_t *zn, int is_local, z_zint_t id);
_zn_subscriber_t *_zn_get_subscription_by_key(zn_session_t *zn, int is_local, const zn_reskey_t *reskey);
int _zn_register_subscription(zn_session_t *zn, int is_local, _zn_subscriber_t *sub);
void _zn_unregister_subscription(zn_session_t *zn, int is_local, _zn_subscriber_t *sub);
void _zn_flush_subscriptions(zn_session_t *zn);
void _zn_trigger_subscriptions(zn_session_t *zn, const zn_reskey_t reskey, const z_bytes_t payload);

/*------------------ Pull ------------------*/
z_zint_t _zn_get_pull_id(zn_session_t *zn);

/*------------------ Query ------------------*/
z_zint_t _zn_get_query_id(zn_session_t *zn);
int _zn_register_pending_query(zn_session_t *zn, _zn_pending_query_t *pq);
void _zn_unregister_pending_query(zn_session_t *zn, _zn_pending_query_t *pq);
void _zn_flush_pending_queries(zn_session_t *zn);
void _zn_trigger_query_reply_partial(zn_session_t *zn, const _zn_reply_context_t *reply_context, const zn_reskey_t reskey, const z_bytes_t payload, const _zn_data_info_t data_info);
void _zn_trigger_query_reply_final(zn_session_t *zn, const _zn_reply_context_t *reply_context);

/*------------------ Queryable ------------------*/
_zn_queryable_t *_zn_get_queryable_by_id(zn_session_t *zn, z_zint_t id);
int _zn_register_queryable(zn_session_t *zn, _zn_queryable_t *q);
void _zn_unregister_queryable(zn_session_t *zn, _zn_queryable_t *q);
void _zn_flush_queryables(zn_session_t *zn);
void _zn_trigger_queryables(zn_session_t *zn, const _zn_query_t *query);

#endif /* _ZENOH_NET_PICO_INTERNAL_H */
