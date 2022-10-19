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
#include "zenoh-pico/system/link/serial.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/checksum.h"
#include "zenoh-pico/utils/encoding.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

#if Z_LINK_TCP == 1
/*------------------ TCP sockets ------------------*/
_z_sys_net_endpoint_t _z_create_endpoint_tcp(const char *s_addr, const char *s_port) {
    _z_sys_net_endpoint_t ep;

    // Parse and check the validity of the port
    uint32_t port = strtoul(s_port, NULL, 10);
    if ((port < (uint32_t)1) || (port > (uint32_t)65355)) {  // Port numbers should range from 1 to 65355
        goto ERR;
    }

    ep._addr = new SocketAddress(s_addr, port);

    ep._err = true;
    return ep;

ERR:
    ep._err = false;
    return ep;
}

void _z_free_endpoint_tcp(_z_sys_net_endpoint_t ep) { delete ep._addr; }

_z_sys_net_socket_t _z_open_tcp(_z_sys_net_endpoint_t rep, uint32_t tout) {
    _z_sys_net_socket_t sock;

    NetworkInterface *net = NetworkInterface::get_default_instance();

    sock._tcp = new TCPSocket();
    sock._tcp->set_timeout(tout);
    if (sock._tcp->open(net) < 0) {
        goto _Z_OPEN_TCP_ERROR_1;
    }

    if (sock._tcp->connect(*rep._addr) < 0) {
        goto _Z_OPEN_TCP_ERROR_1;
    }

    sock._err = false;
    return sock;

_Z_OPEN_TCP_ERROR_1:
    delete sock._tcp;
    sock._err = true;
    return sock;
}

_z_sys_net_socket_t _z_listen_tcp(_z_sys_net_endpoint_t lep) {
    _z_sys_net_socket_t sock;
    (void)lep;

    // @TODO: To be implemented

    sock._err = true;
    return sock;
}

void _z_close_tcp(_z_sys_net_socket_t sock) {
    if (sock._err == true) {
        return;
    }

    sock._tcp->close();
    delete sock._tcp;
}

size_t _z_read_tcp(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    nsapi_size_or_error_t rb = sock._tcp->recv(ptr, len);
    if (rb < 0) {
        return SIZE_MAX;
    }

    return rb;
}

size_t _z_read_exact_tcp(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_read_tcp(sock, pos, len - n);
        if (rb == SIZE_MAX) {
            n = rb;
            break;
        }

        n += rb;
        pos = _z_ptr_u8_offset(pos, n);
    } while (n != len);

    return n;
}

size_t _z_send_tcp(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) { return sock._tcp->send(ptr, len); }
#endif

#if Z_LINK_UDP_UNICAST == 1 || Z_LINK_UDP_MULTICAST == 1
/*------------------ UDP sockets ------------------*/
_z_sys_net_endpoint_t _z_create_endpoint_udp(const char *s_addr, const char *s_port) {
    _z_sys_net_endpoint_t ep;

    // Parse and check the validity of the port
    uint32_t port = strtoul(s_port, NULL, 10);
    if ((port < (uint32_t)1) || (port > (uint32_t)65355)) {  // Port numbers should range from 1 to 65355
        goto ERR;
    }

    ep._addr = new SocketAddress(s_addr, port);

    ep._err = true;
    return ep;

ERR:
    ep._err = false;
    return ep;
}

void _z_free_endpoint_udp(_z_sys_net_endpoint_t ep) {
    SocketAddress *addr = reinterpret_cast<SocketAddress *>(ep._addr);
    delete addr;
}
#endif

#if Z_LINK_UDP_UNICAST == 1
_z_sys_net_socket_t _z_open_udp_unicast(_z_sys_net_endpoint_t rep, uint32_t tout) {
    _z_sys_net_socket_t sock;
    (void)(rep);

    NetworkInterface *net = NetworkInterface::get_default_instance();

    sock._udp = new UDPSocket();
    sock._udp->set_timeout(tout);
    if (sock._udp->open(net) < 0) {
        goto _Z_OPEN_UDP_UNICAST_ERROR_1;
    }

    sock._err = false;
    return sock;

_Z_OPEN_UDP_UNICAST_ERROR_1:
    delete sock._udp;
    sock._err = true;
    return sock;
}

_z_sys_net_socket_t _z_listen_udp_unicast(_z_sys_net_endpoint_t lep, uint32_t tout) {
    _z_sys_net_socket_t sock;
    (void)lep;
    (void)tout;

    // @TODO: To be implemented

    sock._err = false;
    return sock;
}

void _z_close_udp_unicast(_z_sys_net_socket_t sock) {
    if (sock._err == true) {
        return;
    }

    sock._udp->close();
    delete sock._udp;
}

size_t _z_read_udp_unicast(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    SocketAddress raddr;
    nsapi_size_or_error_t rb = sock._udp->recvfrom(&raddr, ptr, len);
    if (rb < 0) {
        return SIZE_MAX;
    }

    return rb;
}

size_t _z_read_exact_udp_unicast(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_read_udp_unicast(sock, pos, len - n);
        if (rb == SIZE_MAX) {
            n = rb;
            break;
        }

        n += rb;
        pos = _z_ptr_u8_offset(pos, n);
    } while (n != len);

    return n;
}

size_t _z_send_udp_unicast(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len, _z_sys_net_endpoint_t rep) {
    int sb = sock._udp->sendto(*rep._addr, ptr, len);
    return sb;
}
#endif

#if Z_LINK_UDP_MULTICAST == 1
_z_sys_net_socket_t _z_open_udp_multicast(_z_sys_net_endpoint_t rep, _z_sys_net_endpoint_t *lep, uint32_t tout,
                                          const char *iface) {
    _z_sys_net_socket_t sock;
    (void)(lep);  // Multicast messages are not self-consumed, so no need to save the local address

    NetworkInterface *net = NetworkInterface::get_default_instance();

    sock._udp = new UDPSocket();
    sock._udp->set_timeout(tout);
    if (sock._udp->open(net) < 0) {
        goto _Z_OPEN_UDP_MULTICAST_ERROR_1;
    }

    sock._err = false;
    return sock;

_Z_OPEN_UDP_MULTICAST_ERROR_1:
    delete sock._udp;
    sock._err = true;
    return sock;
}

_z_sys_net_socket_t _z_listen_udp_multicast(_z_sys_net_endpoint_t rep, uint32_t tout, const char *iface) {
    _z_sys_net_socket_t sock;

    NetworkInterface *net = NetworkInterface::get_default_instance();

    sock._udp = new UDPSocket();
    sock._udp->set_timeout(tout);
    if (sock._udp->open(net) < 0) {
        goto _Z_LISTEN_UDP_MULTICAST_ERROR_1;
    }

    if (sock._udp->bind(rep._addr->get_port()) < 0) {
        goto _Z_LISTEN_UDP_MULTICAST_ERROR_1;
    }

    if (sock._udp->join_multicast_group(*rep._addr) < 0) {
        goto _Z_LISTEN_UDP_MULTICAST_ERROR_1;
    }

    sock._err = false;
    return sock;

_Z_LISTEN_UDP_MULTICAST_ERROR_1:
    delete sock._udp;
    sock._err = true;
    return sock;
}

void _z_close_udp_multicast(_z_sys_net_socket_t sockrecv, _z_sys_net_socket_t socksend, _z_sys_net_endpoint_t rep) {
    // Both sockrecv and socksend must be compared to err,
    //  because we dont know if the close is trigger by a normal close
    //  or some of the sockets failed during the opening/listening procedure.
    if (sockrecv._err != true) {
        sockrecv._udp->leave_multicast_group(*rep._addr);
        sockrecv._udp->close();
        delete sockrecv._udp;
    }

    if (socksend._err != true) {
        socksend._udp->close();
        delete socksend._udp;
    }
}

size_t _z_read_udp_multicast(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len, _z_sys_net_endpoint_t lep,
                             _z_bytes_t *addr) {
    SocketAddress raddr;
    nsapi_size_or_error_t rb = 0;

    do {
        rb = sock._udp->recvfrom(&raddr, ptr, len);
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

size_t _z_read_exact_udp_multicast(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len, _z_sys_net_endpoint_t lep,
                                   _z_bytes_t *addr) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_read_udp_multicast(sock, pos, len - n, lep, addr);
        if (rb == SIZE_MAX) {
            n = rb;
            break;
        }

        n += rb;
        pos = _z_ptr_u8_offset(pos, n);
    } while (n != len);

    return n;
}

size_t _z_send_udp_multicast(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len, _z_sys_net_endpoint_t rep) {
    int wb = sock._udp->sendto(*rep._addr, ptr, len);
    return wb;
}
#endif

#if Z_LINK_SERIAL == 1

_z_sys_net_socket_t _z_open_serial(uint32_t txpin, uint32_t rxpin, uint32_t baudrate) {
    _z_sys_net_socket_t sock;
    sock._serial = new BufferedSerial(PinName(txpin), PinName(rxpin), baudrate);

    sock._serial->set_format(8, BufferedSerial::None, 1);  // Default in Zenoh Rust
    sock._serial->enable_input(true);
    sock._serial->enable_output(true);

    sock._serial = sock._serial;
    sock._err = false;
    return sock;
}

_z_sys_net_socket_t _z_listen_serial(uint32_t txpin, uint32_t rxpin, uint32_t baudrate) {
    _z_sys_net_socket_t sock;
    (void)(txpin);
    (void)(rxpin);
    (void)(baudrate);

    sock._err = true;
    return sock;
}

void _z_close_serial(_z_sys_net_socket_t sock) {
    if (sock._err == true) {
        return;
    }

    delete sock._serial;
}

size_t _z_read_serial(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    uint8_t *before_cobs = new uint8_t[_Z_SERIAL_MAX_COBS_BUF_SIZE]();
    size_t rb = 0;
    for (size_t i = 0; i < _Z_SERIAL_MAX_COBS_BUF_SIZE; i++) {
        sock._serial->read(&before_cobs[i], 1);
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
    i = i + (size_t)payload_len;

    {  // Limit the scope of CRC checks
        uint32_t crc = 0;
        for (uint8_t j = 0; j < sizeof(crc); j++) {
            crc |= (uint32_t)(after_cobs[i] << (j * (uint8_t)8));
            i += (size_t)1;
        }

        uint32_t c_crc = _z_crc32(ptr, payload_len);
        if (c_crc != crc) {
            goto ERR;
        }
    }

    delete before_cobs;
    delete after_cobs;

    return payload_len;

ERR:
    delete before_cobs;
    delete after_cobs;

    return SIZE_MAX;
}

size_t _z_read_exact_serial(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_read_serial(sock, ptr, len - n);
        if (rb == SIZE_MAX) {
            n = rb;
            break;
        }

        n += rb;
        pos = _z_ptr_u8_offset(pos, n);
    } while (n != len);

    return n;
}

size_t _z_send_serial(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    uint8_t *before_cobs = new uint8_t[_Z_SERIAL_MFS_SIZE]();
    size_t i = 0;
    for (; i < sizeof(uint16_t); ++i) {
        before_cobs[i] = (len >> (i * (size_t)8)) & (size_t)0XFF;
    }

    (void)memcpy(&before_cobs[i], ptr, len);
    i = i + len;

    uint32_t crc = _z_crc32(ptr, len);
    for (uint8_t j = 0; j < sizeof(crc); j++) {
        before_cobs[i] = (crc >> (j * (uint8_t)8)) & (uint32_t)0XFF;
        i += (size_t)1;
    }

    uint8_t *after_cobs = new uint8_t[_Z_SERIAL_MAX_COBS_BUF_SIZE]();
    ssize_t twb = _z_cobs_encode(before_cobs, i, after_cobs);
    after_cobs[twb] = 0x00;  // Manually add the COBS delimiter

    ssize_t wb = sock._serial->write(after_cobs, twb + (ssize_t)1);
    if (wb != (twb + (ssize_t)1)) {
        goto ERR;
    }

    delete before_cobs;
    delete after_cobs;
    return len;

ERR:
    delete before_cobs;
    delete after_cobs;

    return SIZE_MAX;
}
#endif

#if Z_LINK_BLUETOOTH == 1
#error "Bluetooth not supported yet on MBED port of Zenoh-Pico"
#endif

}  // extern "C"