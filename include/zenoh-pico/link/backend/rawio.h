#ifndef ZENOH_PICO_LINK_BACKEND_RAWIO_H
#define ZENOH_PICO_LINK_BACKEND_RAWIO_H

#include <stddef.h>
#include <stdint.h>

#include "zenoh-pico/system/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    z_result_t (*open_from_pins)(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin, uint32_t baudrate);
    z_result_t (*open_from_dev)(_z_sys_net_socket_t *sock, const char *dev, uint32_t baudrate);
    z_result_t (*listen_from_pins)(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin, uint32_t baudrate);
    z_result_t (*listen_from_dev)(_z_sys_net_socket_t *sock, const char *dev, uint32_t baudrate);
    void (*close)(_z_sys_net_socket_t *sock);
    // flawfinder: ignore
    size_t (*read)(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len);
    size_t (*write)(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len);
} _z_rawio_ops_t;

static inline z_result_t _z_rawio_open_from_pins(const _z_rawio_ops_t *ops, _z_sys_net_socket_t *sock, uint32_t txpin,
                                                 uint32_t rxpin, uint32_t baudrate) {
    return ops->open_from_pins(sock, txpin, rxpin, baudrate);
}

static inline z_result_t _z_rawio_open_from_dev(const _z_rawio_ops_t *ops, _z_sys_net_socket_t *sock, const char *dev,
                                                uint32_t baudrate) {
    return ops->open_from_dev(sock, dev, baudrate);
}

static inline z_result_t _z_rawio_listen_from_pins(const _z_rawio_ops_t *ops, _z_sys_net_socket_t *sock, uint32_t txpin,
                                                   uint32_t rxpin, uint32_t baudrate) {
    return ops->listen_from_pins(sock, txpin, rxpin, baudrate);
}

static inline z_result_t _z_rawio_listen_from_dev(const _z_rawio_ops_t *ops, _z_sys_net_socket_t *sock, const char *dev,
                                                  uint32_t baudrate) {
    return ops->listen_from_dev(sock, dev, baudrate);
}

static inline void _z_rawio_close(const _z_rawio_ops_t *ops, _z_sys_net_socket_t *sock) { ops->close(sock); }

static inline size_t _z_rawio_read(const _z_rawio_ops_t *ops, _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    // flawfinder: ignore
    return ops->read(sock, ptr, len);
}

static inline size_t _z_rawio_write(const _z_rawio_ops_t *ops, _z_sys_net_socket_t sock, const uint8_t *ptr,
                                    size_t len) {
    return ops->write(sock, ptr, len);
}

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_LINK_BACKEND_RAWIO_H */
