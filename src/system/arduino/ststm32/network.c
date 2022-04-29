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

#if Z_LINK_TCP == 1 || Z_LINK_UDP_UNICAST == 1

typedef struct
{
    int sock;
    enum
    {
        __Z_SOCKET_BRIDGE_UNINITIALIZED = 0,
        __Z_SOCKET_BRIDGE_BIND_OR_CONNECTED = 1,
        __Z_SOCKET_BRIDGE_ERROR = 255,
    } state;

    char *_buffer;
    char *_buffer_current;
    uint16_t _buffer_lenght;
    uint16_t _buffer_available;
} __z_wifi_socket_t;

int _z_wifi_socket_eq(const __z_wifi_socket_t *one, const __z_wifi_socket_t *two)
{
    return one->sock == two->sock;
}

void _z_wifi_socket_clear(__z_wifi_socket_t *res)
{
    free(res->buffer);
    res->buffer = NULL;
    res->buffer_current = NULL;
}

_Z_ELEM_DEFINE(_z_wifi_socket, __z_wifi_socket_t, _z_noop_size, _z_wifi_socket_clear, _z_noop_copy)
_Z_LIST_DEFINE(_z_wifi_socket, __z_wifi_socket_t)

_z_wifi_socket_list_t *g_wifi_buffers = NULL;

int _z_inet_pton(const _z_str_t address, struct sockaddr_in *addr)
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

void _z_socket_event_handler(SOCKET sock, uint8_t ev_type, void *ev_msg)
{
    __z_wifi_socket_t lookup = {.sock = sock};
    _z_wifi_socket_list_t *xs = _z_wifi_socket_list_find(g_wifi_buffers, _z_wifi_socket_eq, &lookup);
    if (xs == NULL)
        goto ERR_OR_SUCCESS;

    __z_wifi_socket_t *ws = _z_wifi_socket_list_head(xs);
    switch (ev_type)
    {
        case SOCKET_MSG_BIND:
        {
            tstrSocketBindMsg *ev_bind_msg = (tstrSocketBindMsg *)ev_msg;
            if (ev_bind_msg && ev_bind_msg->status == -1)
                ws->state = __Z_SOCKET_BRIDGE_BIND_OR_CONNECTED;
            else
                ws->state = __Z_SOCKET_BRIDGE_ERROR;

        } break;

        case SOCKET_MSG_CONNECT:
        {
            tstrSocketConnectMsg *ev_conn_msg = (tstrSocketConnectMsg *)ev_msg;
            if (ev_conn_msg && ev_conn_msg->s8Error >= 0)
                ws->state = __Z_SOCKET_BRIDGE_BIND_OR_CONNECTED;
            else
                ws->state = __Z_SOCKET_BRIDGE_ERROR;

        } break;

        case SOCKET_MSG_RECV:
        case SOCKET_MSG_RECVFROM:
        {
            tstrSocketRecvMsg *ev_recv_msg = (tstrSocketRecvMsg *)ev_msg;
//            uint32 strRemoteHostAddr = ev_recv_msg->strRemoteAddr.sin_addr.s_addr;
//            uint16 u16port = ev_recv_msg->strRemoteAddr.sin_port;

            if (ev_recv_msg->s16BufferSize <= 0)
                goto ERR_OR_SUCCESS;

            if (ws->buffer_available < ev_recv_msg->s16BufferSize)
                goto ERR_OR_SUCCESS;

            unsigned int to_read = ev_recv_msg->s16BufferSize;
            if (hif_receive(ev_recv_msg->pu8Buffer, (unsigned char *)ws->buffer_current, to_read, 1) != M2M_SUCCESS)
                goto ERR_OR_SUCCESS;

            ws->buffer_current += to_read;
            ws->buffer_lenght += to_read;
            ws->buffer_available -= to_read;

        } break;

        case SOCKET_MSG_SEND:
        case SOCKET_MSG_SENDTO:
        {
            // Do nothing at the moment
        } break;

        default:
            break;
    }

ERR_OR_SUCCESS:
    return;
}

#if Z_LINK_TCP == 1
/*------------------ TCP sockets ------------------*/
void *_z_create_endpoint_tcp(const _z_str_t s_addr, const _z_str_t port)
{
    struct sockaddr_in *addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    memset(addr, 0, sizeof(addr));

    addr->sin_family = AF_INET;
    _z_inet_pton(s_addr, addr);
    addr->sin_port = _htons(atoi(port));

    return addr;
}

void _z_free_endpoint_tcp(void *arg)
{
    struct sockaddr_in *self = (struct sockaddr_in *)arg;

    free(self);
}

int _z_open_tcp(void *arg, const clock_t tout)
{
    struct sockaddr_in *raddr = (struct sockaddr_in *)arg;

    registerSocketCallback(_z_socket_event_handler, NULL);

    int sock = socket(raddr->sin_family, SOCK_STREAM, 0);
    if (sock < 0)
        goto _Z_OPEN_TCP_UNICAST_ERROR_1;

    if (connect(sock, (struct sockaddr*)raddr, sizeof(struct sockaddr_in)) < 0)
        goto _Z_OPEN_TCP_UNICAST_ERROR_2;

    __z_wifi_socket_t *ws = (__z_wifi_socket_t *)malloc(sizeof(__z_wifi_socket_t));
    ws->state = __Z_SOCKET_BRIDGE_UNINITIALIZED;
    ws->sock = sock;
    ws->buffer_lenght = 0;
    ws->buffer_available = Z_BATCH_SIZE;
    ws->buffer = (char *)malloc(ws->buffer_available);
    ws->buffer_current = ws->buffer;
    g_wifi_buffers = _z_wifi_socket_list_push(g_wifi_buffers, ws);

    _z_clock_t start = _z_clock_now();
    while (ws->state != 1 && _z_clock_elapsed_ms(&start) < 2000)
        m2m_wifi_handle_events(NULL);
    if(ws->state != __Z_SOCKET_BRIDGE_BIND_OR_CONNECTED)
        goto _Z_OPEN_TCP_UNICAST_ERROR_3;

    // This is required after binding (just for TCP) due to WiFi101 library
    recv(sock, NULL, 0, 0);

    return sock;

_Z_OPEN_TCP_UNICAST_ERROR_3:
    g_wifi_buffers = _z_wifi_socket_list_drop_filter(g_wifi_buffers, _z_wifi_socket_eq, ws);

_Z_OPEN_TCP_UNICAST_ERROR_2:
    close(sock);

_Z_OPEN_TCP_UNICAST_ERROR_1:
    return -1;
}

int _z_listen_tcp(void *arg)
{
    struct sockaddr_in *laddr = (struct sockaddr_in *)arg;

    // @TODO: To be implemented

    return -1;
}

void _z_close_tcp(int sock)
{
    m2m_wifi_handle_events(NULL);
    close(sock);
    m2m_wifi_handle_events(NULL);

    __z_wifi_socket_t ws = {.sock = sock};
    g_wifi_buffers = _z_wifi_socket_list_drop_filter(g_wifi_buffers, _z_wifi_socket_eq, &ws);
}

size_t _z_read_tcp(int sock, uint8_t *ptr, size_t len)
{
    __z_wifi_socket_t lookup = {.sock = sock};
    _z_wifi_socket_list_t *xs = _z_wifi_socket_list_find(g_wifi_buffers, _z_wifi_socket_eq, &lookup);
    if (xs == NULL)
        return SIZE_MAX;

    __z_wifi_socket_t *ws = _z_wifi_socket_list_head(xs);
    do
    {
        if (len <= ws->buffer_lenght)
        {
            memcpy(ptr, ws->buffer, len);
            ws->buffer_lenght -= len;
            ws->buffer_available += len;

            memmove(ws->buffer, ws->buffer + len, ws->buffer_lenght);
            ws->buffer_current = ws->buffer;

            return len;
        }

        if (recv(sock, ws->buffer_current, ws->buffer_available, 0) < 0)
            return SIZE_MAX;
        m2m_wifi_handle_events(NULL);
    } while (1);

    return 0;
}

size_t _z_read_exact_tcp(int sock, uint8_t *ptr, size_t len)
{
    size_t n = len;
    size_t rb = 0;

    do
    {
        rb = _z_read_tcp(sock, ptr, n);
        if (rb == SIZE_MAX)
            return rb;

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > 0);

    return len;
}

size_t _z_send_tcp(int sock, const uint8_t *ptr, size_t len)
{
    m2m_wifi_handle_events(NULL);

    if (send(sock, (void*)ptr, len, 0) < 0)
        return SIZE_MAX;

    m2m_wifi_handle_events(NULL);

    return len;
}
#endif

#if Z_LINK_UDP_UNICAST == 1
/*------------------ UDP sockets ------------------*/
void *_z_create_endpoint_udp(const _z_str_t s_addr, const _z_str_t port)
{
    struct sockaddr_in *addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    memset(addr, 0, sizeof(addr));

    addr->sin_family = AF_INET;
    _z_inet_pton(s_addr, addr);
    addr->sin_port = _htons(atoi(port));

    return addr;
}

void _z_free_endpoint_udp(void *arg)
{
    struct sockaddr_in *self = (struct sockaddr_in *)arg;

    free(self);
}

int _z_open_udp_unicast(void *arg, const clock_t tout)
{
    struct sockaddr_in *raddr = (struct sockaddr_in *)arg;

    registerSocketCallback(_z_socket_event_handler, NULL);

    int sock = socket(raddr->sin_family, SOCK_DGRAM, 0);
    if (sock < 0)
        goto _Z_OPEN_UDP_UNICAST_ERROR_1;

    struct sockaddr_in *addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    memset(addr, 0, sizeof(addr));
    addr->sin_family = AF_INET;
    if (bind(sock, (struct sockaddr*)addr, sizeof(struct sockaddr_in)) < 0)
        goto _Z_OPEN_UDP_UNICAST_ERROR_2;

    __z_wifi_socket_t *ws = (__z_wifi_socket_t *)malloc(sizeof(__z_wifi_socket_t));
    ws->state = __Z_SOCKET_BRIDGE_UNINITIALIZED;
    ws->sock = sock;
    ws->buffer_lenght = 0;
    ws->buffer_available = Z_BATCH_SIZE;
    ws->buffer = (char *)malloc(ws->buffer_available);
    ws->buffer_current = ws->buffer;
    g_wifi_buffers = _z_wifi_socket_list_push(g_wifi_buffers, ws);

    _z_clock_t start = _z_clock_now();
    while (ws->state != 1 && _z_clock_elapsed_ms(&start) < 2000)
        m2m_wifi_handle_events(NULL);
    if(ws->state != __Z_SOCKET_BRIDGE_BIND_OR_CONNECTED)
        goto _Z_OPEN_UDP_UNICAST_ERROR_3;

    // This is required after binding (just for UDP) due to WiFi101 library
    recvfrom(sock, NULL, 0, 0);

    return sock;

_Z_OPEN_UDP_UNICAST_ERROR_3:
    g_wifi_buffers = _z_wifi_socket_list_drop_filter(g_wifi_buffers, _z_wifi_socket_eq, ws);

_Z_OPEN_UDP_UNICAST_ERROR_2:
    close(sock);

_Z_OPEN_UDP_UNICAST_ERROR_1:
    return -1;
}

int _z_listen_udp_unicast(void *arg, const clock_t tout)
{
    struct sockaddr_in *laddr = (struct sockaddr_in *)arg;

    // @TODO: To be implemented

    return -1;
}

void _z_close_udp_unicast(int sock)
{
    m2m_wifi_handle_events(NULL);
    close(sock);
    m2m_wifi_handle_events(NULL);

    __z_wifi_socket_t ws = {.sock = sock};
    g_wifi_buffers = _z_wifi_socket_list_drop_filter(g_wifi_buffers, _z_wifi_socket_eq, &ws);
}

size_t _z_read_udp_unicast(int sock, uint8_t *ptr, size_t len)
{
    __z_wifi_socket_t lookup = {.sock = sock};
    _z_wifi_socket_list_t *xs = _z_wifi_socket_list_find(g_wifi_buffers, _z_wifi_socket_eq, &lookup);
    if (xs == NULL)
        return SIZE_MAX;

    __z_wifi_socket_t *ws = _z_wifi_socket_list_head(xs);
    do
    {
        if (ws->buffer_lenght > 0)
        {
            memcpy(ptr, ws->buffer, len);
            ws->buffer_lenght = 0;
            ws->buffer_available = Z_BATCH_SIZE;
            ws->buffer_current = ws->buffer;

            return len;
        }

        if (recvfrom(sock, ws->buffer_current, ws->buffer_available, 0) < 0)
            return SIZE_MAX;
        m2m_wifi_handle_events(NULL);
    } while (1);

    return 0;
}

size_t _z_read_exact_udp_unicast(int sock, uint8_t *ptr, size_t len)
{
    size_t n = len;
    size_t rb = 0;

    do
    {
        rb = _z_read_udp_unicast(sock, ptr, n);
        if (rb == SIZE_MAX)
            return rb;

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > 0);

    return len;
}

size_t _z_send_udp_unicast(int sock, const uint8_t *ptr, size_t len, void *arg)
{
    m2m_wifi_handle_events(NULL);

    struct sockaddr_in *raddr = (struct sockaddr_in *)arg;
    if (sendto(sock, (void*)ptr, len, 0, (struct sockaddr*)raddr, sizeof(struct sockaddr_in)) < 0)
        return SIZE_MAX;

    m2m_wifi_handle_events(NULL);

    return len;
}
#endif

#if Z_LINK_UDP_MULTICAST == 1
int _z_open_udp_multicast(void *arg_1, void **arg_2, const clock_t tout, const _z_str_t iface)
{
    struct sockaddr_in *raddr = (struct sockaddr_in *)arg_1;
    struct sockaddr_in *laddr = NULL;
    unsigned int addrlen = 0;

    registerSocketCallback(_z_socket_event_handler, NULL);

    int sock = socket(raddr->sin_family, SOCK_DGRAM, 0);
    if (sock < 0)
        goto _Z_OPEN_UDP_MULTICAST_ERROR_1;

    struct sockaddr_in *addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    memset(addr, 0, sizeof(addr));
    addr->sin_family = AF_INET;
    if (bind(sock, (struct sockaddr*)addr, sizeof(struct sockaddr_in)) < 0)
        goto _Z_OPEN_UDP_MULTICAST_ERROR_2;

    __z_wifi_socket_t *ws = (__z_wifi_socket_t *)malloc(sizeof(__z_wifi_socket_t));
    ws->state = __Z_SOCKET_BRIDGE_UNINITIALIZED;
    ws->sock = sock;
    ws->buffer_lenght = 0;
    ws->buffer_available = Z_BATCH_SIZE;
    ws->buffer = (char *)malloc(ws->buffer_available);
    ws->buffer_current = ws->buffer;
    g_wifi_buffers = _z_wifi_socket_list_push(g_wifi_buffers, ws);

    _z_clock_t start = _z_clock_now();
    while (ws->state != 1 && _z_clock_elapsed_ms(&start) < 2000)
        m2m_wifi_handle_events(NULL);
    if(ws->state != __Z_SOCKET_BRIDGE_BIND_OR_CONNECTED)
        goto _Z_OPEN_UDP_MULTICAST_ERROR_3;

    // Create laddr endpoint
    *arg_2 = addr;

    // This is required after binding (just for UDP) due to WiFi101 library
    recvfrom(sock, NULL, 0, 0);

    return sock;

_Z_OPEN_UDP_MULTICAST_ERROR_3:
    g_wifi_buffers = _z_wifi_socket_list_drop_filter(g_wifi_buffers, _z_wifi_socket_eq, ws);

_Z_OPEN_UDP_MULTICAST_ERROR_2:
    close(sock);

_Z_OPEN_UDP_MULTICAST_ERROR_1:
    return -1;
}

int _z_listen_udp_multicast(void *arg, const clock_t tout, const _z_str_t iface)
{
    // @TODO: To be implemented

    return -1;
}

void _z_close_udp_multicast(int sock_recv, int sock_send, void *arg)
{
    m2m_wifi_handle_events(NULL);
    close(sock_recv);
    m2m_wifi_handle_events(NULL);
    close(sock_send);
    m2m_wifi_handle_events(NULL);

    __z_wifi_socket_t ws;
    ws.sock = sock_recv;
    g_wifi_buffers = _z_wifi_socket_list_drop_filter(g_wifi_buffers, _z_wifi_socket_eq, &ws);
}

size_t _z_read_udp_multicast(int sock, uint8_t *ptr, size_t len, void *arg, _z_bytes_t *addr)
{
    __z_wifi_socket_t lookup = {.sock = sock};
    _z_wifi_socket_list_t *xs = _z_wifi_socket_list_find(g_wifi_buffers, _z_wifi_socket_eq, &lookup);
    if (xs == NULL)
        return SIZE_MAX;

    __z_wifi_socket_t *ws = _z_wifi_socket_list_head(xs);
    do
    {
        if (ws->buffer_lenght > 0)
        {
            memcpy(ptr, ws->buffer, len);
            ws->buffer_lenght = 0;
            ws->buffer_available = Z_BATCH_SIZE;
            ws->buffer_current = ws->buffer;

            return len;
        }

        if (recvfrom(sock, ws->buffer_current, ws->buffer_available, 0) < 0)
            return SIZE_MAX;
        m2m_wifi_handle_events(NULL);
    } while (1);

    return 0;
}

size_t _z_read_exact_udp_multicast(int sock, uint8_t *ptr, size_t len, void *arg, _z_bytes_t *addr)
{
    size_t n = len;
    size_t rb = 0;

    do
    {
        rb = _z_read_udp_multicast(sock, ptr, n, arg, addr);
        if (rb == SIZE_MAX)
            return rb;

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > 0);

    return len;
}

size_t _z_send_udp_multicast(int sock, const uint8_t *ptr, size_t len, void *arg)
{
    m2m_wifi_handle_events(NULL);

    struct sockaddr_in *raddr = (struct sockaddr_in *)arg;
    if (sendto(sock, (void*)ptr, len, 0, (struct sockaddr*)raddr, sizeof(struct sockaddr_in)) < 0)
        return SIZE_MAX;

    m2m_wifi_handle_events(NULL);

    return len;
}
#endif

#if Z_LINK_BLUETOOTH == 1
    #error "Bluetooth not supported yet on STSTM32 port of Zenoh-Pico"
#endif