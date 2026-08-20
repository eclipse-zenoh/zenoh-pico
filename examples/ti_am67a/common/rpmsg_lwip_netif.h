/*
 * Copyright (c) 2026 ZettaScale Technology
 * Copyright (c) 2026 T3 Foundation Gemstone Project (www.t3gemstone.org)
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * RPMsg-backed lwIP netif for TI AM67A Cortex-R5F (IPC mode).
 *
 * When ZENOH_TI_AM67A_IPC is defined at build time, zenoh-pico examples use
 * this netif instead of the CPSW Ethernet driver.  The netif wraps the TI
 * MCU+ SDK IPC RPMessage API so that Ethernet frames travel over the VRING
 * shared-memory channel between the R5F and the Linux A53 core.
 *
 * On the Linux side, load the rpmsg_net kernel module (see
 * tools/rpmsg_net/rpmsg_net.c).  The module registers service name
 * "rpmsg-enet"; when this firmware announces the same name the kernel creates
 * an rpmsg0 virtual Ethernet interface with a matching MTU.
 *
 * VRING buffer size and MTU
 * ─────────────────────────
 * The Linux kernel's VirtIO RPMsg subsystem uses 512-byte buffers by default:
 *   RPMsg payload = 512 - 16 (header) = 496 bytes
 *   Ethernet frame = 14 (L2 hdr) + IP MTU + 4 (FCS)
 *   → IP MTU = 496 - 14 - 4 = 478 bytes
 *
 * zenoh-pico's built-in fragmentation (Z_FEATURE_FRAGMENTATION=1) handles
 * zenoh messages larger than this MTU transparently.
 *
 * Usage
 * ─────
 *   // After Drivers_open() / Board_driversOpen():
 *   struct netif nf;
 *   ip4_addr_t ip, mask, gw;
 *   ip4addr_aton(RPMSG_NETIF_IP,   &ip);
 *   ip4addr_aton(RPMSG_NETIF_MASK, &mask);
 *   ip4addr_aton(RPMSG_NETIF_GW,   &gw);
 *   netif_add(&nf, &ip, &mask, &gw, NULL, rpmsg_lwip_netif_init, ethernet_input);
 *   netif_set_default(&nf);
 *   netif_set_up(&nf);
 *   // Optionally wait for DHCP or use static IP above.
 */

#pragma once

#include "lwip/err.h"
#include "lwip/netif.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- RPMsg channel parameters -------------------------------------------
 *
 * RPMSG_NETIF_ENDPT   Local RPMessage endpoint ID announced to Linux.
 *                     Linux rpmsg_net.ko binds to service "rpmsg-enet"; the
 *                     endpoint number is chosen by the Linux kernel when the
 *                     service is discovered — we just pick a local endpoint.
 *
 * RPMSG_NETIF_MTU_IP  IP-layer MTU.  Must satisfy:
 *                     RPMSG_NETIF_MTU_IP + 14 (L2) + 4 (FCS) <= 496 (RPMsg payload)
 */
#ifndef RPMSG_NETIF_ENDPT
#define RPMSG_NETIF_ENDPT   14U
#endif

/* RPMsg payload ceiling with default 512-byte Linux VirtIO VRING buffers */
#define RPMSG_PAYLOAD_MAX   496U
#define RPMSG_ETH_HDR       14U   /* Ethernet II header (no VLAN) */
#define RPMSG_ETH_FCS       4U    /* Frame Check Sequence (CRC32) */

#define RPMSG_NETIF_MTU_IP  (RPMSG_PAYLOAD_MAX - RPMSG_ETH_HDR - RPMSG_ETH_FCS)  /* 478 */

/* ---- Static IP defaults for the point-to-point RPMsg link ---------------
 * R5F: 192.168.200.1  — Linux rpmsg0: 192.168.200.2  — /30 subnet
 * Override via compiler -D flags if needed.
 */
#ifndef RPMSG_NETIF_IP
#define RPMSG_NETIF_IP    "192.168.200.1"
#endif

#ifndef RPMSG_NETIF_MASK
#define RPMSG_NETIF_MASK  "255.255.255.252"
#endif

#ifndef RPMSG_NETIF_GW
#define RPMSG_NETIF_GW    "192.168.200.2"
#endif

/* ---- Interface name reported to zenoh-pico multicast join --------------- */
#ifndef RPMSG_NETIF_NAME
#define RPMSG_NETIF_NAME  "rp0"
#endif

/* ---- Result codes -------------------------------------------------------- */
#define RPMSG_NETIF_OK           0
#define RPMSG_NETIF_ERR_INIT   (-1)
#define RPMSG_NETIF_ERR_LINUX  (-2)   /* RPMessage_waitForLinuxReady timed out */

/* ---- Public API ---------------------------------------------------------- */

/*
 * rpmsg_lwip_netif_init
 *
 * lwIP netif init function — pass as the 'init' argument to netif_add().
 * Constructs the RPMessage endpoint, announces "rpmsg-enet" to Linux, and
 * waits (up to RPMSG_LINUX_READY_TIMEOUT_MS) for the kernel to be ready.
 *
 * Must be called from the tcpip_thread context (i.e. from the callback
 * passed to tcpip_init(), or via tcpip_callback()).
 *
 * Returns ERR_OK on success, ERR_IF on failure.
 */
err_t rpmsg_lwip_netif_init(struct netif *netif);

/*
 * rpmsg_lwip_netif_deinit
 *
 * Destroys the RPMessage endpoint and stops the receiver task.
 * Call before zenoh_net_deinit().
 */
void rpmsg_lwip_netif_deinit(struct netif *netif);

/*
 * rpmsg_lwip_netif_wait_peer
 *
 * Blocks until Linux rpmsg_net.ko has connected (first packet received).
 * Returns RPMSG_NETIF_OK or RPMSG_NETIF_ERR_LINUX on timeout.
 *
 * Call AFTER netif_set_up() and before using the network.
 * Timeout in milliseconds; 0 = wait forever.
 */
int rpmsg_lwip_netif_wait_peer(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
