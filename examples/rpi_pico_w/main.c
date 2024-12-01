#include <stdio.h>
#include <zenoh-pico.h>

#include "FreeRTOS.h"
#include "lwip/sockets.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

// #define WIFI_SSID "your_ssid"
// #define WIFI_PASSWORD "your_password"

void blink(int count) {
    for (int i = 0; i < count; ++i) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(250);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(350);
    }
}

int main() {
    stdio_init_all();

    blink(1);

    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed\n");
        return -1;
    }

    cyw43_arch_enable_sta_mode();

    blink(2);

    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Failed to connect to Wi-Fi\n");
        cyw43_arch_deinit();
        return -1;
    }

    printf("Connected to Wi-Fi\n");
    blink(3);

    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, "client");
    zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, "tcp/192.168.0.108:7447");

    printf("Opening session...\n");
    z_owned_session_t s;
    if (z_open(&s, z_move(config), NULL) < 0) {
        printf("Unable to open session!\n");
        return -1;
    }

    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan_mut(s), NULL) < 0 || zp_start_lease_task(z_loan_mut(s), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        z_drop(z_move(s));
        return -1;
    }

    const char* keyexpr = "demo/example/zenoh-pico-pub";
    const char* value = "[RPI] Publication from Zenoh-Pico!";

    // Declare publisher
    printf("Declaring publisher for '%s'...\n", keyexpr);
    z_owned_publisher_t pub;
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str(&ke, keyexpr);
    if (z_declare_publisher(z_loan(s), &pub, z_loan(ke), NULL) < 0) {
        printf("Unable to declare publisher for key expression!\n");
        return -1;
    }

    // Publish data
    char buf[256];
    for (int idx = 0;; ++idx) {
        blink(1);
        z_sleep_s(1);
        sprintf(buf, "[%4d] %s", idx, value);
        printf("Putting Data ('%s': '%s')...\n", keyexpr, buf);

        // Create payload
        z_owned_bytes_t payload;
        z_bytes_copy_from_str(&payload, buf);

        z_publisher_put(z_loan(pub), z_move(payload), NULL);
    }
    // Clean up
    z_drop(z_move(pub));
    z_drop(z_move(s));

    cyw43_arch_deinit();

    return 0;
}
