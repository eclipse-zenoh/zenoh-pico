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

#include <mbed.h>
#if Z_LINK_TCP == 1 || Z_LINK_UDP_UNICAST == 1 || Z_LINK_UDP_MULTICAST == 1
#include <EthernetInterface.h>
#endif
#if Z_LINK_SERIAL == 1
#include <USBSerial.h>
#endif

extern "C" {
#include <netdb.h>
#include <stddef.h>
#include <string.h>

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/system/link/serial.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/checksum.h"
#include "zenoh-pico/utils/encoding.h"
#include "zenoh-pico/utils/logging.h"

#if Z_LINK_TCP == 1
/*------------------ TCP sockets ------------------*/
void *_z_create_endpoint_tcp(const char *s_addr, const char *s_port) {
    // Parse and check the validity of the port
    uint32_t port = strtoul(s_port, NULL, 10);
    if ((port < (uint32_t)1) || (port > (uint32_t)65355)) {  // Port numbers should range from 1 to 65355
        goto ERR;
    }

    return new SocketAddress(s_addr, port);

ERR:
    return NULL;
}

void _z_free_endpoint_tcp(void *addr_arg) {
    SocketAddress *addr = static_cast<SocketAddress *>(addr_arg);
    delete addr;
}

TCPSocket *_z_open_tcp(void *raddr_arg, uint32_t tout) {
    NetworkInterface *net = NetworkInterface::get_default_instance();
    SocketAddress *raddr = static_cast<SocketAddress *>(raddr_arg);

    TCPSocket *sock = new TCPSocket();
    sock->set_timeout(tout);
    if (sock->open(net) < 0) {
        goto _Z_OPEN_TCP_UNICAST_ERROR_1;
    }

    if (sock->connect(*raddr) < 0) {
        goto _Z_OPEN_TCP_UNICAST_ERROR_1;
    }

    return sock;

_Z_OPEN_TCP_UNICAST_ERROR_1:
    delete sock;
    return NULL;
}

TCPSocket *_z_listen_tcp(void *raddr_arg) {
    // @TODO: to be implemented

    return NULL;
}

void _z_close_tcp(void *sock_arg) {
    TCPSocket *sock = static_cast<TCPSocket *>(sock_arg);
    if (sock == NULL) {
        return;
    }

    sock->close();
    delete sock;
}

size_t _z_read_tcp(void *sock_arg, uint8_t *ptr, size_t len) {
    TCPSocket *sock = static_cast<TCPSocket *>(sock_arg);
    nsapi_size_or_error_t rb = sock->recv(ptr, len);
    if (rb < 0) {
        return SIZE_MAX;
    }

    return rb;
}

size_t _z_read_exact_tcp(void *sock_arg, uint8_t *ptr, size_t len) {
    size_t n = len;

    do {
        size_t rb = _z_read_tcp(sock_arg, ptr, n);
        if (rb == SIZE_MAX) {
            return rb;
        }

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > (size_t)0);

    return len;
}

size_t _z_send_tcp(void *sock_arg, const uint8_t *ptr, size_t len) {
    TCPSocket *sock = static_cast<TCPSocket *>(sock_arg);
    return sock->send(ptr, len);
}
#endif

#if Z_LINK_UDP_UNICAST == 1 || Z_LINK_UDP_MULTICAST == 1
/*------------------ UDP sockets ------------------*/
void *_z_create_endpoint_udp(const char *s_addr, const char *s_port) {
    // Parse and check the validity of the port
    uint32_t port = strtoul(s_port, NULL, 10);
    if ((port < (uint32_t)1) || (port > (uint32_t)65355)) {  // Port numbers should range from 1 to 65355
        goto ERR;
    }

    return new SocketAddress(s_addr, port);

ERR:
    return NULL;
}

void _z_free_endpoint_udp(void *addr_arg) {
    SocketAddress *addr = static_cast<SocketAddress *>(addr_arg);
    delete addr;
}
#endif

#if Z_LINK_UDP_UNICAST == 1
UDPSocket *_z_open_udp_unicast(void *raddr_arg, uint32_t tout) {
    NetworkInterface *net = NetworkInterface::get_default_instance();

    UDPSocket *sock = new UDPSocket();
    sock->set_timeout(tout);
    if (sock->open(net) < 0) {
        goto _Z_OPEN_UDP_UNICAST_ERROR_1;
    }

    return sock;

_Z_OPEN_UDP_UNICAST_ERROR_1:
    delete sock;
    return NULL;
}

UDPSocket *_z_listen_udp_unicast(void *raddr_arg, uint32_t tout) {
    // @TODO: To be implemented

    return NULL;
}

void _z_close_udp_unicast(void *sock_arg) {
    UDPSocket *sock = static_cast<UDPSocket *>(sock_arg);
    if (sock == NULL) {
        return;
    }

    sock->close();
    delete sock;
}

size_t _z_read_udp_unicast(void *sock_arg, uint8_t *ptr, size_t len) {
    UDPSocket *sock = static_cast<UDPSocket *>(sock_arg);

    SocketAddress raddr;
    nsapi_size_or_error_t rb = sock->recvfrom(&raddr, ptr, len);
    if (rb < 0) {
        return SIZE_MAX;
    }

    return rb;
}

size_t _z_read_exact_udp_unicast(void *sock_arg, uint8_t *ptr, size_t len) {
    size_t n = len;

    do {
        size_t rb = _z_read_udp_unicast(sock_arg, ptr, n);
        if (rb == SIZE_MAX) {
            return rb;
        }

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > (size_t)0);

    return len;
}

size_t _z_send_udp_unicast(void *sock_arg, const uint8_t *ptr, size_t len, void *raddr_arg) {
    UDPSocket *sock = static_cast<UDPSocket *>(sock_arg);
    SocketAddress *raddr = static_cast<SocketAddress *>(raddr_arg);

    int sb = sock->sendto(*raddr, ptr, len);
    return sb;
}
#endif

#if Z_LINK_UDP_MULTICAST == 1
UDPSocket *_z_open_udp_multicast(void *raddr_arg, void **laddr_arg, uint32_t tout, const char *iface) {
    *laddr_arg = NULL;  // Multicast messages are not self-consumed,
                        // so no need to save the local address

    NetworkInterface *net = NetworkInterface::get_default_instance();

    UDPSocket *sock = new UDPSocket();
    sock->set_timeout(tout);
    if (sock->open(net) < 0) {
        goto _Z_OPEN_UDP_MULTICAST_ERROR_1;
    }

    return sock;

_Z_OPEN_UDP_MULTICAST_ERROR_1:
    delete sock;
    return NULL;
}

UDPSocket *_z_listen_udp_multicast(void *raddr_arg, uint32_t tout, const char *iface) {
    NetworkInterface *net = NetworkInterface::get_default_instance();
    SocketAddress *raddr = static_cast<SocketAddress *>(raddr_arg);

    UDPSocket *sock = new UDPSocket();
    sock->set_timeout(tout);
    if (sock->open(net) < 0) {
        goto _Z_OPEN_UDP_MULTICAST_ERROR_1;
    }

    if (sock->bind(raddr->get_port()) < 0) {
        goto _Z_OPEN_UDP_MULTICAST_ERROR_1;
    }

    if (sock->join_multicast_group(*raddr) < 0) {
        goto _Z_OPEN_UDP_MULTICAST_ERROR_1;
    }

    return sock;

_Z_OPEN_UDP_MULTICAST_ERROR_1:
    delete sock;
    return NULL;
}

void _z_close_udp_multicast(void *sockrecv_arg, void *socksend_arg, void *raddr_arg) {
    UDPSocket *sockrecv = static_cast<UDPSocket *>(sockrecv_arg);
    UDPSocket *socksend = static_cast<UDPSocket *>(socksend_arg);
    SocketAddress *raddr = static_cast<SocketAddress *>(raddr_arg);

    // Both sockrecv and socksend must be compared to NULL,
    //  because we dont know if the close is trigger by a normal close
    //  or some of the sockets failed during the opening/listening procedure.
    if (sockrecv != NULL) {
        sockrecv->leave_multicast_group(*raddr);

        sockrecv->close();
        delete sockrecv;
    }

    if (sockrecv != NULL) {
        socksend->close();
        delete socksend;
    }
}

size_t _z_read_udp_multicast(void *sock_arg, uint8_t *ptr, size_t len, void *laddr_arg, _z_bytes_t *addr) {
    UDPSocket *sock = static_cast<UDPSocket *>(sock_arg);
    SocketAddress raddr;
    nsapi_size_or_error_t rb = 0;

    do {
        rb = sock->recvfrom(&raddr, ptr, len);
        if (rb < 0) {
            return SIZE_MAX;
        }

        if (raddr.get_ip_version() == NSAPI_IPv4) {
            *addr = _z_bytes_make(NSAPI_IPv4_BYTES + sizeof(uint16_t));
            (void)memcpy(const_cast<uint8_t *>(addr->start), raddr.get_ip_bytes(), NSAPI_IPv4_BYTES);
            uint16_t port = raddr.get_port();
            (void)memcpy(const_cast<uint8_t *>(addr->start + NSAPI_IPv4_BYTES), &port, sizeof(uint16_t));
            break;
        } else if (raddr.get_ip_version() == NSAPI_IPv6) {
            *addr = _z_bytes_make(NSAPI_IPv6_BYTES + sizeof(uint16_t));
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

size_t _z_read_exact_udp_multicast(void *sock_arg, uint8_t *ptr, size_t len, void *arg, _z_bytes_t *addr) {
    size_t n = len;

    do {
        size_t rb = _z_read_udp_multicast(sock_arg, ptr, n, arg, addr);
        if (rb == SIZE_MAX) {
            return rb;
        }

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > (size_t)0);

    return len;
}

size_t _z_send_udp_multicast(void *sock_arg, const uint8_t *ptr, size_t len, void *raddr_arg) {
    UDPSocket *sock = static_cast<UDPSocket *>(sock_arg);
    SocketAddress *raddr = static_cast<SocketAddress *>(raddr_arg);

    int wb = sock->sendto(*raddr, ptr, len);
    return wb;
}
#endif

#if Z_LINK_SERIAL == 1

void *_z_open_serial(uint32_t txpin, uint32_t rxpin, uint32_t baudrate) {
    BufferedSerial *sock = new BufferedSerial(PinName(txpin), PinName(rxpin), baudrate);

    sock->set_format(8, BufferedSerial::None, 1);  // Default in Zenoh Rust
    sock->enable_input(true);
    sock->enable_output(true);

    return sock;
}

void *_z_listen_serial(uint32_t txpin, uint32_t rxpin, uint32_t baudrate) { return NULL; }

void _z_close_serial(void *sock_arg) {
    BufferedSerial *sock = static_cast<BufferedSerial *>(sock_arg);
    delete sock;
}

size_t _z_read_serial(void *sock_arg, uint8_t *ptr, size_t len) {
    BufferedSerial *sock = static_cast<BufferedSerial *>(sock_arg);

    uint8_t *before_cobs = new uint8_t[_Z_SERIAL_MAX_COBS_BUF_SIZE]();
    size_t rb = 0;
    for (size_t i = 0; i < _Z_SERIAL_MAX_COBS_BUF_SIZE; i++) {
        sock->read(&before_cobs[i], 1);
        rb = rb + (size_t)1;
        if (before_cobs[i] == (uint8_t)0x00) {
            break;
        }
    }

    uint8_t *after_cobs = new uint8_t[_Z_SERIAL_MFS_SIZE]();
    size_t trb = _z_cobs_decode(before_cobs, rb, after_cobs);

    size_t i = 0;
    uint16_t payload_len = 0;
    for (; i < sizeof(payload_len); i++) {
        payload_len |= (after_cobs[i] << ((uint8_t)i * (uint8_t)8));
    }

    if (trb < ((size_t)payload_len + (size_t)6)) {
        goto ERR;
    }

    (void)memcpy(ptr, &after_cobs[i], payload_len);
    i = i + payload_len;

    {  // Limit the scope of CRC checks
        uint32_t crc = 0;
        for (uint8_t j = 0; j < sizeof(crc); j++) {
            crc |= (after_cobs[i] << ((uint8_t)j * (uint8_t)8));
            i += 1;
        }

        uint32_t c_crc = _z_crc32(ptr, payload_len);
        if (c_crc != crc) {
            goto ERR;
        }
    }

    free(before_cobs);
    free(after_cobs);

    return payload_len;

ERR:
    free(before_cobs);
    free(after_cobs);

    return SIZE_MAX;
}

size_t _z_read_exact_serial(void *sock_arg, uint8_t *ptr, size_t len) {
    size_t n = len;

    do {
        size_t rb = _z_read_serial(sock_arg, ptr, n);
        if (rb == SIZE_MAX) {
            return rb;
        }

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > (size_t)0);

    return len;
}

size_t _z_send_serial(void *sock_arg, const uint8_t *ptr, size_t len) {
    BufferedSerial *sock = static_cast<BufferedSerial *>(sock_arg);

    uint8_t *before_cobs = new uint8_t[_Z_SERIAL_MFS_SIZE]();
    size_t i = 0;
    for (; i < sizeof(uint16_t); ++i) {
        before_cobs[i] = (len >> (i * (size_t)8)) & (size_t)0XFF;
    }

    (void)memcpy(&before_cobs[i], ptr, len);
    i = i + len;

    uint32_t crc = _z_crc32(ptr, len);
    for (uint32_t j = 0; j < sizeof(crc); j++) {
        before_cobs[i] = (crc >> (j * (uint32_t)8)) & (uint32_t)0XFF;
        i += 1;
    }

    uint8_t *after_cobs = new uint8_t[_Z_SERIAL_MAX_COBS_BUF_SIZE]();
    int twb = _z_cobs_encode(before_cobs, i, after_cobs);
    after_cobs[twb] = 0x00;  // Manually add the COBS delimiter

    int wb = sock->write(after_cobs, twb + 1);
    if (wb != (twb + 1)) {
        goto ERR;
    }

    free(before_cobs);
    free(after_cobs);
    return len;

ERR:
    free(before_cobs);
    free(after_cobs);

    return SIZE_MAX;
}
#endif

#if Z_LINK_BLUETOOTH == 1
#error "Bluetooth not supported yet on MBED port of Zenoh-Pico"
#endif

}  // extern "C"