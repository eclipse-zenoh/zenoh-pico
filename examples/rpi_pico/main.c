//
// Copyright (c) 2024 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   ZettaScale Zenoh Team, <zenoh@zettascale.tech>
//

#include <stdio.h>

#include "FreeRTOS.h"
#include "config.h"

#if WIFI_SUPPORT_ENABLED
#include "pico/cyw43_arch.h"
#endif

#include "pico/stdlib.h"
#include "task.h"

#define TASK_PRIORITY (tskIDLE_PRIORITY + 2UL)
#define WIFI_TIMEOUT 30000

int app_main();

#if WIFI_SUPPORT_ENABLED
void print_ip_address() {
    struct netif *netif = &cyw43_state.netif[CYW43_ITF_STA];
    if (netif_is_up(netif)) {
        printf("IP Address: %s\n", ip4addr_ntoa(netif_ip4_addr(netif)));
        printf("Netmask: %s\n", ip4addr_ntoa(netif_ip4_netmask(netif)));
        printf("Gateway: %s\n", ip4addr_ntoa(netif_ip4_gw(netif)));
    } else {
        printf("Network interface is down.\n");
    }
}
#endif

void main_task(void *params) {
    (void)params;
#ifndef NDEBUG
    vTaskDelay(pdMS_TO_TICKS(3000));
#endif

#if WIFI_SUPPORT_ENABLED
    if (cyw43_arch_init()) {
        printf("Failed to initialise\n");
        return;
    }

    if (strlen(WIFI_SSID) != 0) {
        cyw43_arch_enable_sta_mode();
        printf("Connecting to Wi-Fi...\n");
        if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, WIFI_TIMEOUT) == 0) {
            printf("Wi-Fi connected.\n");
            print_ip_address();
            app_main();
        } else {
            printf("Failed to connect Wi-Fi\n");
        }
    } else {
        printf("Offline mode\n");
        app_main();
    }
#else
    app_main();
#endif

    printf("Terminate.\n");

#if WIFI_SUPPORT_ENABLED
    cyw43_arch_deinit();
#endif

    vTaskDelete(NULL);
}

int main(void) {
    stdio_init_all();

    xTaskCreate(main_task, "MainThread", configMINIMAL_STACK_SIZE * 16, NULL, TASK_PRIORITY, NULL);

    vTaskStartScheduler();

    return 0;
}
