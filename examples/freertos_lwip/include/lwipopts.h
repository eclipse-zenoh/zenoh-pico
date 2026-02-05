//
// Copyright (c) 2023 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   hyojun.an <anhj2473@ivis.ai>

#ifndef LWIPOPTS_H
#define LWIPOPTS_H

/* Platform specific locking */
#define NO_SYS 0
#define LWIP_TIMERS 1
#define LWIP_TIMEVAL_PRIVATE 0

/* Memory options */
#define MEM_ALIGNMENT 4
#define MEM_SIZE (16 * 1024)
#define MEMP_NUM_PBUF 16
#define MEMP_NUM_UDP_PCB 6
#define MEMP_NUM_TCP_PCB 10
#define MEMP_NUM_TCP_PCB_LISTEN 8
#define MEMP_NUM_TCP_SEG 16
#define MEMP_NUM_SYS_TIMEOUT 10
#define MEMP_NUM_NETBUF 8
#define MEMP_NUM_NETCONN 8
#define MEMP_NUM_TCPIP_MSG_INPKT 16

/* Pbuf options */
#define PBUF_POOL_SIZE 16
#define PBUF_POOL_BUFSIZE 512

/* TCP options */
#define LWIP_TCP 1
#define TCP_MSS 1460
#define TCP_WND (4 * TCP_MSS)
#define TCP_SND_BUF (4 * TCP_MSS)
#define TCP_SND_QUEUELEN ((4 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS))
#define LWIP_TCP_KEEPALIVE 1

/* UDP options */
#define LWIP_UDP 1
#define UDP_TTL 255

/* DHCP options */
#define LWIP_DHCP 1

/* DNS options */
#define LWIP_DNS 1
#define DNS_TABLE_SIZE 4
#define DNS_MAX_NAME_LENGTH 256

/* ARP options */
#define LWIP_ARP 1
#define ARP_TABLE_SIZE 10
#define ARP_QUEUEING 1

/* Netif options */
#define LWIP_NETIF_STATUS_CALLBACK 1

/* IP options */
#define IP_FORWARD 0
#define IP_OPTIONS_ALLOWED 1
#define IP_REASSEMBLY 1
#define IP_FRAG 1
#define IP_REASS_MAXAGE 3
#define IP_REASS_MAX_PBUFS 10
#define IP_FRAG_USES_STATIC_BUF 0
#define IP_DEFAULT_TTL 255

/* ICMP options */
#define LWIP_ICMP 1

/* RAW options */
#define LWIP_RAW 0

/* SNMP options */
#define LWIP_SNMP 0

/* Statistics options */
#define LWIP_STATS 0
#define LWIP_STATS_DISPLAY 0

/* Checksum options */
#define CHECKSUM_GEN_IP 1
#define CHECKSUM_GEN_UDP 1
#define CHECKSUM_GEN_TCP 1
#define CHECKSUM_CHECK_IP 1
#define CHECKSUM_CHECK_UDP 1
#define CHECKSUM_CHECK_TCP 1

/* Debugging options */
#define LWIP_DEBUG 0
#define LWIP_DBG_MIN_LEVEL LWIP_DBG_LEVEL_ALL
#define LWIP_DBG_TYPES_ON LWIP_DBG_ON

/* Threading options */
#define TCPIP_THREAD_NAME "tcpip_thread"
#define TCPIP_THREAD_STACKSIZE 1024
#define TCPIP_THREAD_PRIO 8
#define TCPIP_MBOX_SIZE 16
#define DEFAULT_UDP_RECVMBOX_SIZE 16
#define DEFAULT_TCP_RECVMBOX_SIZE 16
#define DEFAULT_ACCEPTMBOX_SIZE 16
#define DEFAULT_THREAD_STACKSIZE 1024
#define DEFAULT_THREAD_PRIO 1

/* Sequential API options */
#define LWIP_NETCONN 1
#define LWIP_SOCKET 1
#define LWIP_COMPAT_SOCKETS 1
#define LWIP_POSIX_SOCKETS_IO_NAMES 1
#define LWIP_SO_RCVTIMEO 1
#define LWIP_SO_SNDTIMEO 1
#define LWIP_SO_RCVBUF 1
#define SO_REUSE 1

/* OS */
#define LWIP_TCPIP_CORE_LOCKING 0

#endif /* LWIPOPTS_H */
