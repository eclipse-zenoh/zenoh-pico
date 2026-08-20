/*
 * Copyright (c) 2026 ZettaScale Technology
 * Copyright (c) 2026 T3 Foundation Gemstone Project (www.t3gemstone.org)
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * z_pubsub — zenoh-pico combined publisher + subscriber example for TI AM64x.
 *
 * A single zenoh session is shared between two FreeRTOS tasks:
 *   - pub_task  : publishes on "t3/pubsub/tx" every second
 *   - sub_task  : subscribes to "t3/pubsub/rx", prints received samples
 *
 * Uses peer mode over UDP multicast (224.0.0.224:7446).
 * Both tasks wait on a shared semaphore before accessing the session.
 *
 * Build prerequisites: see z_pub/main.c header.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* TI MCU+ SDK — SysConfig-generated (must exist in syscfg/) */
#include "ti_drivers_config.h"
#include "ti_drivers_open_close.h"
#include "ti_board_config.h"
#include "ti_board_open_close.h"

/* TI DPL */
#include <kernel/dpl/DebugP.h>

/* FreeRTOS */
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

/* zenoh-pico */
#include <zenoh-pico.h>

/* zenoh-pico pool allocator stats */
#include "zenoh-pico/system/platform/ti_am64x/mem_pool.h"

/* Example network init */
#include "zenoh_net_init.h"

/* ---- Configuration -------------------------------------------------------- */
#define PUB_KEYEXPR       "t3/pubsub/tx"
#define SUB_KEYEXPR       "t3/pubsub/rx"
#define MULTICAST_EP      "udp/224.0.0.224:7446#iface=rp0"
#define PUB_PERIOD_MS     1000U
#define PUB_POOL_STAT_N   10U

/* ---- FreeRTOS task parameters --------------------------------------------- */
#define PUB_TASK_STACK  (8192U / sizeof(configSTACK_DEPTH_TYPE))
#define SUB_TASK_STACK  (8192U / sizeof(configSTACK_DEPTH_TYPE))
#define MAIN_TASK_STACK (16384U / sizeof(configSTACK_DEPTH_TYPE))

static StackType_t  gPubTaskStack[PUB_TASK_STACK]   __attribute__((aligned(32)));
static StaticTask_t gPubTaskObj;
static StackType_t  gSubTaskStack[SUB_TASK_STACK]   __attribute__((aligned(32)));
static StaticTask_t gSubTaskObj;
static StackType_t  gMainTaskStack[MAIN_TASK_STACK] __attribute__((aligned(32)));
static StaticTask_t gMainTaskObj;

/* ---- Shared session state ------------------------------------------------- */
static z_owned_session_t     gs_session;
static SemaphoreHandle_t     gs_session_ready;
static StaticSemaphore_t     gs_session_ready_buf;

/* ---- Sample handler ------------------------------------------------------- */
static void rx_handler(z_loaned_sample_t *sample, void *ctx) {
    (void)ctx;

    z_view_string_t keystr;
    z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);

    z_owned_string_t value;
    z_bytes_to_string(z_sample_payload(sample), &value);

    DebugP_log("[z_pubsub/sub] >> ('%.*s': '%.*s')\r\n",
               (int)z_string_len(z_loan(keystr)),
               z_string_data(z_loan(keystr)),
               (int)z_string_len(z_loan(value)),
               z_string_data(z_loan(value)));

    z_drop(z_move(value));
}

/* ---- Publisher task ------------------------------------------------------- */
static void pub_task(void *arg) {
    (void)arg;

    /* Wait until the session is open */
    xSemaphoreTake(gs_session_ready, portMAX_DELAY);
    /* Give it back so sub_task can take it too */
    xSemaphoreGive(gs_session_ready);

    z_owned_publisher_t pub;
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str(&ke, PUB_KEYEXPR);
    if (z_declare_publisher(z_loan(gs_session), &pub, z_loan(ke), NULL) < 0) {
        DebugP_log("[z_pubsub/pub] z_declare_publisher failed\r\n");
        vTaskDelete(NULL);
        return;
    }

    DebugP_log("[z_pubsub/pub] Publishing on '%s'...\r\n", PUB_KEYEXPR);

    char buf[64];
    for (uint32_t idx = 0; ; idx++) {
        snprintf(buf, sizeof(buf), "[%4u] Hello from AM64x R5F!", idx);

        z_owned_bytes_t payload;
        z_bytes_copy_from_str(&payload, buf);
        z_publisher_put(z_loan(pub), z_move(payload), NULL);

        DebugP_log("[z_pubsub/pub] Put '%s'\r\n", buf);

        if ((idx % PUB_POOL_STAT_N) == (PUB_POOL_STAT_N - 1U)) {
            z_mem_pool_print_stats();
        }

        vTaskDelay(pdMS_TO_TICKS(PUB_PERIOD_MS));
    }

    z_drop(z_move(pub));
    vTaskDelete(NULL);
}

/* ---- Subscriber task ------------------------------------------------------ */
static void sub_task(void *arg) {
    (void)arg;

    /* Wait until the session is open */
    xSemaphoreTake(gs_session_ready, portMAX_DELAY);
    xSemaphoreGive(gs_session_ready);

    z_owned_subscriber_t sub;
    z_owned_closure_sample_t cb;
    z_closure(&cb, rx_handler, NULL, NULL);

    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str(&ke, SUB_KEYEXPR);
    if (z_declare_subscriber(z_loan(gs_session), &sub, z_loan(ke), z_move(cb), NULL) < 0) {
        DebugP_log("[z_pubsub/sub] z_declare_subscriber failed\r\n");
        vTaskDelete(NULL);
        return;
    }

    DebugP_log("[z_pubsub/sub] Waiting on '%s'...\r\n", SUB_KEYEXPR);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000U));
    }

    z_drop(z_move(sub));
    vTaskDelete(NULL);
}

/* ---- FreeRTOS entry ------------------------------------------------------- */
void freertos_main(void *arg) {
    (void)arg;

    Drivers_open();
    Board_driversOpen();

    /* Network */
    int net_rc = zenoh_net_init();
    if (net_rc != ZENOH_NET_OK) {
        DebugP_log("[z_pubsub] Network init failed (%d)\r\n", net_rc);
        Board_driversClose();
        Drivers_close();
        vTaskDelete(NULL);
        return;
    }

    char ip_str[16];
    zenoh_net_ip_str(ip_str, sizeof(ip_str));
    DebugP_log("[z_pubsub] IP: %s\r\n", ip_str);

    /* Session ready semaphore (binary, starts taken) */
    gs_session_ready = xSemaphoreCreateBinaryStatic(&gs_session_ready_buf);

    /* Spawn pub and sub tasks before opening the session so they exist when
     * the semaphore is given — they will block on xSemaphoreTake. */
    xTaskCreateStatic(pub_task, "z_pubsub_pub", PUB_TASK_STACK,
                      NULL, configMAX_PRIORITIES - 2, gPubTaskStack, &gPubTaskObj);
    xTaskCreateStatic(sub_task, "z_pubsub_sub", SUB_TASK_STACK,
                      NULL, configMAX_PRIORITIES - 2, gSubTaskStack, &gSubTaskObj);

    /* Configure and open session */
    z_owned_config_t cfg;
    z_config_default(&cfg);
    zp_config_insert(z_loan_mut(cfg), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(cfg), Z_CONFIG_LISTEN_KEY, MULTICAST_EP);

    if (z_open(&gs_session, z_move(cfg), NULL) < 0) {
        DebugP_log("[z_pubsub] z_open failed\r\n");
        vTaskDelete(NULL);
        return;
    }
    DebugP_log("[z_pubsub] Session open OK\r\n");

    if (zp_start_read_task(z_loan_mut(gs_session), NULL) < 0 ||
        zp_start_lease_task(z_loan_mut(gs_session), NULL) < 0) {
        DebugP_log("[z_pubsub] Failed to start background tasks\r\n");
        z_drop(z_move(gs_session));
        vTaskDelete(NULL);
        return;
    }

    /* Unblock pub and sub tasks */
    xSemaphoreGive(gs_session_ready);

    vTaskDelete(NULL);
}

int main(void) {
    System_init();
    Board_init();

    xTaskCreateStatic(freertos_main, "freertos_main", MAIN_TASK_STACK,
                      NULL, configMAX_PRIORITIES - 1,
                      gMainTaskStack, &gMainTaskObj);

    vTaskStartScheduler();
    DebugP_assertNoLog(0);
    return 0;
}
