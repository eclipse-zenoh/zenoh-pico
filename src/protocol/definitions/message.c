#include "zenoh-pico/protocol/definitions/message.h"

void _z_msg_reply_clear(_z_msg_reply_t *msg) { _z_value_clear(&msg->_value); }

void _z_msg_put_clear(_z_msg_put_t *msg) {
    _z_bytes_clear(&msg->_encoding.suffix);
    _z_bytes_clear(&msg->_payload);
    _z_timestamp_clear(&msg->_commons._timestamp);
}

_z_msg_query_reqexts_t _z_msg_query_required_extensions(const _z_msg_query_t *msg) {
    return (_z_msg_query_reqexts_t){.body = msg->_value.payload.start != NULL,
                                    .info = _z_id_check(msg->_info._id),
                                    .consolidation = msg->_consolidation != Z_CONSOLIDATION_MODE_AUTO};
}

void _z_msg_query_clear(_z_msg_query_t *msg) {
    _z_bytes_clear(&msg->_parameters);
    _z_value_clear(&msg->_value);
}
