#include <stdio.h>

#include "FreeRTOS.h"

// #include "lwip/sockets.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

// #define WIFI_SSID "your_ssid"
// #define WIFI_PASSWORD "your_password"

int main() {
    stdio_init_all();

    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed\n");
        return -1;
    }

    cyw43_arch_enable_sta_mode();

    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Failed to connect to Wi-Fi\n");
        cyw43_arch_deinit();
        return -1;
    }

    printf("Connected to Wi-Fi\n");

    // main code

    cyw43_arch_deinit();

    return 0;
}
