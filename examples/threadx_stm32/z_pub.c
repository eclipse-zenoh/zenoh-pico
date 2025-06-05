/**
 ******************************************************************************
 * @file    z_pub.c
 * @brief   Example zenoh-pico publisher on STM32 running ThreadX

 ******************************************************************************
 */

#include <stdio.h>

#include "app_threadx.h"
#include "zenoh-pico.h"

#define MODE "client"
#define LOCATOR "serial/Serial"
#define KEYEXPR "demo/example/zenoh-pico-pub"
#define VALUE "[STM32 THREADX] Pub from Zenoh-Pico!"
#define LOGGING 0

#if LOGGING == 1
#define _LOG(...) printf(__VA_ARGS__)
#else
#define _LOG(...)
#endif

/* Pointer used by system.c implementation to allocate from pool*/
TX_BYTE_POOL* pthreadx_byte_pool;
VOID start_example_thread(ULONG initial_input) {
    z_owned_config_t config;
    z_owned_session_t s;
    z_result_t r = _Z_ERR_GENERIC;
    while (r != Z_OK) {
        // Wait until router is started
        z_config_default(&config);
        zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, MODE);
        if (strcmp(MODE, "client") == 0) {
            zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, LOCATOR);
        } else {
            zp_config_insert(z_loan_mut(config), Z_CONFIG_LISTEN_KEY, LOCATOR);
        }

        _LOG("Opening %s session ...\n", ZENOH_CONFIG_MODE);
        if ((r = z_open(&s, z_move(config), NULL)) < 0) {
            _LOG("Unable to open session!\n");
        }
    }
    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan_mut(s), NULL) < 0 || zp_start_lease_task(z_loan_mut(s), NULL) < 0) {
        _LOG("Unable to start read and lease tasks\n");
        z_drop(z_move(s));
        return;
    }

    _LOG("Declaring publisher for '%s'...\n", KEYEXPR);
    z_owned_publisher_t pub;
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str_unchecked(&ke, KEYEXPR);
    if (z_declare_publisher(z_loan(s), &pub, z_loan(ke), NULL) < 0) {
        _LOG("Unable to declare publisher for key expression!\n");
        return;
    }

    // Publish data
    while (true) {
        char buf[256];
        for (int idx = 0; 1; ++idx) {
            z_sleep_s(1);
            snprintf(buf, 256, "[%4d] %s", idx, VALUE);
            _LOG("Putting Data ('%s': '%s')...\n", KEYEXPR, buf);

            // Create payload
            z_owned_bytes_t payload;
            z_bytes_copy_from_str(&payload, buf);

            z_publisher_put_options_t options;
            z_publisher_put_options_default(&options);
            z_publisher_put(z_loan(pub), z_move(payload), &options);
        }
    }
    // Clean-up
    z_drop(z_move(pub));
    z_drop(z_move(s));
}

TX_THREAD example_thread;
UINT App_ThreadX_Init(VOID* memory_ptr) {
    UINT ret = TX_SUCCESS;
    TX_BYTE_POOL* byte_pool = (TX_BYTE_POOL*)memory_ptr;

    pthreadx_byte_pool = (TX_BYTE_POOL*)memory_ptr;
    void* pointer;

    /* Allocate the stack for the serial thread.  */
    tx_byte_allocate(byte_pool, &pointer, 4096, TX_NO_WAIT);
    /* Create the serial thread.  */
    tx_thread_create(&example_thread, "Zenoh-Pico-Example", start_example_thread, 0, pointer, 4096, 15, 15,
                     TX_NO_TIME_SLICE, TX_AUTO_START);

    return ret;
}

void MX_ThreadX_Init(void) { tx_kernel_enter(); }
