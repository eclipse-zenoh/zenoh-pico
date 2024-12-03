#include "FreeRTOS.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "task.h"

#define TEST_TASK_PRIORITY (tskIDLE_PRIORITY + 2UL)

int app_main();

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

void main_task(__unused void *params) {
    if (cyw43_arch_init()) {
        printf("Failed to initialise\n");
        return;
    }

    cyw43_arch_enable_sta_mode();
    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000) == 0) {
        printf("Wi-Fi connected.\n");
        print_ip_address();
        app_main();
    } else {
        printf("Failed to connect Wi-Fi\n");
    }

    printf("Terminate.\n");

    cyw43_arch_deinit();

    vTaskDelete(NULL);
}

int main(void) {
    stdio_init_all();

    xTaskCreate(main_task, "TestMainThread", configMINIMAL_STACK_SIZE * 16, NULL, TEST_TASK_PRIORITY, NULL);

    vTaskStartScheduler();

    return 0;
}
