/*
 * Copyright (c) 2026 ZettaScale Technology
 * Copyright (c) 2026 T3 Foundation Gemstone Project (www.t3gemstone.org)
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * RPMsg-backed lwIP netif — R5F side (IPC mode).
 *
 * Architecture
 * ────────────
 * lwIP sees a standard Ethernet netif.  The linkoutput hook copies the pbuf
 * into a flat buffer and calls RPMessage_send() to the Linux A53 core.
 * A dedicated FreeRTOS receiver task calls RPMessage_recv() in a loop; each
 * received Ethernet frame is injected back into lwIP via netif->input().
 *
 * The service name "rpmsg-enet" matches the rpmsg_net Linux kernel module
 * (tools/rpmsg_net/rpmsg_net.c).  When Linux loads that module it sends a
 * small dummy frame to learn our return address — we store that remote
 * endpoint on the first received message and use it for all subsequent sends.
 *
 * MTU
 * ───
 * With the default 512-byte VirtIO VRING buffers:
 *   RPMsg payload max = 496 bytes
 *   Ethernet frame    ≤ 496 bytes → IP MTU = 496 - 14 (L2 hdr) - 4 (FCS) = 478 bytes
 * zenoh-pico's Z_FEATURE_FRAGMENTATION=1 handles messages larger than this.
 */

#include "rpmsg_lwip_netif.h"

#include <string.h>

/* FreeRTOS */
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

/* TI DPL */
#include <kernel/dpl/DebugP.h>
#include <kernel/dpl/ClockP.h>
#include <drivers/hw_include/j722s/cslr_soc_defines.h>

/* IPC RPMessage */
#include <drivers/ipc_rpmsg.h>
#include <drivers/ipc_notify.h>

/* lwIP */
#include "lwip/err.h"
#include "lwip/etharp.h"
#include "lwip/mem.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "netif/ethernet.h"

/* ---- Module-private constants -------------------------------------------- */

#define RPMSG_LINUX_READY_TIMEOUT_MS  10000U
#define RPMSG_PEER_READY_TIMEOUT_MS   15000U

/* Stack and priority for the RPMessage receiver task */
#define RECV_TASK_STACK_DEPTH   2048U
#define RECV_TASK_PRIORITY      (configMAX_PRIORITIES - 2)

/* Sentinel: remote endpoint not yet known */
#define REMOTE_ENDPT_UNKNOWN    0xFFFFU

/* ---- Module-private state ------------------------------------------------- */

/* RPMessage objects must be global/static (never on stack) */
static RPMessage_Object s_rpmsg_obj;

/* Remote A53 endpoint — discovered from the first received message */
static volatile uint16_t s_remote_endpt = REMOTE_ENDPT_UNKNOWN;

/* Pointer back to the netif (set during init, used by recv task) */
static struct netif *s_netif = NULL;

/* Semaphore: given when the first frame from Linux arrives (peer connected) */
static SemaphoreHandle_t s_peer_sem;
static StaticSemaphore_t s_peer_sem_buf;

/* Receiver task handle */
static TaskHandle_t s_recv_task_handle = NULL;
static StaticTask_t s_recv_task_tcb;
static StackType_t  s_recv_task_stack[RECV_TASK_STACK_DEPTH];

/* Staging buffer for one RPMessage payload */
static uint8_t s_recv_buf[RPMSG_PAYLOAD_MAX];

/* ---- Forward declarations ------------------------------------------------- */
static void _recv_task(void *arg);
static err_t _netif_linkoutput(struct netif *netif, struct pbuf *p);

/* ---- RPMessage receiver task --------------------------------------------- */

/*
 * Runs forever; calls RPMessage_recv() and injects frames into lwIP.
 * The first received message teaches us the remote endpoint address.
 */
static void _recv_task(void *arg) {
    (void)arg;

    for (;;) {
        uint16_t rx_len   = (uint16_t)sizeof(s_recv_buf);
        uint16_t core_id  = 0U;
        uint32_t endpt    = 0U;

        int32_t status = RPMessage_recv(&s_rpmsg_obj,
                                        s_recv_buf, &rx_len,
                                        &core_id, &endpt,
                                        SystemP_WAIT_FOREVER);
        if (status != SystemP_SUCCESS) {
            continue;
        }

        /* Learn the remote endpoint on the very first frame */
        if (s_remote_endpt == REMOTE_ENDPT_UNKNOWN) {
            s_remote_endpt = (uint16_t)endpt;
            DebugP_log("[rpmsg_netif] Linux peer connected (core %u, ep %u)\r\n",
                       (unsigned)core_id, (unsigned)endpt);
            xSemaphoreGive(s_peer_sem);
        }

        /* Discard zero-length and oversized frames */
        if ((rx_len == 0U) || (rx_len > (uint16_t)RPMSG_PAYLOAD_MAX)) {
            continue;
        }

        /* Allocate from heap (PBUF_POOL_SIZE=0 in SDK lwipopts, custom mem pools used) */
        struct pbuf *p = pbuf_alloc(PBUF_RAW, rx_len, PBUF_RAM);
        if (p == NULL) {
            DebugP_log("[rpmsg_netif] pbuf_alloc failed (dropped %u bytes)\r\n",
                       (unsigned)rx_len);
            continue;
        }
        pbuf_take(p, s_recv_buf, rx_len);

        /* Hand the frame to lwIP via tcpip_input() (set as netif->input in
         * netif_add).  tcpip_input posts the pbuf to the tcpip mailbox so this
         * is safe to call from any task without holding the core lock. */
        DebugP_log("[rpmsg_netif] recv: injecting %u-byte frame into lwIP\r\n", (unsigned)rx_len);
        err_t inp_err = s_netif->input(p, s_netif);
        DebugP_log("[rpmsg_netif] recv: tcpip_input ret=%d\r\n", (int)inp_err);
        if (inp_err != ERR_OK) {
            pbuf_free(p);
        }
    }
}

/* ---- lwIP netif linkoutput ----------------------------------------------- */

/*
 * Called by lwIP when it wants to send an Ethernet frame.
 * Copies the (possibly chained) pbuf into a flat buffer and sends via RPMessage.
 */
static err_t _netif_linkoutput(struct netif *netif, struct pbuf *p) {
    (void)netif;

    if (s_remote_endpt == REMOTE_ENDPT_UNKNOWN) {
        /* Linux not yet connected — drop silently */
        return ERR_OK;
    }

    uint16_t total_len = (uint16_t)pbuf_clen(p);
    (void)total_len;  /* used only for bounds check via pbuf_copy_partial */

    /* Flatten the pbuf chain into the tx staging buffer on the stack.
     * RPMSG_PAYLOAD_MAX = 496; Ethernet frames must fit (MTU=478 enforced by
     * the netif mtu field, so IP never generates frames > 478+14+4 = 496). */
    uint8_t tx_buf[RPMSG_PAYLOAD_MAX];
    uint16_t len = (uint16_t)pbuf_copy_partial(p, tx_buf, sizeof(tx_buf), 0);
    if (len == 0U) {
        return ERR_OK;
    }

    /* Use NO_WAIT: holding tcpip core lock while blocking here deadlocks the
     * entire lwIP stack if the TX vring is temporarily full. */
    int32_t status = RPMessage_send(
        tx_buf, len,
        CSL_CORE_ID_A53SS0_0,          /* destination: Linux A53 core */
        (uint16_t)s_remote_endpt,       /* destination endpoint */
        RPMSG_NETIF_ENDPT,             /* source (local) endpoint */
        SystemP_NO_WAIT
    );

    if (status != SystemP_SUCCESS) {
        DebugP_log("[rpmsg_netif] RPMessage_send failed (%d)\r\n", (int)status);
        return ERR_IF;
    }

    return ERR_OK;
}

/* ---- lwIP netif init function -------------------------------------------- */

err_t rpmsg_lwip_netif_init(struct netif *netif) {
    s_netif = netif;

    /* ---- Semaphore: peer-connected signal ---- */
    s_peer_sem = xSemaphoreCreateBinaryStatic(&s_peer_sem_buf);

    /* ---- Wait for Linux remoteproc to boot ---- */
    DebugP_log("[rpmsg_netif] Waiting for Linux IPC ready...\r\n");
    int32_t status = RPMessage_waitForLinuxReady(
        pdMS_TO_TICKS(RPMSG_LINUX_READY_TIMEOUT_MS));
    if (status != SystemP_SUCCESS) {
        /* Timeout is non-fatal: Linux may have already sent the sync before
         * this task started.  The VirtIO VRING is up regardless; proceed. */
        DebugP_log("[rpmsg_netif] IPC ready timeout (non-fatal), proceeding\r\n");
    } else {
        DebugP_log("[rpmsg_netif] Linux IPC ready\r\n");
    }

    /* ---- Create local RPMessage endpoint ---- */
    RPMessage_CreateParams cp;
    RPMessage_CreateParams_init(&cp);
    cp.localEndPt = RPMSG_NETIF_ENDPT;

    status = RPMessage_construct(&s_rpmsg_obj, &cp);
    if (status != SystemP_SUCCESS) {
        DebugP_log("[rpmsg_netif] RPMessage_construct failed (%d)\r\n", (int)status);
        return ERR_IF;
    }

    /* ---- Announce service to Linux ("rpmsg-enet" matches rpmsg_net.ko) ---- */
    status = RPMessage_announce(CSL_CORE_ID_A53SS0_0,
                                RPMSG_NETIF_ENDPT,
                                "rpmsg-enet");
    if (status != SystemP_SUCCESS) {
        DebugP_log("[rpmsg_netif] RPMessage_announce failed (%d)\r\n", (int)status);
        RPMessage_destruct(&s_rpmsg_obj);
        return ERR_IF;
    }

    /* ---- Spawn receiver task ---- */
    s_recv_task_handle = xTaskCreateStatic(
        _recv_task,
        "rpmsg_rx",
        RECV_TASK_STACK_DEPTH,
        NULL,
        RECV_TASK_PRIORITY,
        s_recv_task_stack,
        &s_recv_task_tcb
    );
    if (s_recv_task_handle == NULL) {
        DebugP_log("[rpmsg_netif] xTaskCreateStatic (recv) failed\r\n");
        RPMessage_destruct(&s_rpmsg_obj);
        return ERR_IF;
    }

    /* ---- Set lwIP netif fields ---- */
    /* Interface name: 'r','p' — lwIP appends a digit, giving "rp0" */
    netif->name[0] = 'r';
    netif->name[1] = 'p';

    /* Fake MAC address: locally administered 02:52:50:00:00:01 */
    netif->hwaddr_len = ETH_HWADDR_LEN;
    netif->hwaddr[0] = 0x02U;
    netif->hwaddr[1] = 0x52U;
    netif->hwaddr[2] = 0x50U;
    netif->hwaddr[3] = 0x00U;
    netif->hwaddr[4] = 0x00U;
    netif->hwaddr[5] = 0x01U;

    netif->mtu        = (uint16_t)RPMSG_NETIF_MTU_IP;
    netif->flags      = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP |
                        NETIF_FLAG_ETHERNET   | NETIF_FLAG_IGMP;

    netif->output     = etharp_output;
    netif->linkoutput = _netif_linkoutput;

    DebugP_log("[rpmsg_netif] init OK (ep=%u, MTU=%u)\r\n",
               RPMSG_NETIF_ENDPT, RPMSG_NETIF_MTU_IP);

    return ERR_OK;
}

/* ---- Public helpers ------------------------------------------------------ */

void rpmsg_lwip_netif_deinit(struct netif *netif) {
    (void)netif;

    if (s_recv_task_handle != NULL) {
        vTaskDelete(s_recv_task_handle);
        s_recv_task_handle = NULL;
    }

    RPMessage_unblock(&s_rpmsg_obj);
    RPMessage_destruct(&s_rpmsg_obj);
    s_netif        = NULL;
    s_remote_endpt = REMOTE_ENDPT_UNKNOWN;
}

int rpmsg_lwip_netif_wait_peer(uint32_t timeout_ms) {
    TickType_t ticks = (timeout_ms == 0U) ? portMAX_DELAY
                                          : pdMS_TO_TICKS(timeout_ms);
    if (xSemaphoreTake(s_peer_sem, ticks) != pdTRUE) {
        DebugP_log("[rpmsg_netif] Peer (Linux rpmsg_net) not connected after %u ms\r\n",
                   (unsigned)timeout_ms);
        return RPMSG_NETIF_ERR_LINUX;
    }
    return RPMSG_NETIF_OK;
}
