#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "FreeRTOS_IP.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/system/platform.h"

/*------------------ Random ------------------*/
uint8_t z_random_u8(void) { return z_random_u32(); }

uint16_t z_random_u16(void) { return z_random_u32(); }

uint32_t z_random_u32(void) {
    uint32_t ret = 0;
    xApplicationGetRandomNumber(&ret);
    return ret;
}

uint64_t z_random_u64(void) {
    uint64_t ret = 0;
    ret |= z_random_u32();
    ret = ret << 32;
    ret |= z_random_u32();
    return ret;
}

void z_random_fill(void *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        *((uint8_t *)buf) = z_random_u8();
    }
}

/*------------------ Memory ------------------*/
void *z_malloc(size_t size) { return pvPortMalloc(size); }

void *z_realloc(void *ptr, size_t size) {
    // realloc not implemented in FreeRTOS
    return NULL;
}

void z_free(void *ptr) { vPortFree(ptr); }

/*------------------ Sleep ------------------*/
int z_sleep_us(size_t time) {
    vTaskDelay(pdMS_TO_TICKS(time / 1000));
    return 0;
}

int z_sleep_ms(size_t time) {
    vTaskDelay(pdMS_TO_TICKS(time));
    return 0;
}

int z_sleep_s(size_t time) {
    vTaskDelay(pdMS_TO_TICKS(time * 1000));
    return 0;
}

/*------------------ Clock ------------------*/
z_clock_t z_clock_now(void) { return z_time_now(); }

unsigned long z_clock_elapsed_us(z_clock_t *instant) { return z_clock_elapsed_ms(instant) * 1000; }

unsigned long z_clock_elapsed_ms(z_clock_t *instant) { return z_time_elapsed_ms(instant); }

unsigned long z_clock_elapsed_s(z_clock_t *instant) { return z_time_elapsed_ms(instant) * 1000; }

/*------------------ Time ------------------*/
z_time_t z_time_now(void) { return xTaskGetTickCount(); }

const char *z_time_now_as_str(char *const buf, unsigned long buflen) {
    snprintf(buf, buflen, "%u", z_time_now());
    return buf;
}

unsigned long z_time_elapsed_us(z_time_t *time) { return z_time_elapsed_ms(time) * 1000; }

unsigned long z_time_elapsed_ms(z_time_t *time) {
    z_time_t now = z_time_now();

    unsigned long elapsed = (now - *time) * portTICK_PERIOD_MS;
    return elapsed;
}

unsigned long z_time_elapsed_s(z_time_t *time) { return z_time_elapsed_ms(time) / 1000; }
