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

#include <EthernetInterface.h>
#include <USBSerial.h>
#include <mbed.h>

extern "C" {
#include <netdb.h>
#include <string.h>

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/logging.h"

#if Z_LINK_TCP == 1
/*------------------ TCP sockets ------------------*/
void *_z_create_endpoint_tcp(const char *s_addr, const char *port) {
    SocketAddress *addr = new SocketAddress(s_addr, atoi(port));
    return addr;
}

void _z_free_endpoint_tcp(void *addr_arg) {
    SocketAddress *addr = (SocketAddress *)addr_arg;
    delete addr;
}

TCPSocket *_z_open_tcp(void *raddr_arg, uint32_t tout) {
    NetworkInterface *net = NetworkInterface::get_default_instance();
    SocketAddress *raddr = (SocketAddress *)raddr_arg;

    TCPSocket *sock = new TCPSocket();
    sock->set_timeout(tout);
    if (sock->open(net) < 0) goto _Z_OPEN_TCP_UNICAST_ERROR_1;

    if (sock->connect(*raddr) < 0) goto _Z_OPEN_TCP_UNICAST_ERROR_1;

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
    TCPSocket *sock = (TCPSocket *)sock_arg;
    if (sock == NULL) return;

    sock->close();
    delete sock;
}

size_t _z_read_tcp(void *sock_arg, uint8_t *ptr, size_t len) {
    TCPSocket *sock = (TCPSocket *)sock_arg;
    nsapi_size_or_error_t rb = sock->recv(ptr, len);
    if (rb < 0) return SIZE_MAX;

    return rb;
}

size_t _z_read_exact_tcp(void *sock_arg, uint8_t *ptr, size_t len) {
    size_t n = len;
    size_t rb = 0;

    do {
        rb = _z_read_tcp(sock_arg, ptr, n);
        if (rb == SIZE_MAX) return rb;

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > 0);

    return len;
}

size_t _z_send_tcp(void *sock_arg, const uint8_t *ptr, size_t len) {
    TCPSocket *sock = (TCPSocket *)sock_arg;
    return sock->send(ptr, len);
}
#endif

#if Z_LINK_UDP_UNICAST == 1 || Z_LINK_UDP_MULTICAST == 1
/*------------------ UDP sockets ------------------*/
void *_z_create_endpoint_udp(const char *s_addr, const char *port) {
    SocketAddress *addr = new SocketAddress(s_addr, atoi(port));
    return addr;
}

void _z_free_endpoint_udp(void *addr_arg) {
    SocketAddress *addr = (SocketAddress *)addr_arg;
    delete addr;
}
#endif

#if Z_LINK_UDP_UNICAST == 1
UDPSocket *_z_open_udp_unicast(void *raddr_arg, uint32_t tout) {
    NetworkInterface *net = NetworkInterface::get_default_instance();

    UDPSocket *sock = new UDPSocket();
    sock->set_timeout(tout);
    if (sock->open(net) < 0) goto _Z_OPEN_UDP_UNICAST_ERROR_1;

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
    UDPSocket *sock = (UDPSocket *)sock_arg;
    if (sock == NULL) return;

    sock->close();
    delete sock;
}

size_t _z_read_udp_unicast(void *sock_arg, uint8_t *ptr, size_t len) {
    UDPSocket *sock = (UDPSocket *)sock_arg;

    SocketAddress raddr;
    nsapi_size_or_error_t rb = sock->recvfrom(&raddr, ptr, len);
    if (rb < 0) return SIZE_MAX;

    return rb;
}

size_t _z_read_exact_udp_unicast(void *sock_arg, uint8_t *ptr, size_t len) {
    size_t n = len;
    size_t rb = 0;

    do {
        rb = _z_read_udp_unicast(sock_arg, ptr, n);
        if (rb == SIZE_MAX) return rb;

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > 0);

    return len;
}

size_t _z_send_udp_unicast(void *sock_arg, const uint8_t *ptr, size_t len, void *raddr_arg) {
    UDPSocket *sock = (UDPSocket *)sock_arg;
    SocketAddress *raddr = (SocketAddress *)raddr_arg;

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
    if (sock->open(net) < 0) goto _Z_OPEN_UDP_MULTICAST_ERROR_1;

    return sock;

_Z_OPEN_UDP_MULTICAST_ERROR_1:
    delete sock;
    return NULL;
}

UDPSocket *_z_listen_udp_multicast(void *raddr_arg, uint32_t tout, const char *iface) {
    NetworkInterface *net = NetworkInterface::get_default_instance();
    SocketAddress *raddr = (SocketAddress *)raddr_arg;

    UDPSocket *sock = new UDPSocket();
    sock->set_timeout(tout);
    if (sock->open(net) < 0) goto _Z_OPEN_UDP_MULTICAST_ERROR_1;

    if (sock->bind(raddr->get_port()) < 0) goto _Z_OPEN_UDP_MULTICAST_ERROR_1;

    if (sock->join_multicast_group(*raddr)) goto _Z_OPEN_UDP_MULTICAST_ERROR_1;

    return sock;

_Z_OPEN_UDP_MULTICAST_ERROR_1:
    delete sock;
    return NULL;
}

void _z_close_udp_multicast(void *sockrecv_arg, void *socksend_arg, void *raddr_arg) {
    UDPSocket *sockrecv = (UDPSocket *)sockrecv_arg;
    UDPSocket *socksend = (UDPSocket *)socksend_arg;
    SocketAddress *raddr = (SocketAddress *)raddr_arg;

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
    UDPSocket *sock = (UDPSocket *)sock_arg;
    SocketAddress raddr;
    nsapi_size_or_error_t rb = 0;

    do {
        rb = sock->recvfrom(&raddr, ptr, len);
        if (rb < 0) return SIZE_MAX;

        if (raddr.get_ip_version() == NSAPI_IPv4) {
            *addr = _z_bytes_make(NSAPI_IPv4_BYTES + sizeof(uint16_t));
            memcpy((void *)addr->start, raddr.get_ip_bytes(), NSAPI_IPv4_BYTES);
            uint16_t port = raddr.get_port();
            memcpy((void *)(addr->start + NSAPI_IPv4_BYTES), &port, sizeof(uint16_t));
            break;
        } else if (raddr.get_ip_version() == NSAPI_IPv6) {
            *addr = _z_bytes_make(NSAPI_IPv6_BYTES + sizeof(uint16_t));
            memcpy((void *)addr->start, raddr.get_ip_bytes(), NSAPI_IPv6_BYTES);
            uint16_t port = raddr.get_port();
            memcpy((void *)(addr->start + NSAPI_IPv6_BYTES), &port, sizeof(uint16_t));
            break;
        }
    } while (1);

    return rb;
}

size_t _z_read_exact_udp_multicast(void *sock_arg, uint8_t *ptr, size_t len, void *arg, _z_bytes_t *addr) {
    size_t n = len;
    size_t rb = 0;

    do {
        rb = _z_read_udp_multicast(sock_arg, ptr, n, arg, addr);
        if (rb == SIZE_MAX) return rb;

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > 0);

    return len;
}

size_t _z_send_udp_multicast(void *sock_arg, const uint8_t *ptr, size_t len, void *raddr_arg) {
    UDPSocket *sock = (UDPSocket *)sock_arg;
    SocketAddress *raddr = (SocketAddress *)raddr_arg;

    int wb = sock->sendto(*raddr, ptr, len);
    return wb;
}
#endif

#if Z_LINK_SERIAL == 1

#include "zenoh-pico/system/link/serial.h"
#include "zenoh-pico/utils/checksum.h"
#include "zenoh-pico/utils/encoding.h"

void *_z_open_serial(uint32_t txpin, uint32_t rxpin, uint32_t baudrate) {
    BufferedSerial *sock = new BufferedSerial(PinName(txpin), PinName(rxpin), baudrate);

    sock->set_format(8, BufferedSerial::None, 1);  // Default in Zenoh Rust
    sock->enable_input(true);
    sock->enable_output(true);

    return sock;
}

void *_z_listen_serial(uint32_t txpin, uint32_t rxpin, uint32_t baudrate) { return NULL; }

void _z_close_serial(void *sock_arg) {
    BufferedSerial *sock = (BufferedSerial *)sock_arg;
    delete sock;
}

size_t _z_read_serial(void *sock_arg, uint8_t *ptr, size_t len) {
    BufferedSerial *sock = (BufferedSerial *)sock_arg;

    uint8_t *before_cobs = (uint8_t *)z_malloc(_Z_SERIAL_MAX_COBS_BUF_SIZE);
    size_t rb = 0;
    for (size_t i = 0; i < _Z_SERIAL_MAX_COBS_BUF_SIZE; i++) {
        sock->read(&before_cobs[i], 1);
        rb = rb + 1;
        if (before_cobs[i] == 0x00) {
            break;
        }
    }

    uint8_t *after_cobs = (uint8_t *)z_malloc(_Z_SERIAL_MFS_SIZE);
    int trb = _z_cobs_decode(before_cobs, rb, after_cobs);

    size_t i = 0;
    uint16_t payload_len = 0;
    for (; i < sizeof(payload_len); i++) {
        payload_len |= (after_cobs[i] << (i * 8));
    }

    if (trb < payload_len + 6) {
        goto ERR;
    }

    memcpy(ptr, &after_cobs[i], payload_len);
    i = i + payload_len;

    {  // Limit the scope of CRC checks
        uint32_t crc = 0;
        for (unsigned int j = 0; j < sizeof(crc); i++, j++) {
            crc |= (after_cobs[i] << (j * 8));
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
    size_t rb = 0;

    do {
        rb = _z_read_serial(sock_arg, ptr, n);
        if (rb == SIZE_MAX) {
            return rb;
        }

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > 0);

    return len;
}

size_t _z_send_serial(void *sock_arg, const uint8_t *ptr, size_t len) {
    BufferedSerial *sock = (BufferedSerial *)sock_arg;

    uint8_t *before_cobs = (uint8_t *)z_malloc(_Z_SERIAL_MFS_SIZE);
    size_t i = 0;
    for (; i < sizeof(uint16_t); ++i) {
        before_cobs[i] = (len >> (i * 8)) & 0XFF;
    }

    memcpy(&before_cobs[i], ptr, len);
    i = i + len;

    uint32_t crc = _z_crc32(ptr, len);
    for (unsigned int j = 0; j < sizeof(crc); i++, j++) {
        before_cobs[i] = (crc >> (j * 8)) & 0XFF;
    }

    uint8_t *after_cobs = (uint8_t *)z_malloc(_Z_SERIAL_MAX_COBS_BUF_SIZE);
    int twb = _z_cobs_encode(before_cobs, i, after_cobs);
    after_cobs[twb] = 0x00;  // Manually add the COBS delimiter

    int wb = sock->write(after_cobs, twb + 1);
    if (wb != twb + 1) {
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