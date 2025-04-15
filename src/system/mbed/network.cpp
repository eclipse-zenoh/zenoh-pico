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

#include <NetworkInterface.h>
#include <USBSerial.h>
#include <mbed.h>

extern "C" {
#include <netdb.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/protocol/codec/serial.h"
#include "zenoh-pico/system/common/serial.h"
#include "zenoh-pico/system/link/serial.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/checksum.h"
#include "zenoh-pico/utils/encoding.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

z_result_t _z_socket_set_non_blocking(const _z_sys_net_socket_t *sock) {
    _ZP_UNUSED(sock);
    _Z_ERROR("Function not yet supported on this system");
    return _Z_ERR_GENERIC;
}

z_result_t _z_socket_accept(const _z_sys_net_socket_t *sock_in, _z_sys_net_socket_t *sock_out) {
    _ZP_UNUSED(sock_in);
    _ZP_UNUSED(sock_out);
    _Z_ERROR("Function not yet supported on this system");
    return _Z_ERR_GENERIC;
}

void _z_socket_close(_z_sys_net_socket_t *sock) { _ZP_UNUSED(sock); }

z_result_t _z_socket_wait_event(void *peers, _z_mutex_rec_t *mutex) {
    _ZP_UNUSED(peers);
    _ZP_UNUSED(mutex);
    _Z_ERROR("Function not yet supported on this system");
    return _Z_ERR_GENERIC;
}

#if Z_FEATURE_LINK_TCP == 1
/*------------------ TCP sockets ------------------*/
z_result_t _z_create_endpoint_tcp(_z_sys_net_endpoint_t *ep, const char *s_address, const char *s_port) {
    z_result_t ret = _Z_RES_OK;

    // Parse and check the validity of the port
    uint32_t port = strtoul(s_port, NULL, 10);
    if ((port > (uint32_t)0) && (port <= (uint32_t)65355)) {  // Port numbers should range from 1 to 65355
        ep->_iptcp = new SocketAddress(s_address, port);
    } else {
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

void _z_free_endpoint_tcp(_z_sys_net_endpoint_t *ep) { delete ep->_iptcp; }

z_result_t _z_open_tcp(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout) {
    z_result_t ret = _Z_RES_OK;

    sock->_tcp = new TCPSocket();
    sock->_tcp->set_timeout(tout);
    if ((ret == _Z_RES_OK) && (sock->_tcp->open(NetworkInterface::get_default_instance()) < 0)) {
        ret = _Z_ERR_GENERIC;
    }

    if ((ret == _Z_RES_OK) && (sock->_tcp->connect(*rep._iptcp) < 0)) {
        ret = _Z_ERR_GENERIC;
    }

    if (ret != _Z_RES_OK) {
        delete sock->_tcp;
    }

    return ret;
}

z_result_t _z_listen_tcp(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t lep) {
    z_result_t ret = _Z_RES_OK;
    (void)sock;
    (void)lep;

    // @TODO: To be implemented
    ret = _Z_ERR_GENERIC;

    return ret;
}

void _z_close_tcp(_z_sys_net_socket_t *sock) {
    sock->_tcp->close();
    delete sock->_tcp;
}

size_t _z_read_tcp(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    nsapi_size_or_error_t rb = sock._tcp->recv(ptr, len);
    if (rb < (nsapi_size_or_error_t)0) {
        rb = SIZE_MAX;
    }

    return rb;
}

size_t _z_read_exact_tcp(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_read_tcp(sock, pos, len - n);
        if ((rb == SIZE_MAX) || (rb == 0)) {
            n = rb;
            break;
        }

        n = n + rb;
        pos = _z_ptr_u8_offset(pos, n);
    } while (n != len);

    return n;
}

size_t _z_send_tcp(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) { return sock._tcp->send(ptr, len); }
#endif

#if Z_FEATURE_LINK_UDP_UNICAST == 1 || Z_FEATURE_LINK_UDP_MULTICAST == 1
/*------------------ UDP sockets ------------------*/
z_result_t _z_create_endpoint_udp(_z_sys_net_endpoint_t *ep, const char *s_address, const char *s_port) {
    z_result_t ret = _Z_RES_OK;

    // Parse and check the validity of the port
    uint32_t port = strtoul(s_port, NULL, 10);
    if ((port > (uint32_t)0) && (port <= (uint32_t)65355)) {  // Port numbers should range from 1 to 65355
        ep->_iptcp = new SocketAddress(s_address, port);
    } else {
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

void _z_free_endpoint_udp(_z_sys_net_endpoint_t *ep) { delete ep->_iptcp; }
#endif

#if Z_FEATURE_LINK_UDP_UNICAST == 1
z_result_t _z_open_udp_unicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout) {
    z_result_t ret = _Z_RES_OK;
    (void)sock;
    (void)rep;

    sock->_udp = new UDPSocket();
    sock->_udp->set_timeout(tout);
    if ((ret == _Z_RES_OK) && (sock->_udp->open(NetworkInterface::get_default_instance()) < 0)) {
        ret = _Z_ERR_GENERIC;
    }

    if (ret != _Z_RES_OK) {
        delete sock->_tcp;
    }

    return ret;
}

z_result_t _z_listen_udp_unicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t lep, uint32_t tout) {
    z_result_t ret = _Z_RES_OK;
    (void)sock;
    (void)lep;
    (void)tout;

    // @TODO: To be implemented
    ret = _Z_ERR_GENERIC;

    return ret;
}

void _z_close_udp_unicast(_z_sys_net_socket_t *sock) {
    sock->_udp->close();
    delete sock->_udp;
}

size_t _z_read_udp_unicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    SocketAddress raddr;
    nsapi_size_or_error_t rb = sock._udp->recvfrom(&raddr, ptr, len);
    if (rb < (nsapi_size_or_error_t)0) {
        rb = SIZE_MAX;
    }

    return rb;
}

size_t _z_read_exact_udp_unicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_read_udp_unicast(sock, pos, len - n);
        if ((rb == SIZE_MAX) || (rb == 0)) {
            n = rb;
            break;
        }

        n = n + rb;
        pos = _z_ptr_u8_offset(pos, n);
    } while (n != len);

    return n;
}

size_t _z_send_udp_unicast(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len,
                           const _z_sys_net_endpoint_t rep) {
    int sb = sock._udp->sendto(*rep._iptcp, ptr, len);
    return sb;
}
#endif

#if Z_FEATURE_LINK_UDP_MULTICAST == 1
z_result_t _z_open_udp_multicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, _z_sys_net_endpoint_t *lep,
                                 uint32_t tout, const char *iface) {
    z_result_t ret = _Z_RES_OK;
    (void)(lep);  // Multicast messages are not self-consumed, so no need to save the local address

    sock->_udp = new UDPSocket();
    sock->_udp->set_timeout(tout);
    if ((ret == _Z_RES_OK) && (sock->_udp->open(NetworkInterface::get_default_instance()) < 0)) {
        ret = _Z_ERR_GENERIC;
    }

    if (ret != _Z_RES_OK) {
        delete sock->_tcp;
    }

    return ret;
}

z_result_t _z_listen_udp_multicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout,
                                   const char *iface, const char *join) {
    (void)join;
    z_result_t ret = _Z_RES_OK;

    sock->_udp = new UDPSocket();
    sock->_udp->set_timeout(tout);
    if ((ret == _Z_RES_OK) && (sock->_udp->open(NetworkInterface::get_default_instance()) < 0)) {
        ret = _Z_ERR_GENERIC;
    }

    if ((ret == _Z_RES_OK) && (sock->_udp->bind(rep._iptcp->get_port()) < 0)) {
        ret = _Z_ERR_GENERIC;
    }

    if ((ret == _Z_RES_OK) && (sock->_udp->join_multicast_group(*rep._iptcp) < 0)) {
        ret = _Z_ERR_GENERIC;
    }

    if (ret != _Z_RES_OK) {
        delete sock->_tcp;
    }

    return ret;
}

void _z_close_udp_multicast(_z_sys_net_socket_t *sockrecv, _z_sys_net_socket_t *socksend,
                            const _z_sys_net_endpoint_t rep, const _z_sys_net_endpoint_t lep) {
    _ZP_UNUSED(lep);
    sockrecv->_udp->leave_multicast_group(*rep._iptcp);
    sockrecv->_udp->close();
    delete sockrecv->_udp;

    socksend->_udp->close();
    delete socksend->_udp;
}

size_t _z_read_udp_multicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len, const _z_sys_net_endpoint_t lep,
                             _z_slice_t *addr) {
    SocketAddress raddr;
    nsapi_size_or_error_t rb = 0;

    do {
        rb = sock._udp->recvfrom(&raddr, ptr, len);
        if (rb < (nsapi_size_or_error_t)0) {
            rb = SIZE_MAX;
            break;
        }

        if (raddr.get_ip_version() == NSAPI_IPv4) {
            addr->len = NSAPI_IPv4_BYTES + sizeof(uint16_t);
            (void)memcpy(const_cast<uint8_t *>(addr->start), raddr.get_ip_bytes(), NSAPI_IPv4_BYTES);
            uint16_t port = raddr.get_port();
            (void)memcpy(const_cast<uint8_t *>(addr->start + NSAPI_IPv4_BYTES), &port, sizeof(uint16_t));
            break;
        } else if (raddr.get_ip_version() == NSAPI_IPv6) {
            addr->len = NSAPI_IPv6_BYTES + sizeof(uint16_t);
            (void)memcpy(const_cast<uint8_t *>(addr->start), raddr.get_ip_bytes(), NSAPI_IPv6_BYTES);
            uint16_t port = raddr.get_port();
            (void)memcpy(const_cast<uint8_t *>(addr->start + NSAPI_IPv6_BYTES), &port, sizeof(uint16_t));
            break;
        } else {
            continue;  // FIXME: support error report on invalid packet to the upper layer
        }
    } while (1);

    return rb;
}

size_t _z_read_exact_udp_multicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len,
                                   const _z_sys_net_endpoint_t lep, _z_slice_t *addr) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_read_udp_multicast(sock, pos, len - n, lep, addr);
        if ((rb == SIZE_MAX) || (rb == 0)) {
            n = rb;
            break;
        }

        n = n + rb;
        pos = _z_ptr_u8_offset(pos, n);
    } while (n != len);

    return n;
}

size_t _z_send_udp_multicast(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len,
                             const _z_sys_net_endpoint_t rep) {
    return sock._udp->sendto(*rep._iptcp, ptr, len);
}
#endif

#if Z_FEATURE_LINK_SERIAL == 1

z_result_t _z_open_serial_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin, uint32_t baudrate) {
    sock->_serial = new BufferedSerial(PinName(txpin), PinName(rxpin), baudrate);
    if (sock->_serial != NULL) {
        sock->_serial->set_format(8, BufferedSerial::None, 1);  // Default in Zenoh Rust
        sock->_serial->enable_input(true);
        sock->_serial->enable_output(true);
    } else {
        return _Z_ERR_GENERIC;
    }
    return _z_connect_serial(*sock);
}

z_result_t _z_open_serial_from_dev(_z_sys_net_socket_t *sock, char *device, uint32_t baudrate) {
    z_result_t ret = _Z_RES_OK;

    (void)(sock);
    (void)(device);
    (void)(baudrate);
    ret = _Z_ERR_GENERIC;

    return ret;
}

z_result_t _z_listen_serial_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin, uint32_t baudrate) {
    z_result_t ret = _Z_RES_OK;
    (void)sock;
    (void)txpin;
    (void)rxpin;
    (void)baudrate;

    // @TODO: To be implemented
    ret = _Z_ERR_GENERIC;

    return ret;
}

z_result_t _z_listen_serial_from_dev(_z_sys_net_socket_t *sock, char *device, uint32_t baudrate) {
    z_result_t ret = _Z_RES_OK;

    (void)(sock);
    (void)(device);
    (void)(baudrate);
    ret = _Z_ERR_GENERIC;

    return ret;
}

void _z_close_serial(_z_sys_net_socket_t *sock) { delete sock->_serial; }

size_t _z_read_serial_internal(const _z_sys_net_socket_t sock, uint8_t *header, uint8_t *ptr, size_t len) {
    uint8_t *raw_buf = (uint8_t *)z_malloc(_Z_SERIAL_MAX_COBS_BUF_SIZE);
    size_t rb = 0;
    for (size_t i = 0; i < _Z_SERIAL_MAX_COBS_BUF_SIZE; i++) {
        sock._serial->read(&raw_buf[i], 1);
        rb++;
        if (raw_buf[i] == (uint8_t)0x00) {
            break;
        }
    }

    uint8_t *tmp_buf = (uint8_t *)z_malloc(_Z_SERIAL_MFS_SIZE);
    size_t ret = _z_serial_msg_deserialize(raw_buf, rb, ptr, len, header, tmp_buf, _Z_SERIAL_MFS_SIZE);

    z_free(raw_buf);
    z_free(tmp_buf);

    return ret;
}

size_t _z_send_serial_internal(const _z_sys_net_socket_t sock, uint8_t header, const uint8_t *ptr, size_t len) {
    uint8_t *tmp_buf = (uint8_t *)z_malloc(_Z_SERIAL_MFS_SIZE);
    uint8_t *raw_buf = (uint8_t *)z_malloc(_Z_SERIAL_MAX_COBS_BUF_SIZE);
    size_t ret =
        _z_serial_msg_serialize(raw_buf, _Z_SERIAL_MAX_COBS_BUF_SIZE, ptr, len, header, tmp_buf, _Z_SERIAL_MFS_SIZE);

    if (ret == SIZE_MAX) {
        return ret;
    }

    ssize_t wb = sock._serial->write(raw_buf, ret);
    if (wb != (ssize_t)ret) {
        ret = SIZE_MAX;
    }

    z_free(raw_buf);
    z_free(tmp_buf);

    return len;
}
#endif

#if Z_FEATURE_LINK_BLUETOOTH == 1
#error "Bluetooth not supported yet on MBED port of Zenoh-Pico"
#endif

#if Z_FEATURE_RAWETH_TRANSPORT == 1
#error "Raw ethernet transport not supported yet on MBED port of Zenoh-Pico"
#endif

}  // extern "C"
