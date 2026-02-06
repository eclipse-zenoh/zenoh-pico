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

#include <stdio.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "lwip/dhcp.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "netif/ethernet.h"
#include "task.h"

#define APP_TASK_STACK_DEPTH (configMINIMAL_STACK_SIZE * 4)
#define APP_TASK_PRIORITY 10

static ip4_addr_t ipaddr, netmask, gw;
static struct netif my_netif;

static TaskHandle_t xAppTaskHandle;
static StaticTask_t xAppTaskTCB;
static StackType_t uxAppTaskStack[APP_TASK_STACK_DEPTH];

void app_main(void);

// Minimal ethernet interface initialization for simulation
static err_t netif_init_callback(struct netif *netif) {
    netif->name[0] = 'e';
    netif->name[1] = 'n';
    netif->output = etharp_output;
    netif->linkoutput = NULL;  // Would be implemented for real hardware
    netif->mtu = 1500;
    netif->hwaddr_len = 6;
    netif->hwaddr[0] = 0x00;
    netif->hwaddr[1] = 0x11;
    netif->hwaddr[2] = 0x22;
    netif->hwaddr[3] = 0x33;
    netif->hwaddr[4] = 0x44;
    netif->hwaddr[5] = 0x55;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
    return ERR_OK;
}

static void netif_status_callback(struct netif *netif) {
    if (netif_is_up(netif)) {
        printf("Network interface is up!\n");
        printf("IP Address: %s\n", ip4addr_ntoa(netif_ip4_addr(netif)));
        printf("Subnet Mask: %s\n", ip4addr_ntoa(netif_ip4_netmask(netif)));
        printf("Gateway: %s\n", ip4addr_ntoa(netif_ip4_gw(netif)));

        // Notify app task that network is ready
        if (xAppTaskHandle != NULL) {
            xTaskNotifyGive(xAppTaskHandle);
        }
    } else {
        printf("Network interface is down!\n");
    }
}

static void vAppTask(void *argument) {
    (void)argument;

    printf("Waiting for network...\n");

    uint32_t ulNotificationValue = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(10000));

    if (ulNotificationValue == 0) {
        printf("Timed out waiting for network.\n");
    } else {
        printf("Starting Zenoh App...\n");
        app_main();
    }

    vTaskDelete(NULL);
}

int main(int argc, char **argv) {
    printf("FreeRTOS + lwIP Zenoh-Pico Example\n");

    // Initialize lwIP
    tcpip_init(NULL, NULL);

    // Configure network interface with static IP
    IP4_ADDR(&gw, 192, 168, 1, 1);
    IP4_ADDR(&ipaddr, 192, 168, 1, 100);
    IP4_ADDR(&netmask, 255, 255, 255, 0);

    netif_add(&my_netif, &ipaddr, &netmask, &gw, NULL, netif_init_callback, tcpip_input);
    netif_set_default(&my_netif);
    netif_set_status_callback(&my_netif, netif_status_callback);
    netif_set_up(&my_netif);

    // Start DHCP (optional, comment out if using static IP)
    // dhcp_start(&my_netif);

    // For simulation, manually trigger the callback
    netif_status_callback(&my_netif);

    // Create app task
    xAppTaskHandle = xTaskCreateStatic(vAppTask, "app_task", APP_TASK_STACK_DEPTH, NULL, APP_TASK_PRIORITY,
                                       uxAppTaskStack, &xAppTaskTCB);

    // Start FreeRTOS scheduler
    vTaskStartScheduler();

    return 0;
}

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer,
                                   StackType_t *pulIdleTaskStackSize) {
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer,
                                    StackType_t *pulTimerTaskStackSize) {
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
