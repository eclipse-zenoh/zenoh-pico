#include "FreeRTOS.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "task.h"

#define TEST_TASK_PRIORITY (tskIDLE_PRIORITY + 2UL)

int app_main();

void main_task(__unused void *params) {
    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return;
    }
    cyw43_arch_enable_sta_mode();
    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect.\n");
        vTaskDelete(NULL);
    } else {
        printf("Connected.\n");
    }

    app_main();

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
