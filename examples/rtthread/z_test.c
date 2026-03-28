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
#include <stdlib.h>
#include <string.h>
#include <rtthread.h>

#include "zenoh-pico.h"

static void test_random(void) {
    printf("Testing random functions...\n");
    
    uint8_t r8 = z_random_u8();
    uint16_t r16 = z_random_u16();
    uint32_t r32 = z_random_u32();
    uint64_t r64 = z_random_u64();
    
    printf("Random u8: %u\n", r8);
    printf("Random u16: %u\n", r16);
    printf("Random u32: %lu\n", r32);
    printf("Random u64: %llu\n", r64);
    
    uint8_t buf[16];
    z_random_fill(buf, sizeof(buf));
    printf("Random buffer: ");
    for (int i = 0; i < sizeof(buf); i++) {
        printf("%02x ", buf[i]);
    }
    printf("\n");
}

static void test_memory(void) {
    printf("Testing memory functions...\n");
    
    void *ptr1 = z_malloc(100);
    if (ptr1) {
        printf("z_malloc(100): OK\n");
        memset(ptr1, 0xAA, 100);
        
        void *ptr2 = z_realloc(ptr1, 200);
        if (ptr2) {
            printf("z_realloc(200): OK\n");
            z_free(ptr2);
            printf("z_free(): OK\n");
        } else {
            printf("z_realloc(200): FAILED\n");
            z_free(ptr1);
        }
    } else {
        printf("z_malloc(100): FAILED\n");
    }
}

static void test_mutex(void) {
    printf("Testing mutex functions...\n");
    
    _z_mutex_t mutex;
    if (_z_mutex_init(&mutex) == 0) {
        printf("_z_mutex_init(): OK\n");
        
        if (_z_mutex_lock(&mutex) == 0) {
            printf("_z_mutex_lock(): OK\n");
            
            if (_z_mutex_try_lock(&mutex) != 0) {
                printf("_z_mutex_try_lock() (should fail): OK\n");
            } else {
                printf("_z_mutex_try_lock() (should fail): UNEXPECTED SUCCESS\n");
            }
            
            if (_z_mutex_unlock(&mutex) == 0) {
                printf("_z_mutex_unlock(): OK\n");
            } else {
                printf("_z_mutex_unlock(): FAILED\n");
            }
        } else {
            printf("_z_mutex_lock(): FAILED\n");
        }
        
        _z_mutex_drop(&mutex);
        printf("_z_mutex_drop(): OK\n");
    } else {
        printf("_z_mutex_init(): FAILED\n");
    }
}

static void test_clock(void) {
    printf("Testing clock functions...\n");
    
    z_clock_t start = z_clock_now();
    printf("Clock start: %lu\n", start.time);
    
    z_sleep_ms(100);
    
    z_clock_t end = z_clock_now();
    printf("Clock end: %lu\n", end.time);
    
    unsigned long elapsed_ms = z_clock_elapsed_ms(&start);
    printf("Elapsed time: %lu ms\n", elapsed_ms);
    
    z_time_t time_now = z_time_now();
    printf("Time now: %lu\n", time_now.time);
}

static void test_config(void) {
    printf("Testing configuration...\n");
    
    z_owned_config_t config;
    z_config_default(&config);
    printf("z_config_default(): OK\n");
    
    // Test inserting configuration values
    zp_config_insert(z_loan(config), Z_CONFIG_MODE_KEY, Z_CONFIG_MODE_CLIENT);
    printf("zp_config_insert(MODE): OK\n");
    
    z_drop(z_move(config));
    printf("z_drop(config): OK\n");
}

int main(int argc, char **argv) {
    printf("=== Zenoh-Pico RT-Thread Test ===\n");
    
    test_random();
    printf("\n");
    
    test_memory();
    printf("\n");
    
    test_mutex();
    printf("\n");
    
    test_clock();
    printf("\n");
    
    test_config();
    printf("\n");
    
    printf("=== All tests completed ===\n");
    return 0;
}

// RT-Thread MSH command
static int zenoh_test(int argc, char **argv) {
    return main(argc, argv);
}
MSH_CMD_EXPORT(zenoh_test, zenoh-pico basic functionality test);