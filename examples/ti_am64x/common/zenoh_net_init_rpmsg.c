/*
 * Copyright (c) 2026 ZettaScale Technology
 * Copyright (c) 2026 T3 Foundation Gemstone Project (www.t3gemstone.org)
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Network initialisation for TI AM64x IPC mode (ZENOH_TI_AM64X_IPC).
 *
 * Replaces zenoh_net_init.c when the build is configured with
 * -DZENOH_TI_AM64X_IPC=ON.  Instead of bringing up the CPSW Ethernet driver,
 * this module adds an RPMsg-backed lwIP netif that exchanges Ethernet frames
 * with the Linux A53 core via the VRING shared-memory IPC channel.
 *
 * On Linux, the R5F firmware must be loaded via remoteproc.  Then:
 *
 *   sudo insmod rpmsg_net.ko          # creates rpmsg0 when R5F announces
 *   sudo ip addr add 192.168.200.2/30 dev rpmsg0
 *   sudo ip link set rpmsg0 up
 *
 * Then run a zenoh router on the rpmsg0 interface:
 *
 *   zenohd -l udp/192.168.200.2:7447
 *
 * And on the R5F side configure zenoh-pico to connect as client:
 *
 *   Z_CONFIG_MODE_KEY    = "client"
 *   Z_CONFIG_CONNECT_KEY = "udp/192.168.200.2:7447"
 *
 * Or, for peer mode with multicast, add a multicast route on Linux:
 *
 *   sudo ip route add 224.0.0.0/4 dev rpmsg0
 *
 * Implements the same zenoh_net_init / zenoh_net_deinit / zenoh_net_ip_str
 * API as zenoh_net_init.c so that example main.c files are unchanged.
 */

#include "zenoh_net_init.h"
#include "rpmsg_lwip_netif.h"

#include <string.h>

/* FreeRTOS */
#include "FreeRTOS.h"
#include "semphr.h"

/* lwIP */
#include "lwip/dhcp.h"
#include "lwip/err.h"
#include "lwip/inet.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#include "lwip/tcpip.h"

/* TI DPL */
#include <kernel/dpl/DebugP.h>

/* ---- Module state -------------------------------------------------------- */

static struct netif s_netif;

static SemaphoreHandle_t s_tcpip_done_sem;
static StaticSemaphore_t s_tcpip_done_sem_buf;

static volatile int s_setup_result;

/* ---- tcpip_init callback ------------------------------------------------- */

/*
 * Runs inside the lwIP tcpip_thread after it has initialised.
 * Adds the RPMsg netif with a static IP address and brings it up.
 */
static void _tcpip_init_done_cb(void *arg) {
    (void)arg;

    ip4_addr_t ipaddr, netmask, gw;
    ip4addr_aton(RPMSG_NETIF_IP,   &ipaddr);
    ip4addr_aton(RPMSG_NETIF_MASK, &netmask);
    ip4addr_aton(RPMSG_NETIF_GW,   &gw);

    /* Add the RPMsg netif.  rpmsg_lwip_netif_init() is called by lwIP as the
     * netif init function.  Use tcpip_input (NOT ethernet_input) so that
     * _recv_task can safely call netif->input() from outside tcpip_thread:
     * tcpip_input posts the pbuf to the tcpip mailbox and returns immediately;
     * tcpip_thread then calls ethernet_input in the correct core-lock context. */
    struct netif *p = netif_add(&s_netif,
                                &ipaddr, &netmask, &gw,
                                NULL,
                                rpmsg_lwip_netif_init,
                                tcpip_input);
    if (p == NULL) {
        DebugP_log("[zenoh_net_rpmsg] netif_add failed\r\n");
        s_setup_result = ZENOH_NET_ERR_ENET;
        xSemaphoreGive(s_tcpip_done_sem);
        return;
    }

    netif_set_default(&s_netif);
    netif_set_up(&s_netif);
    netif_set_link_up(&s_netif);

    DebugP_log("[zenoh_net_rpmsg] RPMsg netif up, IP: %s\r\n",
               ip4addr_ntoa(netif_ip4_addr(&s_netif)));

    s_setup_result = ZENOH_NET_OK;
    xSemaphoreGive(s_tcpip_done_sem);
}

/* ---- Public API ---------------------------------------------------------- */

int zenoh_net_init(void) {
    s_setup_result  = ZENOH_NET_OK;
    s_tcpip_done_sem = xSemaphoreCreateBinaryStatic(&s_tcpip_done_sem_buf);

    /* Start lwIP tcpip_thread */
    tcpip_init(_tcpip_init_done_cb, NULL);

    /* Wait for netif to be added and IP set */
    xSemaphoreTake(s_tcpip_done_sem, portMAX_DELAY);
    if (s_setup_result != ZENOH_NET_OK) {
        return s_setup_result;
    }

    /* Wait for Linux rpmsg_net to connect (first frame received) */
    int rc = rpmsg_lwip_netif_wait_peer(ZENOH_NET_DHCP_TIMEOUT_MS);
    if (rc != RPMSG_NETIF_OK) {
        DebugP_log("[zenoh_net_rpmsg] Linux peer not ready within %u ms\r\n",
                   (unsigned)ZENOH_NET_DHCP_TIMEOUT_MS);
        return ZENOH_NET_ERR_TIMEOUT;
    }

    DebugP_log("[zenoh_net_rpmsg] Network ready (IP: %s)\r\n",
               ip4addr_ntoa(netif_ip4_addr(&s_netif)));
    return ZENOH_NET_OK;
}

void zenoh_net_deinit(void) {
    rpmsg_lwip_netif_deinit(&s_netif);
    sys_lock_tcpip_core();
    netif_remove(&s_netif);
    sys_unlock_tcpip_core();
}

int zenoh_net_ip_str(char *buf, size_t buflen) {
    if (buf == NULL) {
        return -1;
    }
    const char *ip = ip4addr_ntoa(netif_ip4_addr(&s_netif));
    size_t len = strlen(ip);
    if (len >= buflen) {
        return -1;
    }
    memcpy(buf, ip, len + 1U);
    return 0;
}
