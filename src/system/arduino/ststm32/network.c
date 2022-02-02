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
#include <driver/include/m2m_wifi.h>
#include <driver/source/m2m_hif.h>

typedef struct
{
    enum
    {
        __ZN_SOCKET_BRIDGE_UNINITIALIZED = 0,
        __ZN_SOCKET_BRIDGE_BIND_OR_CONNECTED = 1,
        __ZN_SOCKET_BRIDGE_ERROR = 255,
    } state;

    char *buffer;
    char *buffer_current;
    uint16_t buffer_lenght;
    uint16_t buffer_available;
} __zn_wifi_socket;

__zn_wifi_socket g_wifi_buffer;

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

void _zn_socket_event_handler(SOCKET sock, uint8_t ev_type, void *ev_msg)
{
    switch (ev_type)
    {
        case SOCKET_MSG_BIND:
        {
            tstrSocketBindMsg *ev_bind_msg = (tstrSocketBindMsg *)ev_msg;
            if (ev_bind_msg && ev_bind_msg->status == -1)
                g_wifi_buffer.state = __ZN_SOCKET_BRIDGE_BIND_OR_CONNECTED;
            else
                g_wifi_buffer.state = __ZN_SOCKET_BRIDGE_ERROR;

        } break;

        case SOCKET_MSG_CONNECT:
        {
            tstrSocketConnectMsg *ev_conn_msg = (tstrSocketConnectMsg *)ev_msg;
            if (ev_conn_msg && ev_conn_msg->s8Error >= 0)
                g_wifi_buffer.state = __ZN_SOCKET_BRIDGE_BIND_OR_CONNECTED;
            else
                g_wifi_buffer.state = __ZN_SOCKET_BRIDGE_ERROR;

        } break;

        case SOCKET_MSG_RECV:
        case SOCKET_MSG_RECVFROM:
        {
            tstrSocketRecvMsg *ev_recv_msg = (tstrSocketRecvMsg *)ev_msg;
//            uint32 strRemoteHostAddr = ev_recv_msg->strRemoteAddr.sin_addr.s_addr;
//            uint16 u16port = ev_recv_msg->strRemoteAddr.sin_port;

            if (ev_recv_msg->s16BufferSize <= 0)
                return;

            if (g_wifi_buffer.buffer_available < ev_recv_msg->s16BufferSize)
                return;

            unsigned int to_read = ev_recv_msg->s16BufferSize;
            if (hif_receive(ev_recv_msg->pu8Buffer, (unsigned char *)g_wifi_buffer.buffer_current, to_read, 1) != M2M_SUCCESS)
                return;

            g_wifi_buffer.buffer_current += to_read;
            g_wifi_buffer.buffer_lenght += to_read;
            g_wifi_buffer.buffer_available -= to_read;

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
    struct sockaddr_in *addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    memset(addr, 0, sizeof(addr));

    addr->sin_family = AF_INET;
    _zn_inet_pton(s_addr, addr);
    addr->sin_port = _htons(atoi(port));

    return addr;
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
    struct sockaddr_in *self = (struct sockaddr_in *)arg;

    free(self);
}

void _zn_free_endpoint_udp(void *arg)
{
    struct sockaddr_in *self = (struct sockaddr_in *)arg;

    free(self);
}

/*------------------ TCP sockets ------------------*/
int _zn_open_tcp(void *arg)
{
    struct sockaddr_in *raddr = (struct sockaddr_in *)arg;

    registerSocketCallback(_zn_socket_event_handler, NULL);

    int sock = socket(raddr->sin_family, SOCK_STREAM, 0);
    if (sock < 0)
        goto _ZN_OPEN_TCP_UNICAST_ERROR_1;

    if (connect(sock, (struct sockaddr*)raddr, sizeof(struct sockaddr_in)) < 0)
        goto _ZN_OPEN_TCP_UNICAST_ERROR_2;

    g_wifi_buffer.state = __ZN_SOCKET_BRIDGE_UNINITIALIZED;
    z_clock_t start = z_clock_now();
    while (g_wifi_buffer.state  != 1 && z_clock_elapsed_ms(&start) < 2000)
        m2m_wifi_handle_events(NULL);
    if(g_wifi_buffer.state != __ZN_SOCKET_BRIDGE_BIND_OR_CONNECTED)
        goto _ZN_OPEN_TCP_UNICAST_ERROR_2;

    // This is required after binding (just for TCP) due to WiFi101 library
    recv(sock, NULL, 0, 0);

    g_wifi_buffer.buffer_lenght = 0;
    g_wifi_buffer.buffer_available = ZN_BATCH_SIZE;
    g_wifi_buffer.buffer = (char *)malloc(g_wifi_buffer.buffer_available);
    g_wifi_buffer.buffer_current = g_wifi_buffer.buffer;

    return sock;

_ZN_OPEN_TCP_UNICAST_ERROR_2:
    close(sock);

_ZN_OPEN_TCP_UNICAST_ERROR_1:
    return -1;
}

int _zn_listen_tcp(void *arg)
{
    struct sockaddr_in *laddr = (struct sockaddr_in *)arg;

    // @TODO: To be implemented

    return -1;
}

void _zn_close_tcp(int sock)
{
    m2m_wifi_handle_events(NULL);
    close(sock);
    m2m_wifi_handle_events(NULL);

    free(g_wifi_buffer.buffer);
    g_wifi_buffer.buffer = NULL;
}

size_t _zn_read_tcp(int sock, uint8_t *ptr, size_t len)
{
    do
    {
        if (len <= g_wifi_buffer.buffer_lenght)
        {
            memcpy(ptr, g_wifi_buffer.buffer, len);
            g_wifi_buffer.buffer_lenght -= len;
            g_wifi_buffer.buffer_available += len;

            memmove(g_wifi_buffer.buffer, g_wifi_buffer.buffer + len, g_wifi_buffer.buffer_lenght);
            g_wifi_buffer.buffer_current = g_wifi_buffer.buffer;

            return len;
        }

        if (recv(sock, g_wifi_buffer.buffer_current, g_wifi_buffer.buffer_available, 0) < 0)
            return -1;
        m2m_wifi_handle_events(NULL);
    } while (1);

    return 0;
}

size_t _zn_read_exact_tcp(int sock, uint8_t *ptr, size_t len)
{
    size_t n = len;
    size_t rb = 0;

    do
    {
        rb = _zn_read_tcp(sock, ptr, n);
        if (rb < 0)
            return rb;

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > 0);

    return len;
}

size_t _zn_send_tcp(int sock, const uint8_t *ptr, size_t len)
{
    m2m_wifi_handle_events(NULL);

    if (send(sock, (void*)ptr, len, 0) < 0)
        return -1;

    m2m_wifi_handle_events(NULL);

    return len;
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

    g_wifi_buffer.state = __ZN_SOCKET_BRIDGE_UNINITIALIZED;
    z_clock_t start = z_clock_now();
    while (g_wifi_buffer.state != 1 && z_clock_elapsed_ms(&start) < 2000)
        m2m_wifi_handle_events(NULL);
    if(g_wifi_buffer.state != __ZN_SOCKET_BRIDGE_BIND_OR_CONNECTED)
        goto _ZN_OPEN_UDP_UNICAST_ERROR_2;

    // This is required after binding (just for UDP) due to WiFi101 library
    recvfrom(sock, NULL, 0, 0);

    g_wifi_buffer.buffer_lenght = 0;
    g_wifi_buffer.buffer_available = ZN_BATCH_SIZE;
    g_wifi_buffer.buffer = (char *)malloc(g_wifi_buffer.buffer_available);
    g_wifi_buffer.buffer_current = g_wifi_buffer.buffer;
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
    m2m_wifi_handle_events(NULL);

    free(g_wifi_buffer.buffer);
    g_wifi_buffer.buffer = NULL;
}

size_t _zn_read_udp_unicast(int sock, uint8_t *ptr, size_t len)
{
    do
    {
        if (g_wifi_buffer.buffer_lenght > 0)
        {
            memcpy(ptr, g_wifi_buffer.buffer, len);
            g_wifi_buffer.buffer_lenght = 0;
            g_wifi_buffer.buffer_available = ZN_BATCH_SIZE;
            g_wifi_buffer.buffer_current = g_wifi_buffer.buffer;

            return len;
        }

        if (recvfrom(sock, g_wifi_buffer.buffer_current, g_wifi_buffer.buffer_available, 0) < 0)
            return -1;
        m2m_wifi_handle_events(NULL);
    } while (1);

    return 0;
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
    struct sockaddr_in *raddr = (struct sockaddr_in *)arg_1;
    struct sockaddr_in *laddr = NULL;
    unsigned int addrlen = 0;

    registerSocketCallback(_zn_socket_event_handler, NULL);

    int sock = socket(raddr->sin_family, SOCK_DGRAM, 0);
    if (sock < 0)
        goto _ZN_OPEN_UDP_MULTICAST_ERROR_1;

    struct sockaddr_in *addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    memset(addr, 0, sizeof(addr));
    addr->sin_family = AF_INET;
    if (bind(sock, (struct sockaddr*)addr, sizeof(struct sockaddr_in)) < 0)
        goto _ZN_OPEN_UDP_MULTICAST_ERROR_2;

    g_wifi_buffer.state = __ZN_SOCKET_BRIDGE_UNINITIALIZED;
    z_clock_t start = z_clock_now();
    while (g_wifi_buffer.state != 1 && z_clock_elapsed_ms(&start) < 2000)
        m2m_wifi_handle_events(NULL);
    if(g_wifi_buffer.state != __ZN_SOCKET_BRIDGE_BIND_OR_CONNECTED)
        goto _ZN_OPEN_UDP_MULTICAST_ERROR_2;

    // Create laddr endpoint
    *arg_2 = addr;

    // This is required after binding (just for UDP) due to WiFi101 library
    recvfrom(sock, NULL, 0, 0);

    g_wifi_buffer.buffer_lenght = 0;
    g_wifi_buffer.buffer_available = ZN_BATCH_SIZE;
    g_wifi_buffer.buffer = (char *)malloc(g_wifi_buffer.buffer_available);
    g_wifi_buffer.buffer_current = g_wifi_buffer.buffer;

    return sock;

_ZN_OPEN_UDP_MULTICAST_ERROR_2:
    close(sock);

_ZN_OPEN_UDP_MULTICAST_ERROR_1:
    return -1;
}

int _zn_listen_udp_multicast(void *arg, const clock_t tout, const z_str_t iface)
{
    // @TODO: To be implemented

    return -1;
}

void _zn_close_udp_multicast(int sock_recv, int sock_send, void *arg)
{
    m2m_wifi_handle_events(NULL);
    close(sock_recv);
    m2m_wifi_handle_events(NULL);
    close(sock_send);
    m2m_wifi_handle_events(NULL);

    free(g_wifi_buffer.buffer);
    g_wifi_buffer.buffer = NULL;
}

size_t _zn_read_udp_multicast(int sock, uint8_t *ptr, size_t len, void *arg, z_bytes_t *addr)
{
    do
    {
        if (g_wifi_buffer.buffer_lenght > 0)
        {
            memcpy(ptr, g_wifi_buffer.buffer, len);
            g_wifi_buffer.buffer_lenght = 0;
            g_wifi_buffer.buffer_available = ZN_BATCH_SIZE;
            g_wifi_buffer.buffer_current = g_wifi_buffer.buffer;

            return len;
        }

        if (recvfrom(sock, g_wifi_buffer.buffer_current, g_wifi_buffer.buffer_available, 0) < 0)
            return -1;
        m2m_wifi_handle_events(NULL);
    } while (1);

    return 0;
}

size_t _zn_read_exact_udp_multicast(int sock, uint8_t *ptr, size_t len, void *arg, z_bytes_t *addr)
{
    size_t n = len;
    size_t rb = 0;

    do
    {
        rb = _zn_read_udp_multicast(sock, ptr, n, arg, addr);
        if (rb < 0)
            return rb;

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > 0);

    return len;
}

size_t _zn_send_udp_multicast(int sock, const uint8_t *ptr, size_t len, void *arg)
{
    m2m_wifi_handle_events(NULL);

    struct sockaddr_in *raddr = (struct sockaddr_in *)arg;
    if (sendto(sock, (void*)ptr, len, 0, (struct sockaddr*)raddr, sizeof(struct sockaddr_in)) < 0)
        return -1;

    m2m_wifi_handle_events(NULL);

    return len;
}
