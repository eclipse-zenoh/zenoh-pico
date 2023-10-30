//
// Copyright (c) 2023 Fictionlab sp. z o.o.
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   Błażej Sowa, <blazej@fictionlab.pl>

#include <stdio.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"
#include "task.h"

#define APP_TASK_STACK_DEPTH configMINIMAL_STACK_SIZE
#define APP_TASK_PRIORITY 10

static const uint8_t ucIPAddress[] = {192, 168, 1, 80};
static const uint8_t ucNetMask[] = {255, 255, 255, 0};
static const uint8_t ucGatewayAddress[] = {192, 168, 1, 1};
static const uint8_t ucDNSServerAddress[] = {192, 168, 1, 1};
static const uint8_t ucMACAddress[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};

NetworkInterface_t *pxLibslirp_FillInterfaceDescriptor(BaseType_t xEMACIndex, NetworkInterface_t *pxInterface);

static NetworkInterface_t xInterface;
static NetworkEndPoint_t xEndPoint;

static TaskHandle_t xAppTaskHandle;
static StaticTask_t xAppTaskTCB;
static StackType_t uxAppTaskStack[APP_TASK_STACK_DEPTH];

void app_main();

static void vAppTask(void * /*argument*/) {
    printf("Waiting for network...\n");

    uint32_t ulNotificationValue = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(5000));

    if (ulNotificationValue == 0) {
        printf("Timed out waiting for network.\n");
    } else {
        printf("Starting Zenoh App...\n");
        app_main();
    }

    vTaskDelete(NULL);
}

int main(int argc, char **argv) {
    memcpy(ipLOCAL_MAC_ADDRESS, ucMACAddress, sizeof(ucMACAddress));

    pxLibslirp_FillInterfaceDescriptor(0, &xInterface);

    FreeRTOS_FillEndPoint(&xInterface, &xEndPoint, ucIPAddress, ucNetMask, ucGatewayAddress, ucDNSServerAddress,
                          ucMACAddress);
    xEndPoint.bits.bWantDHCP = 1;

    FreeRTOS_IPInit_Multi();

    xAppTaskHandle =
        xTaskCreateStatic(vAppTask, "", APP_TASK_STACK_DEPTH, NULL, APP_TASK_PRIORITY, uxAppTaskStack, &xAppTaskTCB);
    vTaskStartScheduler();

    return 0;
}

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize) {
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

BaseType_t xApplicationGetRandomNumber(uint32_t *pulNumber) {
    *pulNumber = (uint32_t)rand();

    return pdTRUE;
}

uint32_t ulApplicationGetNextSequenceNumber(uint32_t /*ulSourceAddress*/, uint16_t /*usSourcePort*/,
                                            uint32_t /*ulDestinationAddress*/, uint16_t /*usDestinationPort*/) {
    uint32_t ulNext = 0;
    xApplicationGetRandomNumber(&ulNext);

    return ulNext;
}

const char *pcApplicationHostnameHook(void) { return "FreeRTOS-simulator"; }

void vApplicationIPNetworkEventHook_Multi(eIPCallbackEvent_t eNetworkEvent, struct xNetworkEndPoint * /*xEndPoint*/) {
    if (eNetworkEvent == eNetworkUp) {
        printf("Network is up!\n");

        uint32_t ulIPAddress = 0;
        uint32_t ulNetMask = 0;
        uint32_t ulGatewayAddress = 0;
        uint32_t ulDNSServerAddress = 0;
        char cBuf[50];

        // The network is up and configured. Print out the configuration obtained
        // from the DHCP server.
        FreeRTOS_GetEndPointConfiguration(&ulIPAddress, &ulNetMask, &ulGatewayAddress, &ulDNSServerAddress, &xEndPoint);

        // Convert the IP address to a string then print it out.
        FreeRTOS_inet_ntoa(ulIPAddress, cBuf);
        printf("IP Address: %s\n", cBuf);

        // Convert the net mask to a string then print it out.
        FreeRTOS_inet_ntoa(ulNetMask, cBuf);
        printf("Subnet Mask: %s\n", cBuf);

        // Convert the IP address of the gateway to a string then print it out.
        FreeRTOS_inet_ntoa(ulGatewayAddress, cBuf);
        printf("Gateway IP Address: %s\n", cBuf);

        // Convert the IP address of the DNS server to a string then print it out.
        FreeRTOS_inet_ntoa(ulDNSServerAddress, cBuf);
        printf("DNS server IP Address: %s\n", cBuf);

        // Make sure MAC address of the gateway is known
        if (xARPWaitResolution(ulGatewayAddress, pdMS_TO_TICKS(3000)) < 0) {
            xTaskNotifyGive(xAppTaskHandle);
        } else {
            printf("Failed to obtain the MAC address of the gateway!\n");
        }

    } else if (eNetworkEvent == eNetworkDown) {
        printf("IPv4 End Point is down!\n");
    }
}
