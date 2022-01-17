/*
 * Copyright (c) 2017, 2021 ADLINK Technology Inc.
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

#include "zenoh-pico/config.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/collections/string.h"

#include <string.h>
#include <socket/include/socket.h>

int _zn_inet_pton(const z_str_t address, struct sockaddr_in *addr)
{
    unsigned long ret = 0;

    int shift = 0;
    const char *it = address - 1;
    const char *last = &address[strlen(address)];
    while (it != last)
    {
        const char *p_start = ++it;
        const char *p_end = strchr(it, '.');
        if (p_end == NULL)
            p_end = last;

        size_t len = p_end - p_start;
        char *s_octet = (char *)malloc((len + 1) * sizeof(char));
        strncpy(s_octet, p_start, len);
        s_octet[len] = '\0';

        uint8_t octet = atoi(s_octet);
        free(s_octet);
        if (octet > 255)
            return -1;

        ret += (octet << shift);
        shift += 8;
        it = p_end;
    }

    addr->sin_addr.s_addr = ret;
    return 0;
}

// Global variables used in the wifi handler
int s_state = 0;
unsigned char *s_rcv_buf = NULL;
ssize_t s_rb = 0;

void _zn_socket_event_handler(SOCKET sock, uint8_t ev_type, void *ev_msg)
{
    switch (ev_type)
    {
        case SOCKET_MSG_BIND:
        {
            s_state = 1;
        } break;

        case SOCKET_MSG_RECV:
        case SOCKET_MSG_RECVFROM:
        {
            tstrSocketRecvMsg *ev_recv_msg = (tstrSocketRecvMsg *)ev_msg;
//            uint32 strRemoteHostAddr = ev_recv_msg->strRemoteAddr.sin_addr.s_addr;
//            uint16 u16port = ev_recv_msg->strRemoteAddr.sin_port;

            if (ev_recv_msg->s16BufferSize <= 0)
            {
                s_rb = -1;
                return;
            }

            s_rb = ev_recv_msg->s16BufferSize;
            if (hif_receive(ev_recv_msg->pu8Buffer, s_rcv_buf, s_rb, 0) != M2M_SUCCESS)
            {
                s_rb = -1;
                return;
            }
            hif_receive(0, NULL, 0, 1); // Discard message from the buffer after read
        } break;

        case SOCKET_MSG_SEND:
        case SOCKET_MSG_SENDTO:
        {
            // Do nothing at the moment
        } break;

        default:
            break;
    }
}

/*------------------ Endpoint ------------------*/
void *_zn_create_endpoint_tcp(const z_str_t s_addr, const z_str_t port)
{
    // @TODO: To be implemented

    return -1;
}

void *_zn_create_endpoint_udp(const z_str_t s_addr, const z_str_t port)
{
    struct sockaddr_in *addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    memset(addr, 0, sizeof(addr));

    addr->sin_family = AF_INET;
    _zn_inet_pton(s_addr, addr);
    addr->sin_port = _htons(atoi(port));
    
    return addr;
}

void _zn_free_endpoint_tcp(void *arg)
{
    // @TODO: To be implemented

    return -1;
}

void _zn_free_endpoint_udp(void *arg)
{
    struct sockaddr_in *self = (struct sockaddr_in *)arg;

    free(self);
}

/*------------------ TCP sockets ------------------*/
int _zn_open_tcp(void *arg)
{
    // @TODO: To be implemented

    return -1;
}

int _zn_listen_tcp(void *arg)
{
    // @TODO: To be implemented

    return -1;
}

void _zn_close_tcp(int sock)
{
    // @TODO: To be implemented
}

size_t _zn_read_tcp(int sock, uint8_t *ptr, size_t len)
{
    // @TODO: To be implemented

    return -1;
}

size_t _zn_read_exact_tcp(int sock, uint8_t *ptr, size_t len)
{
    // @TODO: To be implemented

    return -1;
}

size_t _zn_send_tcp(int sock, const uint8_t *ptr, size_t len)
{
    // @TODO: To be implemented

    return -1;
}

/*------------------ UDP sockets ------------------*/
int _zn_open_udp_unicast(void *arg, const clock_t tout)
{
    struct sockaddr_in *raddr = (struct sockaddr_in *)arg;

    registerSocketCallback(_zn_socket_event_handler, NULL);

    int sock = socket(raddr->sin_family, SOCK_DGRAM, 0);
    if (sock < 0)
        goto _ZN_OPEN_UDP_UNICAST_ERROR_1;

    struct sockaddr_in *addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    memset(addr, 0, sizeof(addr));
    addr->sin_family = AF_INET;
    if (bind(sock, (struct sockaddr*)addr, sizeof(struct sockaddr_in)) < 0)
        goto _ZN_OPEN_UDP_UNICAST_ERROR_2;

    s_state = 0; // Global variable used in wifi handle
    z_clock_t start = z_clock_now();
    while (s_state != 1 && z_clock_elapsed_ms(&start) < 2000)
        m2m_wifi_handle_events(NULL);
    if(s_state == 0)
        goto _ZN_OPEN_UDP_UNICAST_ERROR_2;

    // This is required after binding (just for UDP) due to WiFi101 library
    recvfrom(sock, NULL, 0, 0);

    return sock;

_ZN_OPEN_UDP_UNICAST_ERROR_2:
    close(sock);

_ZN_OPEN_UDP_UNICAST_ERROR_1:
    return -1;
}

int _zn_listen_udp_unicast(void *arg, const clock_t tout)
{
    struct sockaddr_in *laddr = (struct sockaddr_in *)arg;

    // @TODO: To be implemented

    return -1;
}

void _zn_close_udp_unicast(int sock)
{
    m2m_wifi_handle_events(NULL);
    close(sock);
}

size_t _zn_read_udp_unicast(int sock, uint8_t *ptr, size_t len)
{
    // Global variables used in wifi handle
    s_rb = 0;
    s_rcv_buf = ptr;

    while (s_rb == 0)
    {
        m2m_wifi_handle_events(NULL);
        if (recvfrom(sock, ptr, len, 0) < 0)
            return -1;
    }

    return s_rb;
}

size_t _zn_read_exact_udp_unicast(int sock, uint8_t *ptr, size_t len)
{
    size_t n = len;
    size_t rb = 0;

    do
    {
        rb = _zn_read_udp_unicast(sock, ptr, n);
        if (rb < 0)
            return rb;

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > 0);

    return len;
}

size_t _zn_send_udp_unicast(int sock, const uint8_t *ptr, size_t len, void *arg)
{
    m2m_wifi_handle_events(NULL);

    struct sockaddr_in *raddr = (struct sockaddr_in *)arg;
    if (sendto(sock, (void*)ptr, len, 0, (struct sockaddr*)raddr, sizeof(struct sockaddr_in)) < 0)
        return -1;

    m2m_wifi_handle_events(NULL);

    return len;
}

int _zn_open_udp_multicast(void *arg_1, void **arg_2, const clock_t tout, const z_str_t iface)
{
    // @TODO: To be implemented

    return -1;
}

int _zn_listen_udp_multicast(void *arg, const clock_t tout, const z_str_t iface)
{
    // @TODO: To be implemented

    return -1;
}

void _zn_close_udp_multicast(int sock_recv, int sock_send, void *arg)
{
    // @TODO: To be implemented
}

size_t _zn_read_udp_multicast(int sock, uint8_t *ptr, size_t len, void *arg, z_bytes_t *addr)
{
    // @TODO: To be implemented

    return -1;
}

size_t _zn_read_exact_udp_multicast(int sock, uint8_t *ptr, size_t len, void *arg, z_bytes_t *addr)
{
    // @TODO: To be implemented

    return -1;
}

size_t _zn_send_udp_multicast(int sock, const uint8_t *ptr, size_t len, void *arg)
{
    // @TODO: To be implemented

    return -1;
}
