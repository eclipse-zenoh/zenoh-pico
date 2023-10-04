#include "zenoh-pico/protocol/codec/core.h"

#include "zenoh-pico/protocol/iobuf.h"
int8_t _z_zbuf_read_exact(_z_zbuf_t *zbf, uint8_t *dest, size_t length) {
    if (length > _z_zbuf_len(zbf)) {
        return _Z_ERR_MESSAGE_DESERIALIZATION_FAILED;
    }
    _z_zbuf_read_bytes(zbf, dest, 0, length);
    return _Z_RES_OK;
}