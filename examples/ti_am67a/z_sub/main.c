/*
 * Copyright (c) 2026 ZettaScale Technology
 * Copyright (c) 2026 T3 Foundation Gemstone Project (www.t3gemstone.org)
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * z_sub — zenoh-pico subscriber example for TI AM67A Cortex-R5F.
 *
 * Subscribes to "t3/pub/**" and prints each received sample via DebugP_log.
 * Uses peer mode over UDP multicast (224.0.0.224:7446).
 *
 * Build prerequisites: see z_pub/main.c header.
 */

#include <stdint.h>
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

/* Example network init */
#include "zenoh_net_init.h"

/* ---- Configuration -------------------------------------------------------- */
#define SUB_KEYEXPR       "t3/pub/**"
#define SUB_MULTICAST_EP  "udp/224.0.0.224:7446"

/* ---- FreeRTOS task parameters --------------------------------------------- */
#define APP_TASK_NAME  "z_sub"
#define APP_TASK_PRI   (configMAX_PRIORITIES - 2)
#define APP_TASK_STACK (8192U / sizeof(configSTACK_DEPTH_TYPE))

static StackType_t  gAppTaskStack[APP_TASK_STACK] __attribute__((aligned(32)));
static StaticTask_t gAppTaskObj;

#define MAIN_TASK_STACK (16384U / sizeof(configSTACK_DEPTH_TYPE))
static StackType_t  gMainTaskStack[MAIN_TASK_STACK] __attribute__((aligned(32)));
static StaticTask_t gMainTaskObj;

/* ---- Sample handler (called from zenoh read task) ------------------------- */
static void sample_handler(z_loaned_sample_t *sample, void *ctx) {
    (void)ctx;

    z_view_string_t keystr;
    z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);

    z_owned_string_t value;
    z_bytes_to_string(z_sample_payload(sample), &value);

    DebugP_log("[z_sub] >> ('%.*s': '%.*s')\r\n",
               (int)z_string_len(z_loan(keystr)),
               z_string_data(z_loan(keystr)),
               (int)z_string_len(z_loan(value)),
               z_string_data(z_loan(value)));

    z_drop(z_move(value));
}

/* ---- Subscriber task ------------------------------------------------------ */
static void z_sub_task(void *arg) {
    (void)arg;

    /* 1. Bring up the network */
    int net_rc = zenoh_net_init();
    if (net_rc != ZENOH_NET_OK) {
        DebugP_log("[z_sub] Network init failed (%d)\r\n", net_rc);
        vTaskDelete(NULL);
        return;
    }

    char ip_str[16];
    zenoh_net_ip_str(ip_str, sizeof(ip_str));
    DebugP_log("[z_sub] IP: %s  keyexpr: %s\r\n", ip_str, SUB_KEYEXPR);

    /* 2. Configure zenoh session — peer mode, UDP multicast */
    z_owned_config_t cfg;
    z_config_default(&cfg);
    zp_config_insert(z_loan_mut(cfg), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(cfg), Z_CONFIG_LISTEN_KEY, SUB_MULTICAST_EP);

    /* 3. Open session */
    z_owned_session_t s;
    if (z_open(&s, z_move(cfg), NULL) < 0) {
        DebugP_log("[z_sub] z_open failed\r\n");
        vTaskDelete(NULL);
        return;
    }

    /* 4. Start background read / lease tasks */
    if (zp_start_read_task(z_loan_mut(s), NULL) < 0 ||
        zp_start_lease_task(z_loan_mut(s), NULL) < 0) {
        DebugP_log("[z_sub] Failed to start background tasks\r\n");
        z_drop(z_move(s));
        vTaskDelete(NULL);
        return;
    }

    /* 5. Declare subscriber */
    z_owned_subscriber_t sub;
    z_owned_closure_sample_t cb;
    z_closure(&cb, sample_handler, NULL, NULL);

    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str(&ke, SUB_KEYEXPR);
    if (z_declare_subscriber(z_loan(s), &sub, z_loan(ke), z_move(cb), NULL) < 0) {
        DebugP_log("[z_sub] z_declare_subscriber failed\r\n");
        z_drop(z_move(s));
        vTaskDelete(NULL);
        return;
    }

    DebugP_log("[z_sub] Waiting for data on '%s'...\r\n", SUB_KEYEXPR);

    /* 6. Spin — incoming samples are delivered by the read task */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000U));
    }

    /* Unreachable */
    z_drop(z_move(sub));
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

    xTaskCreateStatic(z_sub_task, APP_TASK_NAME, APP_TASK_STACK,
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
