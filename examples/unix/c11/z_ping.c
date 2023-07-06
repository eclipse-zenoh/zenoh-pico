#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "zenoh-pico.h"
#include "zenoh-pico/system/platform.h"

// WARNING: for the sake of this example we are using "internal" structs and functions (starting with "_").
//          Synchronisation primitives are planned to be added to the API in the future.
_z_condvar_t cond;
_z_mutex_t mutex;

void callback(const z_sample_t* sample, void* context) {
    (void)sample;
    (void)context;
    _z_condvar_signal(&cond);
}
void drop(void* context) {
    (void)context;
    _z_condvar_free(&cond);
}

struct args_t {
    unsigned int size;             // -s
    unsigned int number_of_pings;  // -n
    unsigned int warmup_ms;        // -w
    uint8_t help_requested;        // -h
};
struct args_t parse_args(int argc, char** argv);

int main(int argc, char** argv) {
    struct args_t args = parse_args(argc, argv);
    if (args.help_requested) {
        printf(
            "\
		-n (optional, int, default=4): the number of pings to be attempted\n\
		-s (optional, int, default=8): the size of the payload embedded in the ping and repeated by the pong\n\
		-w (optional, int, default=0): the warmup time in ms during which pings will be emitted but not measured\n\
		-c (optional, string): the path to a configuration file for the session. If this option isn't passed, the default configuration will be used.\n\
		");
        return 1;
    }
    _z_mutex_init(&mutex);
    _z_condvar_init(&cond);
    z_owned_config_t config = z_config_default();
    z_owned_session_t session = z_open(z_move(config));
    if (!z_check(session)) {
        printf("Unable to open session!\n");
        return -1;
    }

    if (zp_start_read_task(z_loan(session), NULL) < 0 || zp_start_lease_task(z_loan(session), NULL) < 0) {
        printf("Unable to start read and lease tasks");
        return -1;
    }

    z_keyexpr_t ping = z_keyexpr_unchecked("test/ping");
    z_keyexpr_t pong = z_keyexpr_unchecked("test/pong");

    z_owned_publisher_t pub = z_declare_publisher(z_loan(session), ping, NULL);
    if (!z_check(pub)) {
        printf("Unable to declare publisher for key expression!\n");
        return -1;
    }

    z_owned_closure_sample_t respond = z_closure(callback, drop, NULL);
    z_owned_subscriber_t sub = z_declare_subscriber(z_loan(session), pong, z_move(respond), NULL);
    if (!z_check(sub)) {
        printf("Unable to declare subscriber for key expression.\n");
        return -1;
    }

    uint8_t* data = z_malloc(args.size);
    for (unsigned int i = 0; i < args.size; i++) {
        data[i] = i % 10;
    }
    _z_mutex_lock(&mutex);
    if (args.warmup_ms) {
        printf("Warming up for %dms...\n", args.warmup_ms);
        clock_t warmup_end = clock() + CLOCKS_PER_SEC * args.warmup_ms / 1000;
        for (clock_t now = clock(); now < warmup_end; now = clock()) {
            z_publisher_put(z_loan(pub), data, args.size, NULL);
            _z_condvar_wait(&cond, &mutex);
        }
    }
    clock_t* results = z_malloc(sizeof(clock_t) * args.number_of_pings);
    for (unsigned int i = 0; i < args.number_of_pings; i++) {
        clock_t start = clock();
        z_publisher_put(z_loan(pub), data, args.size, NULL);
        _z_condvar_wait(&cond, &mutex);
        clock_t end = clock();
        results[i] = end - start;
    }
    for (unsigned int i = 0; i < args.number_of_pings; i++) {
        clock_t rtt = results[i] * 1000000 / CLOCKS_PER_SEC;
        printf("%d bytes: seq=%d rtt=%ldµs lat=%ldµs\n", args.size, i, rtt, rtt / 2);
    }
    _z_mutex_unlock(&mutex);
    z_free(results);
    z_free(data);
    z_drop(z_move(pub));
    z_drop(z_move(sub));

    zp_stop_read_task(z_loan(session));
    zp_stop_lease_task(z_loan(session));

    z_close(z_move(session));
}

char* getopt(int argc, char** argv, char option) {
    for (int i = 0; i < argc; i++) {
        size_t len = strlen(argv[i]);
        if (len >= 2 && argv[i][0] == '-' && argv[i][1] == option) {
            if (len > 2 && argv[i][2] == '=') {
                return argv[i] + 3;
            } else if (i + 1 < argc) {
                return argv[i + 1];
            }
        }
    }
    return NULL;
}

struct args_t parse_args(int argc, char** argv) {
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            return (struct args_t){.help_requested = 1};
        }
    }
    char* arg = getopt(argc, argv, 's');
    unsigned int size = 8;
    if (arg) {
        size = atoi(arg);
    }
    arg = getopt(argc, argv, 'n');
    unsigned int number_of_pings = 4;
    if (arg) {
        number_of_pings = atoi(arg);
    }
    arg = getopt(argc, argv, 'w');
    unsigned int warmup_ms = 0;
    if (arg) {
        warmup_ms = atoi(arg);
    }
    return (struct args_t){
        .help_requested = 0,
        .size = size,
        .number_of_pings = number_of_pings,
        .warmup_ms = warmup_ms,
    };
}