//
// Copyright (c) 2022 ZettaScale Technology
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

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <zenoh-pico.h>
#include <zenoh-pico/system/platform.h>

// First Fit custom / user defined allocators
#define WORD uint64_t             // FIXME: not true for other architectues like 32-bits architectures
#define VAS_SIZE 1 * 1024 * 1024  // Virtual address space in bytes

typedef struct z_m_block_hdr_t {
    size_t size;               // Block size
    struct z_m_block_t *next;  // The next block in the list
    _Bool is_used;             // Whether this block is currently being used
} z_m_block_hdr_t;

typedef struct z_m_block_t {
    z_m_block_hdr_t hdr;
    WORD data[1];  // Payload pointer. Note: this MUST always be the last member in the struct.
} z_m_block_t;

_z_mutex_t mut;
void *z_heap_start = NULL;  // Heap start. It must be explicitly initialized
void *z_heap_end = NULL;    // Heap end. Defined for convenience to avoid computing
                            // it multiple times
z_m_block_t *p_brk = NULL;  // Program break identifying the top of our local heap

void *z_minit(size_t size) {
    z_heap_start = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    if (z_heap_start == MAP_FAILED) {
        z_heap_start = NULL;
        z_heap_end = NULL;
    } else {
        z_heap_end = z_heap_start + (size * sizeof(uint8_t));
    }

    z_m_block_t *new_block = z_heap_start;
    if (((uint8_t *)new_block + (sizeof(z_m_block_hdr_t) + 0)) <= (uint8_t *)z_heap_end) {
        *new_block = (z_m_block_t){.hdr = {.next = NULL, .is_used = true, .size = 0}};
        p_brk = (z_m_block_t *)((uint8_t *)new_block->data + new_block->hdr.size);
    }

    return z_heap_start;
}

void z_muninit(void *start, size_t size) { munmap(start, size); }

// Align the size to the word size.
size_t z_malign(size_t size) {
    size_t aligned_size = (size + sizeof(WORD) - 1) & ~(sizeof(WORD) - 1);
    return aligned_size;
}

// Allocates a block of memory of size bytes, following a first fit strategy.
// Due to allignment extra bytes might be allocated.
void *z_malloc(size_t size) {
    _z_mutex_lock(&mut);
    WORD *ret = NULL;
    size_t aligned_size = z_malign(size);

    z_m_block_t *block = z_heap_start;
    _Bool found = false;
    while (block->hdr.next != NULL) {
        block = block->hdr.next;
        if (block->hdr.is_used == false && block->hdr.size >= aligned_size) {
            found = true;
            break;
        }
    }

    if (found == true) {
        ret = block->data;
        block->hdr.is_used = true;
    } else {
        z_m_block_t *new_block = p_brk;
        if (((uint8_t *)new_block + (sizeof(z_m_block_hdr_t) + aligned_size)) <= (uint8_t *)z_heap_end) {
            *new_block = (z_m_block_t){.hdr = {.next = NULL, .is_used = true, .size = aligned_size}};
            ret = new_block->data;
            block->hdr.next = new_block;
            p_brk = (z_m_block_t *)((uint8_t *)new_block->data + new_block->hdr.size);
        }
    }
    _z_mutex_unlock(&mut);

    return ret;
}

void z_free(void *ptr) {
    _z_mutex_lock(&mut);
    z_m_block_t *block = ptr - sizeof(z_m_block_hdr_t);
    block->hdr.is_used = false;
    _z_mutex_unlock(&mut);
}
//

int main(int argc, char **argv) {
    if (z_minit(VAS_SIZE) == NULL) {
        printf("Failed to reserve memory\n");
    }

    const char *keyexpr = "demo/example/zenoh-pico-pub";
    const char *value = "Pub from Pico!";
    const char *mode = "client";
    char *locator = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "k:v:e:m:")) != -1) {
        switch (opt) {
            case 'k':
                keyexpr = optarg;
                break;
            case 'v':
                value = optarg;
                break;
            case 'e':
                locator = optarg;
                break;
            case 'm':
                mode = optarg;
                break;
            case '?':
                if (optopt == 'k' || optopt == 'v' || optopt == 'e' || optopt == 'm') {
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                } else {
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                }
                return 1;
            default:
                return -1;
        }
    }

    z_owned_config_t config = z_config_default();
    zp_config_insert(z_config_loan(&config), Z_CONFIG_MODE_KEY, z_string_make(mode));
    if (locator != NULL) {
        zp_config_insert(z_config_loan(&config), Z_CONFIG_PEER_KEY, z_string_make(locator));
    }

    printf("Opening session...\n");
    z_owned_session_t s = z_open(z_config_move(&config));
    if (!z_session_check(&s)) {
        printf("Unable to open session!\n");
        return -1;
    }

    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_session_loan(&s), NULL) < 0 || zp_start_lease_task(z_session_loan(&s), NULL) < 0) {
        printf("Unable to start read and lease tasks");
        return -1;
    }

    printf("Declaring publisher for '%s'...\n", keyexpr);
    z_owned_publisher_t pub = z_declare_publisher(z_session_loan(&s), z_keyexpr(keyexpr), NULL);
    if (!z_publisher_check(&pub)) {
        printf("Unable to declare publisher for key expression!\n");
        return -1;
    }

    char *buf = (char *)malloc(256);
    for (int idx = 0; 1; ++idx) {
        sleep(1);
        snprintf(buf, 256, "[%4d] %s", idx, value);
        printf("Putting Data ('%s': '%s')...\n", keyexpr, buf);
        z_publisher_put_options_t options = z_publisher_put_options_default();
        options.encoding = z_encoding(Z_ENCODING_PREFIX_TEXT_PLAIN, NULL);
        z_publisher_put(z_publisher_loan(&pub), (const uint8_t *)buf, strlen(buf), &options);
    }

    z_undeclare_publisher(z_publisher_move(&pub));

    // Stop read and lease tasks for zenoh-pico
    zp_stop_read_task(z_session_loan(&s));
    zp_stop_lease_task(z_session_loan(&s));

    z_close(z_session_move(&s));

    free(buf);

    z_muninit(z_heap_start, VAS_SIZE);

    return 0;
}
