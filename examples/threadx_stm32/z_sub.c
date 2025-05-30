/**
 ******************************************************************************
 * @file    z_sub.c
 * @brief   Example zenoh-pico subscriber on STM32 running Threadx
 ******************************************************************************
 */

#include "app_threadx.h"
#include "zenoh-pico.h"
#include <stdio.h>

#define MODE                    "client"
#define LOCATOR                 "serial/Serial"
#define KEYEXPR                 "demo/example/zenoh-pico-pub"
#define LOGGING                 0

#if LOGGING == 1
    #define _LOG(...) printf(__VA_ARGS__)
#else
    #define _LOG(...)
#endif


/* Pointer used by system.c implementation to allocate from pool*/
TX_BYTE_POOL* pthreadx_byte_pool;

int msg_nb = 0;
void data_handler(z_loaned_sample_t *sample, void *ctx) {
    (void)(ctx);
    z_view_string_t keystr;
    z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);
    z_owned_string_t value;
    z_bytes_to_string(z_sample_payload(sample), &value);
    _LOG(">> [Subscriber] Received ('%.*s': '%.*s')\n", (int)z_string_len(z_loan(keystr)),
           z_string_data(z_loan(keystr)), (int)z_string_len(z_loan(value)), z_string_data(z_loan(value)));
    z_drop(z_move(value));
    msg_nb++;
}

VOID start_example_thread(ULONG initial_input) {
    z_result_t r = _Z_ERR_GENERIC;
    z_owned_session_t s;
    z_owned_config_t config;

    while (r != Z_OK){
        // Wait until router is started
        z_config_default(&config);
        zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, MODE);
        if (strcmp(MODE, "client") == 0) {
           zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, LOCATOR);
        }
        else {
           zp_config_insert(z_loan_mut(config), Z_CONFIG_LISTEN_KEY, LOCATOR);
        }

        _LOG("Opening %s session ...\n", ZENOH_CONFIG_MODE);
        if ((r =z_open(&s, z_move(config), NULL)) < 0) {
           _LOG("Unable to open session!\n");
        }
    }
    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan_mut(s), NULL) < 0 || zp_start_lease_task(z_loan_mut(s), NULL) < 0) {
       _LOG("Unable to start read and lease tasks\n");
       z_drop(z_move(s));
       return;
    }

    z_owned_closure_sample_t callback;
    z_closure(&callback, data_handler, NULL, NULL);
    _LOG("Declaring Subscriber on '%s'...\n", KEYEXPR);
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str_unchecked(&ke, KEYEXPR);
    z_owned_subscriber_t sub;
    if (z_declare_subscriber(z_loan(s), &sub, z_loan(ke), z_move(callback), NULL) < 0) {
       _LOG("Unable to declare subscriber.\n");
       return;
    }

    while (1) {
       z_sleep_s(1);
    }

    // Clean-up
    z_drop(z_move(sub));
    z_drop(z_move(s));

}

TX_THREAD example_thread;
UINT App_ThreadX_Init(VOID *memory_ptr)
{
  UINT ret = TX_SUCCESS;
  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL*)memory_ptr;

  pthreadx_byte_pool = (TX_BYTE_POOL*)memory_ptr;
  void* pointer;

  /* Allocate the stack for the serial thread.  */
  tx_byte_allocate(byte_pool, &pointer, 4096, TX_NO_WAIT);
  /* Create the serial thread.  */
  tx_thread_create(&example_thread, "Zenoh-Pico-Example", start_example_thread,
          0, pointer, 4096, 15, 15, TX_NO_TIME_SLICE, TX_AUTO_START);

  return ret;
}

void MX_ThreadX_Init(void)
{
  tx_kernel_enter();
}
