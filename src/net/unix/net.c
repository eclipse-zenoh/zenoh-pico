/*
 * Copyright (c) 2017, 2020 ADLINK Technology Inc.
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
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/uio.h>

#include "zenoh/private/logging.h"
#include "zenoh/net/session.h"
#include "zenoh/net/private/net.h"
#include "zenoh/net/private/msgcodec.h"

/*------------------ Interfaces and sockets ------------------*/
char *_zn_select_scout_iface()
{
    char *eth_prefix = "en";
    char *lo_prefix = "lo";
    size_t len = 2;
    char *loopback = 0;
    char *iface = 0;
    struct ifaddrs *ifap;
    struct ifaddrs *current;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifap) == -1)
    {
        return 0;
    }
    else
    {
        current = ifap;
        int family;
        do
        {
            family = current->ifa_addr->sa_family;

            if (family == AF_INET)
            {
                if (memcmp(current->ifa_name, eth_prefix, len) == 0)
                {
                    if ((current->ifa_flags & (IFF_MULTICAST | IFF_UP | IFF_RUNNING)) && !(current->ifa_flags & IFF_PROMISC))
                    {
                        getnameinfo(current->ifa_addr,
                                    sizeof(struct sockaddr_in),
                                    host, NI_MAXHOST,
                                    NULL, 0, NI_NUMERICHOST);
                        _Z_DEBUG_VA("\t-- Interface: %s\tAddress: <%s>\n", current->ifa_name, host);
                        iface = host;
                    }
                }
                else if (memcmp(current->ifa_name, lo_prefix, len) == 0)
                {
                    if ((current->ifa_flags & (IFF_UP | IFF_RUNNING)) && !(current->ifa_flags & IFF_PROMISC))
                    {
                        getnameinfo(current->ifa_addr,
                                    sizeof(struct sockaddr_in),
                                    host, NI_MAXHOST,
                                    NULL, 0, NI_NUMERICHOST);
                        _Z_DEBUG_VA("\t-- Interface: %s\tAddress: <%s>\n", current->ifa_name, host);
                        loopback = host;
                    }
                }
            }
            current = current->ifa_next;
        } while ((iface == 0) && (current != 0));
    }
    char *result = strdup((iface != 0) ? iface : loopback);
    freeifaddrs(ifap);
    return result;
}

struct sockaddr_in *_zn_make_socket_address(const char *addr, int port)
{
    struct sockaddr_in *saddr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    memset(saddr, 0, sizeof(struct sockaddr_in));
    saddr->sin_family = AF_INET;
    saddr->sin_port = htons(port);

    if (inet_pton(AF_INET, addr, &(saddr->sin_addr)) <= 0)
    {
        free(saddr);
        return 0;
    }
    return saddr;
}

_zn_socket_result_t _zn_create_udp_socket(const char *addr, int port, int timeout_usec)
{
    _zn_socket_result_t r;
    r.tag = Z_OK_TAG;

    _Z_DEBUG_VA("Binding UDP Socket to: %s:%d\n", addr, port);
    struct sockaddr_in saddr;

    r.value.socket = socket(PF_INET, SOCK_DGRAM, 0);

    if (r.value.socket < 0)
    {
        r.tag = Z_ERROR_TAG;
        r.value.error = r.value.socket;
        r.value.socket = 0;
        return r;
    }

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);

    if (inet_pton(AF_INET, addr, &saddr.sin_addr) <= 0)
    {
        r.tag = Z_ERROR_TAG;
        r.value.error = ZN_INVALID_ADDRESS_ERROR;
        r.value.socket = 0;
        return r;
    }

    if (bind(r.value.socket, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
    {
        r.tag = Z_ERROR_TAG;
        r.value.error = ZN_INVALID_ADDRESS_ERROR;
        r.value.socket = 0;
        return r;
    }
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = timeout_usec;
    if (setsockopt(r.value.socket, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout, sizeof(struct timeval)) == -1)
    {
        r.tag = Z_ERROR_TAG;
        r.value.error = errno;
        close(r.value.socket);
        r.value.socket = 0;
        return r;
    }
    if (setsockopt(r.value.socket, SOL_SOCKET, SO_SNDTIMEO, (void *)&timeout, sizeof(struct timeval)) == -1)
    {
        r.tag = Z_ERROR_TAG;
        r.value.error = errno;
        close(r.value.socket);
        r.value.socket = 0;
        return r;
    }
    return r;
}

_zn_socket_result_t _zn_open_tx_session(const char *locator)
{
    _zn_socket_result_t r;
    r.tag = Z_OK_TAG;
    char *l = strdup(locator);
    _Z_DEBUG_VA("Connecting to: %s:\n", locator);
    char *tx = strtok(l, "/");
    if (strcmp(tx, "tcp") != 0)
    {
        fprintf(stderr, "Locator provided is not for TCP\n");
        exit(1);
    }
    char *addr_name = strdup(strtok(NULL, ":"));
    char *s_port = strtok(NULL, ":");

    int status;
    char ip_addr[INET6_ADDRSTRLEN];
    struct sockaddr_in *remote;
    struct addrinfo *res;
    status = getaddrinfo(addr_name, s_port, NULL, &res);
    free(addr_name);
    if (status == 0 && res != NULL)
    {
        void *addr;
        remote = (struct sockaddr_in *)res->ai_addr;
        addr = &(remote->sin_addr);
        inet_ntop(res->ai_family, addr, ip_addr, sizeof(ip_addr));
    }
    freeaddrinfo(res);

    int port;
    sscanf(s_port, "%d", &port);

    _Z_DEBUG_VA("Connecting to: %s:%d\n", addr, port);
    free(l);
    struct sockaddr_in serv_addr;

    r.value.socket = socket(PF_INET, SOCK_STREAM, 0);

    if (r.value.socket < 0)
    {
        r.tag = Z_ERROR_TAG;
        r.value.error = r.value.socket;
        r.value.socket = 0;
        return r;
    }

    int flags = 1;
    if (setsockopt(r.value.socket, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags)) == -1)
    {
        r.tag = Z_ERROR_TAG;
        r.value.error = errno;
        close(r.value.socket);
        r.value.socket = 0;
        return r;
    }

#if (ZENOH_MACOS == 1)
    setsockopt(r.value.socket, SOL_SOCKET, SO_NOSIGPIPE, (void *)0, sizeof(int));
#endif

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip_addr, &serv_addr.sin_addr) <= 0)
    {
        r.tag = Z_ERROR_TAG;
        r.value.error = ZN_INVALID_ADDRESS_ERROR;
        r.value.socket = 0;
        return r;
    }

    if (connect(r.value.socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        r.tag = Z_ERROR_TAG;
        r.value.error = ZN_TX_CONNECTION_ERROR;
        r.value.socket = 0;
        return r;
    }

    return r;
}

/*------------------ Datagram ------------------*/
int _zn_send_dgram_to(_zn_socket_t sock, const z_iobuf_t *buf, const struct sockaddr *dest, socklen_t salen)
{
    int len = z_iobuf_readable(buf);
    uint8_t *ptr = buf->buf + buf->r_pos;
    int n = len;
    int wb;
    _Z_DEBUG("Sending data on socket....\n");
    wb = sendto(sock, ptr, n, 0, dest, salen);
    _Z_DEBUG_VA("Socket returned: %d\n", wb);
    if (wb <= 0)
    {
        _Z_DEBUG_VA("Error while sending data over socket [%d]\n", wb);
        return -1;
    }
    return wb;
}

int _zn_recv_dgram_from(_zn_socket_t sock, z_iobuf_t *buf, struct sockaddr *from, socklen_t *salen)
{
    size_t writable = buf->capacity - buf->w_pos;
    uint8_t *cp = buf->buf + buf->w_pos;
    int rb = 0;
    rb = recvfrom(sock, cp, writable, 0, from, salen);
    buf->w_pos = buf->w_pos + rb;
    return rb;
}

/*------------------ Send ------------------*/
int _zn_send_buf(_zn_socket_t sock, const z_iobuf_t *buf)
{
    int len = z_iobuf_readable(buf);
    uint8_t *ptr = buf->buf + buf->r_pos;
    int n = len;
    int wb;
    do
    {
        _Z_DEBUG("Sending data on socket....\n");
#if (ZENOH_LINUX == 1)
        wb = send(sock, ptr, n, MSG_NOSIGNAL);
#else
        wb = send(sock, ptr, n, 0);
#endif
        _Z_DEBUG_VA("Socket returned: %d\n", wb);
        if (wb <= 0)
        {
            _Z_DEBUG_VA("Error while sending data over socket [%d]\n", wb);
            return -1;
        }
        n -= wb;
        ptr = ptr + (len - n);
    } while (n > 0);
    return 0;
}

void _zn_prepare_buf(z_iobuf_t *buf)
{
    z_iobuf_clear(buf);

#ifdef ZENOH_NET_TRANSPORT_TCP_IP
    // NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
    //       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
    //       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
    //       the boundary of the serialized messages. The length is encoded as little-endian.
    //       In any case, the length of a message must not exceed 65_535 bytes.
    for (unsigned int i = 0; i < _ZN_MSG_LEN_ENC_SIZE; ++i)
        z_iobuf_write(buf, 0);
#endif /* ZENOH_NET_TRANSPORT_TCP_IP */
}

void _zn_finalize_buf(z_iobuf_t *buf)
{
#ifdef ZENOH_NET_TRANSPORT_TCP_IP
    // NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
    //       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
    //       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
    //       the boundary of the serialized messages. The length is encoded as little-endian.
    //       In any case, the length of a message must not exceed 65_535 bytes.
    unsigned int len = z_iobuf_readable(buf) - _ZN_MSG_LEN_ENC_SIZE;
    assert(len < (1 << (8 * _ZN_MSG_LEN_ENC_SIZE)));

    for (unsigned int i = 0; i < _ZN_MSG_LEN_ENC_SIZE; ++i)
        z_iobuf_put(buf, (uint8_t)((len >> 8 * i) & 0xFF), i);
#endif /* ZENOH_NET_TRANSPORT_TCP_IP */
}

int _zn_send_s_msg(_zn_socket_t sock, z_iobuf_t *buf, _zn_session_message_t *s_msg)
{
    _Z_DEBUG(">> send session message\n");

    // Prepare the buffer eventually reserving space for the message length
    _zn_prepare_buf(buf);
    // Encode the session message
    _zn_session_message_encode(buf, s_msg);
    // Write the message legnth in the reserved space if needed
    _zn_finalize_buf(buf);

    return _zn_send_buf(sock, buf);
}

int _zn_send_z_msg(zn_session_t *z, _zn_zenoh_message_t *z_msg, int reliable)
{
    // @TODO: implement fragmentation
    _Z_DEBUG(">> send zenoh message\n");

    // Create the frame session message that carries the zenoh message
    _zn_session_message_t s_msg;
    s_msg.attachment = NULL;
    s_msg.header = _ZN_MID_FRAME;
    if (reliable)
    {
        _ZN_SET_FLAG(s_msg.header, _ZN_FLAG_S_R);
        s_msg.body.frame.sn = z->sn_tx_reliable;
        // Update the sequence number in modulo operation
        z->sn_tx_reliable = (z->sn_tx_reliable + 1) % z->sn_resolution;
    }
    else
    {
        s_msg.body.frame.sn = z->sn_tx_best_effort;
        // Update the sequence number in modulo operation
        z->sn_tx_best_effort = (z->sn_tx_best_effort + 1) % z->sn_resolution;
    }

    // Do not allocate the vector containing the messages
    s_msg.body.frame.payload.messages.capacity_ = 0;
    s_msg.body.frame.payload.messages.length_ = 0;
    s_msg.body.frame.payload.messages.elem_ = NULL;

    // Prepare the buffer eventually reserving space for the message length
    _zn_prepare_buf(&z->wbuf);
    // Encode the frame header
    _zn_session_message_encode(&z->wbuf, &s_msg);
    // Encode the zenoh message
    _zn_zenoh_message_encode(&z->wbuf, z_msg);
    // Write the message legnth in the reserved space if needed
    _zn_finalize_buf(&z->wbuf);

    return _zn_send_buf(z->sock, &z->wbuf);
}

/*------------------ Receive ------------------*/
int _zn_recv_n(_zn_socket_t sock, uint8_t *ptr, size_t len)
{
    int n = len;
    int rb;

    do
    {
        rb = recv(sock, ptr, n, 0);
        if (rb == 0)
            return -1;
        n -= rb;
        ptr = ptr + (len - n);
    } while (n > 0);

    return 0;
}

int _zn_recv_buf(_zn_socket_t sock, z_iobuf_t *buf)
{
    size_t writable = buf->capacity - buf->w_pos;
    uint8_t *cp = buf->buf + buf->w_pos;
    int rb = 0;
    rb = recv(sock, cp, writable, 0);
    buf->w_pos = buf->w_pos + rb;
    return rb;
}

void zn_recv_s_msg_na(_zn_socket_t sock, z_iobuf_t *buf, _zn_session_message_p_result_t *r)
{
    z_iobuf_clear(buf);
    _Z_DEBUG(">> recv session msg\n");
    r->tag = Z_OK_TAG;

#ifdef ZENOH_NET_TRANSPORT_TCP_IP
    // NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
    //       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
    //       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
    //       the boundary of the serialized messages. The length is encoded as little-endian.
    //       In any case, the length of a message must not exceed 65_535 bytes.

    // Read the message length
    if (_zn_recv_n(sock, buf->buf, _ZN_MSG_LEN_ENC_SIZE) < 0)
    {
        r->tag = Z_ERROR_TAG;
        r->value.error = ZN_IO_ERROR;
        return;
    }
    buf->w_pos = _ZN_MSG_LEN_ENC_SIZE;

    uint16_t len = z_iobuf_read(buf) | (z_iobuf_read(buf) << 8);
    _Z_DEBUG_VA(">> \t msg len = %zu\n", len);
    if (z_iobuf_writable(buf) < len)
    {
        r->tag = Z_ERROR_TAG;
        r->value.error = ZN_INSUFFICIENT_IOBUF_SIZE;
        return;
    }

    // Read enough bytes to decode the message
    if (_zn_recv_n(sock, buf->buf, len) < 0)
    {
        r->tag = Z_ERROR_TAG;
        r->value.error = ZN_IO_ERROR;
        return;
    }

    buf->r_pos = 0;
    buf->w_pos = len;
#else
    if (_zn_recv_buf(sock, buf) < 0)
    {
        r->tag = Z_ERROR_TAG;
        r->value.error = ZN_IO_ERROR;
        return;
    }
#endif /* ZENOH_NET_TRANSPORT_TCP_IP */

    _Z_DEBUG(">> \t session_message_decode\n");
    _zn_session_message_decode_na(buf, r);
}

_zn_session_message_p_result_t _zn_recv_s_msg(_zn_socket_t sock, z_iobuf_t *buf)
{
    _zn_session_message_p_result_t r;
    _zn_session_message_p_result_init(&r);
    zn_recv_s_msg_na(sock, buf, &r);
    return r;
}

// size_t _zn_iovs_len(struct iovec *iov, int iovcnt)
// {
//     size_t len = 0;
//     for (int i = 0; i < iovcnt; ++i)
//         len += iov[i].iov_len;
//     return len;
// }

// int _zn_compute_remaining(struct iovec *iov, int iovcnt, size_t sent)
// {
//     size_t idx = 0;
//     int i = 0;
//     while (idx + iov[i].iov_len <= sent)
//     {
//         idx += sent;
//         i += 1;
//     }
//     int j = 0;
//     if (idx + iov[i].iov_len > sent)
//     {
//         iov[0].iov_base = ((unsigned char *)iov[i].iov_base) + (sent - idx - iov[i].iov_len);
//         j = 1;
//         while (i < iovcnt)
//         {
//             iov[j] = iov[i];
//             j++;
//             i++;
//         }
//     }
//     return j;
// }

// int _zn_send_iovec(_zn_socket_t sock, struct iovec *iov, int iovcnt)
// {
//     int len = 0;
//     for (int i = 0; i < iovcnt; ++i)
//         len += iov[i].iov_len;

//     int n = writev(sock, iov, iovcnt);
//     _Z_DEBUG_VA("z_send_iovec sent %d of %d bytes \n", n, len);
//     while (n < len)
//     {
//         iovcnt = _zn_compute_remaining(iov, iovcnt, n);
//         len = _zn_iovs_len(iov, iovcnt);
//         n = writev(sock, iov, iovcnt);
//         _Z_DEBUG_VA("z_send_iovec sent %d of %d bytes \n", n, len);
//         if (n < 0)
//             return -1;
//     }
//     return 0;
// }
