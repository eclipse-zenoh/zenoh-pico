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

char * _zn_select_scout_iface() {
  char * eth_prefix = "en";
  char * lo_prefix = "lo";
  size_t len = 2;
  char  *loopback = 0;
  char  *iface = 0;
  struct ifaddrs *ifap;
  struct ifaddrs *current;
  char host[NI_MAXHOST];

  if (getifaddrs(&ifap) == -1) {
    return 0;
  } else {
    current = ifap;
    int family;
    do {
      family = current->ifa_addr->sa_family;

      if (family == AF_INET) {
        if (memcmp(current->ifa_name, eth_prefix, len) == 0) {
          if ((current->ifa_flags & (IFF_MULTICAST | IFF_UP |IFF_RUNNING)) && !(current->ifa_flags & IFF_PROMISC)) {
            getnameinfo(current->ifa_addr,
                  sizeof(struct sockaddr_in),
                  host, NI_MAXHOST,
                  NULL, 0, NI_NUMERICHOST);
            _Z_DEBUG_VA("\t-- Interface: %s\tAddress: <%s>\n", current->ifa_name, host);
            iface = host;
          }
        } else if (memcmp(current->ifa_name, lo_prefix, len) == 0) {
          if ((current->ifa_flags & (IFF_UP |IFF_RUNNING)) && !(current->ifa_flags & IFF_PROMISC)) {
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
  char * result = strdup((iface != 0) ? iface : loopback);
  freeifaddrs(ifap);
  return result;
}

struct sockaddr_in *
_zn_make_socket_address(const char* addr, int port) {
  struct sockaddr_in *saddr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
  bzero(saddr, sizeof(struct sockaddr_in));
  saddr->sin_family = AF_INET;
	saddr->sin_port = htons(port);

  if(inet_pton(AF_INET, addr, &(saddr->sin_addr))<=0) {
    free(saddr);
    return 0;
  }
  return saddr;
}

_zn_socket_result_t
_zn_create_udp_socket(const char *addr, int port, int timeout_usec) {
  _zn_socket_result_t r;
  r.tag = Z_OK_TAG;

  _Z_DEBUG_VA("Binding UDP Socket to: %s:%d\n", addr, port);
  struct sockaddr_in saddr;

  r.value.socket = socket(PF_INET, SOCK_DGRAM, 0);

  if (r.value.socket < 0) {
    r.tag = Z_ERROR_TAG;
    r.value.error = r.value.socket;
    r.value.socket = 0;
    return r;
  }

  bzero(&saddr, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);

	if(inet_pton(AF_INET, addr, &saddr.sin_addr)<=0)
	{
    r.tag = Z_ERROR_TAG;
    r.value.error = ZN_INVALID_ADDRESS_ERROR;
    r.value.socket = 0;
    return r;
	}


	if(bind(r.value.socket, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
    r.tag = Z_ERROR_TAG;
    r.value.error = ZN_INVALID_ADDRESS_ERROR;
    r.value.socket = 0;
    return r;
  }
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = timeout_usec;
  if(setsockopt(r.value.socket,SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout, sizeof(struct timeval))==-1){
    r.tag = Z_ERROR_TAG;
    r.value.error = errno;
    close(r.value.socket);
    r.value.socket = 0;
    return r;
  }
  if(setsockopt(r.value.socket,SOL_SOCKET, SO_SNDTIMEO, (void *)&timeout, sizeof(struct timeval))==-1){
    r.tag = Z_ERROR_TAG;
    r.value.error = errno;
    close(r.value.socket);
    r.value.socket = 0;
    return r;
  }
  return r;
}

_zn_socket_result_t
_zn_open_tx_session(const char *locator) {
  _zn_socket_result_t r;
  r.tag = Z_OK_TAG;
  char * l = strdup(locator);
  _Z_DEBUG_VA("Connecting to: %s:\n", locator);
  char *tx = strtok(l, "/");
  if (strcmp(tx, "tcp") != 0) {
    fprintf(stderr, "Locator provided is not for TCP\n");
    exit(1);
  }
  char *addr_name = strdup(strtok(NULL, ":"));
  char *s_port = strtok(NULL, ":");

  int status;
  char ip_addr[INET6_ADDRSTRLEN];
  struct sockaddr_in *remote;
  struct addrinfo *res;
  status=getaddrinfo(addr_name, s_port, NULL, &res);
  if (status == 0 && res != NULL) {
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

  if (r.value.socket < 0) {
    r.tag = Z_ERROR_TAG;
    r.value.error = r.value.socket;
    r.value.socket = 0;
    return r;
  }

  int flags = 1;
  if(setsockopt(r.value.socket,SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags))==-1){
    r.tag = Z_ERROR_TAG;
    r.value.error = errno;
    close(r.value.socket);
    r.value.socket = 0;
    return r;
  }

#if (ZENOH_MACOS == 1)
  setsockopt(r.value.socket, SOL_SOCKET, SO_NOSIGPIPE, (void *)0, sizeof(int));
#endif


  bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);

	if(inet_pton(AF_INET, ip_addr, &serv_addr.sin_addr)<=0)
	{
    r.tag = Z_ERROR_TAG;
    r.value.error = ZN_INVALID_ADDRESS_ERROR;
    r.value.socket = 0;
    return r;
	}

	if( connect(r.value.socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    r.tag = Z_ERROR_TAG;
    r.value.error = ZN_TX_CONNECTION_ERROR;
    r.value.socket = 0;
    return r;
  }

  return r;

}

size_t _zn_iovs_len(struct iovec* iov, int iovcnt) {
  size_t len = 0;
  for (int i = 0; i < iovcnt; ++i)
    len += iov[i].iov_len;
  return len;
}

int _zn_compute_remaining(struct iovec* iov, int iovcnt, size_t sent) {
  size_t idx = 0;
  int i = 0;
  while (idx + iov[i].iov_len <= sent) {
    idx += sent;
    i += 1;
  }
  int j = 0;
  if (idx + iov[i].iov_len > sent) {
    iov[0].iov_base = ((unsigned char*)iov[i].iov_base) + (sent - idx - iov[i].iov_len );
    j = 1;
    while (i < iovcnt) {
      iov[j] = iov[i];
      j++;
      i++;
    }
  }
  return j;
}

int _zn_send_iovec(_zn_socket_t sock, struct iovec* iov, int iovcnt) {
  int len = 0;
  for (int i = 0; i < iovcnt; ++i)
    len += iov[i].iov_len;

  int n = writev(sock, iov, iovcnt);
  _Z_DEBUG_VA("z_send_iovec sent %d of %d bytes \n", n, len);
  while (n < len) {
    iovcnt = _zn_compute_remaining(iov, iovcnt, n);
    len = _zn_iovs_len(iov, iovcnt);
    n = writev(sock, iov, iovcnt);
    _Z_DEBUG_VA("z_send_iovec sent %d of %d bytes \n", n, len);
    if (n < 0)
      return -1;
  }
  return 0;
}

int
_zn_send_dgram_to(_zn_socket_t sock, const z_iobuf_t* buf, const struct sockaddr *dest, socklen_t salen) {
  int len =  z_iobuf_readable(buf);
  uint8_t *ptr = buf->buf + buf->r_pos;
  int n = len;
  int wb;
  _Z_DEBUG("Sending data on socket....\n");
  wb = sendto(sock, ptr, n, 0, dest, salen);
  _Z_DEBUG_VA("Socket returned: %d\n", wb);
  if (wb <= 0) {
    _Z_DEBUG_VA("Error while sending data over socket [%d]\n", wb);
    return -1;
  }
  return wb;
}

int
_zn_recv_dgram_from(_zn_socket_t sock, z_iobuf_t* buf, struct sockaddr* from, socklen_t *salen) {
  size_t writable = buf->capacity - buf->w_pos;
  uint8_t *cp = buf->buf + buf->w_pos;
  int rb = 0;
  rb = recvfrom(sock, cp, writable, 0, from, salen);
  buf->w_pos = buf->w_pos +  rb;
  return rb;
}



int _zn_send_buf(_zn_socket_t sock, const z_iobuf_t* buf) {
  int len =  z_iobuf_readable(buf);
  uint8_t *ptr = buf->buf + buf->r_pos;
  int n = len;
  int wb;
  do {
    _Z_DEBUG("Sending data on socket....\n");
  #if (ZENOH_LINUX == 1)
    wb = send(sock, ptr, n, MSG_NOSIGNAL);
  #else
    wb = send(sock, ptr, n, 0);
  #endif
    _Z_DEBUG_VA("Socket returned: %d\n", wb);
    if (wb <= 0) {
      _Z_DEBUG_VA("Error while sending data over socket [%d]\n", wb);
      return -1;
    }
    n -= wb;
    ptr = ptr + (len - n);
  } while (n > 0);
  return 0;
}

int _zn_recv_n(_zn_socket_t sock, uint8_t* ptr, size_t len) {
  int n = len;
  int rb;
  do {
    rb = recv(sock, ptr, n, 0);
    if (rb == 0) return -1;
    n -= rb;
    ptr = ptr + (len - n);
  } while (n > 0);
  return 0;
}

int _zn_recv_buf(_zn_socket_t sock, z_iobuf_t *buf) {
  size_t writable = buf->capacity - buf->w_pos;
  uint8_t *cp = buf->buf + buf->w_pos;
  int rb = 0;
  rb = recv(sock, cp, writable, 0);
  buf->w_pos = buf->w_pos +  rb;
  return rb;
}

size_t
_zn_send_msg(_zn_socket_t sock, z_iobuf_t* buf, _zn_message_t* m) {
  _Z_DEBUG(">> send_msg\n");
  z_iobuf_clear(buf);
  _zn_message_encode(buf, m);
  z_iobuf_t l_buf = z_iobuf_make(10);
  z_vle_t len =  z_iobuf_readable(buf);
  z_vle_encode(&l_buf, len);
  struct iovec iov[2];
  iov[0].iov_len = z_iobuf_readable(&l_buf);
  iov[0].iov_base = l_buf.buf;
  iov[1].iov_len = len;
  iov[1].iov_base = buf->buf;

  int rv = _zn_send_iovec(sock, iov, 2);
  z_iobuf_free(&l_buf);
  return rv;
}

size_t
_zn_send_large_msg(_zn_socket_t sock, z_iobuf_t* buf, _zn_message_t* m, unsigned int max_len) {
  if(max_len > buf->capacity) {
    z_iobuf_t bigbuf = z_iobuf_make(max_len);
    int rv = _zn_send_msg(sock, &bigbuf, m);
    z_iobuf_free(&bigbuf);
    return rv;
  }
  else {
    return _zn_send_msg(sock, buf, m);
  }
}

z_vle_result_t
_zn_recv_vle(_zn_socket_t sock) {
  z_vle_result_t r;
  uint8_t buf[10];
  int n;
  int i = 0;
  do {
    n = recv(sock, &buf[i], 1, 0);
    _Z_DEBUG_VA(">> recv_vle [%d] : 0x%x\n", i, buf[i]);
  } while ((buf[i++] > 0x7f) && (n != 0) && (i < 10));

  if (n == 0 || i > 10) {
    r.tag = Z_ERROR_TAG;
    r.value.error = Z_VLE_PARSE_ERROR;
    return r;
  }
  z_iobuf_t iobuf;
  iobuf.capacity = 10;
  iobuf.r_pos = 0;
  iobuf.w_pos = i;
  iobuf.buf = buf;
  _Z_DEBUG(">> \tz_vle_decode\n");
  return z_vle_decode(&iobuf);
}

void
_zn_recv_msg_na(_zn_socket_t sock, z_iobuf_t* buf, _zn_message_p_result_t *r) {
  z_iobuf_clear(buf);
  _Z_DEBUG(">> recv_msg\n");
  r->tag = Z_OK_TAG;
  z_vle_result_t r_vle = _zn_recv_vle(sock);
  ASSURE_P_RESULT(r_vle, r, Z_VLE_PARSE_ERROR)
  size_t len = r_vle.value.vle;
  _Z_DEBUG_VA(">> \t msg len = %zu\n", len);
  if (z_iobuf_writable(buf) < len) {
    r->tag = Z_ERROR_TAG;
    r->value.error = ZN_INSUFFICIENT_IOBUF_SIZE;
    return;
  }
  _zn_recv_n(sock, buf->buf, len);
  buf->r_pos = 0;
  buf->w_pos = len;
  _Z_DEBUG(">> \t z_message_decode\n");
  _zn_message_decode_na(buf, r);
}

_zn_message_p_result_t
_zn_recv_msg(_zn_socket_t sock, z_iobuf_t* buf) {
  _zn_message_p_result_t r;
  _zn_message_p_result_init(&r);
  _zn_recv_msg_na(sock, buf, &r);
  return r;
  // z_iobuf_clear(buf);
  // _Z_DEBUG(">> recv_msg\n");
  // z_message_p_result_t r;
  // z_message_p_result_init(&r);
  // r.tag = Z_ERROR_TAG;
  // z_vle_result_t r_vle = z_recv_vle(sock);
  // ASSURE_RESULT(r_vle, r, Z_VLE_PARSE_ERROR)
  // size_t len = r_vle.value.vle;
  // _Z_DEBUG_VA(">> \t msg len = %zu\n", len);
  // if (z_iobuf_writable(buf) < len) {
  //   r.tag = Z_ERROR_TAG;
  //   r.value.error = ZN_INSUFFICIENT_IOBUF_SIZE;
  //   return r;
  // }
  // z_recv_n(sock, buf, len);
  // buf->r_pos = 0;
  // buf->w_pos = len;
  // _Z_DEBUG(">> \t z_message_decode\n");
  // return z_message_decode(buf);
}
