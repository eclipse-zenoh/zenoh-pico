/*
 * Copyright (c) 2026 ZettaScale Technology
 * Copyright (c) 2026 T3 Foundation Gemstone Project (www.t3gemstone.org)
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Network initialisation helper for zenoh-pico examples on TI AM64x.
 *
 * This module wraps the TI MCU+ SDK CPSW Enet + lwIP stack bringup into a
 * single blocking call that returns once the interface has an IP address.
 *
 * Usage in your application task (called AFTER Drivers_open() and
 * Board_driversOpen()):
 *
 *     int rc = zenoh_net_init();
 *     if (rc != ZENOH_NET_OK) { ... error ... }
 *     // network is ready, proceed to z_open() etc.
 *
 * Optional configuration — define BEFORE including this header or via
 * compiler -D flags:
 *
 *   Static IP (skips DHCP):
 *     #define ZENOH_NET_STATIC_IP   "192.168.1.100"
 *     #define ZENOH_NET_STATIC_MASK "255.255.255.0"
 *     #define ZENOH_NET_STATIC_GW   "192.168.1.1"
 *
 *   DHCP wait timeout (default 15 s):
 *     #define ZENOH_NET_DHCP_TIMEOUT_MS  15000U
 *
 *   Netif instance index (default 0):
 *     #define ZENOH_NET_NETIF_IDX  0U
 *
 *   Interface name string reported to zenoh-pico multicast join (default "ti0"):
 *     #define ZENOH_NET_IFACE_NAME  "ti0"
 */

#pragma once

#include <stddef.h>

/* ---- Result codes --------------------------------------------------------- */
#define ZENOH_NET_OK            0
#define ZENOH_NET_ERR_ENET    (-1)
#define ZENOH_NET_ERR_DHCP    (-2)
#define ZENOH_NET_ERR_TIMEOUT (-3)

/* ---- Defaults ------------------------------------------------------------- */
#ifndef ZENOH_NET_DHCP_TIMEOUT_MS
#define ZENOH_NET_DHCP_TIMEOUT_MS  15000U
#endif

#ifndef ZENOH_NET_NETIF_IDX
#define ZENOH_NET_NETIF_IDX  0U
#endif

#ifndef ZENOH_NET_IFACE_NAME
#define ZENOH_NET_IFACE_NAME  "ti0"
#endif

/* ---- API ------------------------------------------------------------------ */

/*
 * zenoh_net_init — bring up CPSW Enet + lwIP, obtain IP (DHCP or static).
 *
 * Must be called from a FreeRTOS task context AFTER Drivers_open() and
 * Board_driversOpen() (which open the underlying CPSW enet driver).
 * Blocks until IP is assigned or timeout expires.
 *
 * Returns ZENOH_NET_OK (0) on success, negative error code on failure.
 */
int zenoh_net_init(void);

/*
 * zenoh_net_deinit — close netif and release lwIP resources.
 *
 * Call before closing the zenoh session if you need a clean shutdown.
 */
void zenoh_net_deinit(void);

/*
 * zenoh_net_ip_str — copy the current IPv4 address into buf as a C string.
 *
 * Returns 0 on success, -1 if network is not up or buf is too small.
 */
int zenoh_net_ip_str(char *buf, size_t buflen);
