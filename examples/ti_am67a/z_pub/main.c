/*
 * Copyright (c) 2026 ZettaScale Technology
 * Copyright (c) 2026 T3 Foundation Gemstone Project (www.t3gemstone.org)
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * z_pub — zenoh-pico publisher example for TI AM67A Cortex-R5F.
 *
 * Publishes a counter message on key expression "t3/pub/data" every second
 * using peer mode over UDP multicast (224.0.0.224:7446).
 *
 * Build prerequisites:
 *   1. Generate SysConfig files for your board into examples/ti_am67a/syscfg/
 *   2. cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain/ti-arm-clang-r5f.cmake \
 *            -DZP_PLATFORM=ti_am67a -DZ_FEATURE_LINK_TCP=0 \
 *            -DBUILD_SHARED_LIBS=OFF -DBUILD_EXAMPLES=ON \
 *            -B build/ti_am67a -S .
 *   3. cmake --build build/ti_am67a --target z_pub
 */

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
#include "task.h"

/* zenoh-pico */
#include <zenoh-pico.h>

/* zenoh-pico pool allocator stats (ti_am67a-specific) */
#include "zenoh-pico/system/platform/ti_am67a/mem_pool.h"

/* Example network init */
#include "zenoh_net_init.h"

/* ---- Configuration -------------------------------------------------------- */
#define PUB_KEYEXPR       "t3/pub/data"
#define PUB_MULTICAST_EP  "udp/224.0.0.224:7446"
#define PUB_PERIOD_MS     1000U
#define PUB_POOL_STAT_N   10U   /* print pool stats every N publications */

/* ---- FreeRTOS task parameters --------------------------------------------- */
#define APP_TASK_NAME  "z_pub"
#define APP_TASK_PRI   (configMAX_PRIORITIES - 2)
#define APP_TASK_STACK (8192U / sizeof(configSTACK_DEPTH_TYPE))

static StackType_t  gAppTaskStack[APP_TASK_STACK] __attribute__((aligned(32)));
static StaticTask_t gAppTaskObj;

#define MAIN_TASK_STACK (16384U / sizeof(configSTACK_DEPTH_TYPE))
static StackType_t  gMainTaskStack[MAIN_TASK_STACK] __attribute__((aligned(32)));
static StaticTask_t gMainTaskObj;

/* ---- Publisher task ------------------------------------------------------- */
static void z_pub_task(void *arg) {
    (void)arg;

    /* 1. Bring up the network */
    int net_rc = zenoh_net_init();
    if (net_rc != ZENOH_NET_OK) {
        DebugP_log("[z_pub] Network init failed (%d)\r\n", net_rc);
        vTaskDelete(NULL);
        return;
    }

    char ip_str[16];
    zenoh_net_ip_str(ip_str, sizeof(ip_str));
    DebugP_log("[z_pub] IP: %s  keyexpr: %s\r\n", ip_str, PUB_KEYEXPR);

    /* 2. Configure zenoh session — peer mode, UDP multicast */
    z_owned_config_t cfg;
    z_config_default(&cfg);
    zp_config_insert(z_loan_mut(cfg), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(cfg), Z_CONFIG_LISTEN_KEY, PUB_MULTICAST_EP);

    /* 3. Open session */
    z_owned_session_t s;
    if (z_open(&s, z_move(cfg), NULL) < 0) {
        DebugP_log("[z_pub] z_open failed\r\n");
        vTaskDelete(NULL);
        return;
    }

    /* 4. Start background read / lease tasks */
    if (zp_start_read_task(z_loan_mut(s), NULL) < 0 ||
        zp_start_lease_task(z_loan_mut(s), NULL) < 0) {
        DebugP_log("[z_pub] Failed to start background tasks\r\n");
        z_drop(z_move(s));
        vTaskDelete(NULL);
        return;
    }

    /* 5. Declare publisher */
    z_owned_publisher_t pub;
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str(&ke, PUB_KEYEXPR);
    if (z_declare_publisher(z_loan(s), &pub, z_loan(ke), NULL) < 0) {
        DebugP_log("[z_pub] z_declare_publisher failed\r\n");
        z_drop(z_move(s));
        vTaskDelete(NULL);
        return;
    }

    DebugP_log("[z_pub] Publishing on '%s' every %u ms...\r\n",
               PUB_KEYEXPR, (unsigned)PUB_PERIOD_MS);

    /* 6. Publish loop */
    char buf[64];
    for (uint32_t idx = 0; ; idx++) {
        snprintf(buf, sizeof(buf), "[%4u] Hello from TI AM67A R5F!", idx);

        z_owned_bytes_t payload;
        z_bytes_copy_from_str(&payload, buf);
        z_publisher_put(z_loan(pub), z_move(payload), NULL);

        DebugP_log("[z_pub] Put '%s'\r\n", buf);

        if ((idx % PUB_POOL_STAT_N) == (PUB_POOL_STAT_N - 1U)) {
            z_mem_pool_print_stats();
        }

        vTaskDelay(pdMS_TO_TICKS(PUB_PERIOD_MS));
    }

    /* Unreachable in normal operation */
    z_drop(z_move(pub));
    zp_stop_read_task(z_loan_mut(s));
    zp_stop_lease_task(z_loan_mut(s));
    z_drop(z_move(s));
    zenoh_net_deinit();
    vTaskDelete(NULL);
}

/* ---- FreeRTOS entry ------------------------------------------------------- */
void freertos_main(void *arg) {
    (void)arg;

    Drivers_open();
    Board_driversOpen();

    xTaskCreateStatic(z_pub_task, APP_TASK_NAME, APP_TASK_STACK,
                      NULL, APP_TASK_PRI, gAppTaskStack, &gAppTaskObj);

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
