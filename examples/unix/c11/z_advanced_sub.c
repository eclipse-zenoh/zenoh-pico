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
#include <unistd.h>
#include <zenoh-pico.h>

#if Z_FEATURE_ADVANCED_SUBSCRIPTION == 1

static int parse_args(int argc, char** argv, z_owned_config_t* config, char** keyexpr);

const char* kind_to_str(z_sample_kind_t kind);

void data_handler(z_loaned_sample_t* sample, void* ctx) {
    (void)(ctx);
    z_view_string_t keystr;
    z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);
    z_owned_string_t value;
    z_bytes_to_string(z_sample_payload(sample), &value);
    printf(">> [Advanced Subscriber] Received %s ('%.*s': '%.*s')\n", kind_to_str(z_sample_kind(sample)),
           (int)z_string_len(z_loan(keystr)), z_string_data(z_loan(keystr)), (int)z_string_len(z_loan(value)),
           z_string_data(z_loan(value)));
    z_drop(z_move(value));
}

void liveliness_handler(z_loaned_sample_t* sample, void* ctx) {
    (void)(ctx);
    z_view_string_t key_string;
    z_keyexpr_as_view_string(z_sample_keyexpr(sample), &key_string);
    switch (z_sample_kind(sample)) {
        case Z_SAMPLE_KIND_PUT:
            printf(">> [Liveliness Subscriber] New alive token ('%.*s')\n", (int)z_string_len(z_loan(key_string)),
                   z_string_data(z_loan(key_string)));
            break;
        case Z_SAMPLE_KIND_DELETE:
            printf(">> [Liveliness Subscriber] Dropped token ('%.*s')\n", (int)z_string_len(z_loan(key_string)),
                   z_string_data(z_loan(key_string)));
            break;
    }
}

void miss_handler(const ze_miss_t* miss, void* arg) {
    (void)(arg);
    z_id_t id = z_entity_global_id_zid(&miss->source);
    z_owned_string_t id_string;
    z_id_to_string(&id, &id_string);
    printf(">> [Advanced Subscriber] Missed %d samples from '%.*s' !!!", miss->nb, (int)z_string_len(z_loan(id_string)),
           z_string_data(z_loan(id_string)));
    z_drop(z_move(id_string));
}

int main(int argc, char** argv) {
    char* keyexpr = "demo/example/**";

    z_owned_config_t config;
    z_config_default(&config);

    int ret = parse_args(argc, argv, &config, &keyexpr);
    if (ret != 0) {
        return ret;
    }

    printf("Opening session...\n");
    z_owned_session_t s;
    if (z_open(&s, z_move(config), NULL) < 0) {
        printf("Unable to open session!\n");
        return -1;
    }

    // Start read, lease and periodic scheduler tasks for zenoh-pico
    if (zp_start_read_task(z_loan_mut(s), NULL) < 0 || zp_start_lease_task(z_loan_mut(s), NULL) < 0 ||
        zp_start_periodic_scheduler_task(z_loan_mut(s), NULL) < 0) {
        printf("Unable to start read, lease and periodic scheduler tasks\n");
        z_drop(z_move(s));
        return -1;
    }

    ze_advanced_subscriber_options_t sub_opts;
    ze_advanced_subscriber_options_default(&sub_opts);
    ze_advanced_subscriber_history_options_default(&sub_opts.history);
    // or sub_opts.history.is_enabled = true;
    sub_opts.history.detect_late_publishers = true;
    ze_advanced_subscriber_recovery_options_default(&sub_opts.recovery);
    // or sub_opts.recovery.is_enabled = true;
    ze_advanced_subscriber_last_sample_miss_detection_options_default(&sub_opts.recovery.last_sample_miss_detection);
    // or sub_opts.recovery.last_sample_miss_detection.is_enabled = true;
    sub_opts.recovery.last_sample_miss_detection.periodic_queries_period_ms = 0;
    // use publisher heartbeats by default, otherwise enable periodic queries as follows:
    // sub_opts.recovery.last_sample_miss_detection.periodic_queries_period_ms = 1000;
    sub_opts.subscriber_detection = true;

    z_owned_closure_sample_t callback;
    z_closure(&callback, data_handler, NULL, NULL);
    printf("Declaring AdvancedSubscriber on '%s'...\n", keyexpr);
    ze_owned_advanced_subscriber_t sub;
    z_view_keyexpr_t ke;
    if (z_view_keyexpr_from_str(&ke, keyexpr) < 0) {
        printf("%s is not a valid key expression\n", keyexpr);
        return -1;
    }
    if (ze_declare_advanced_subscriber(z_loan(s), &sub, z_loan(ke), z_move(callback), &sub_opts) < 0) {
        printf("Unable to declare advanced subscriber.\n");
        return -1;
    }

    ze_owned_closure_miss_t miss_callback;
    ze_owned_sample_miss_listener_t miss_listener;
    z_closure(&miss_callback, miss_handler, NULL, NULL);
    ze_advanced_subscriber_declare_sample_miss_listener(z_loan(sub), &miss_listener, z_move(miss_callback));

    z_owned_closure_sample_t liveliness_callback;
    z_closure(&liveliness_callback, liveliness_handler, NULL, NULL);

    z_liveliness_subscriber_options_t liveliness_sub_opt;
    z_liveliness_subscriber_options_default(&liveliness_sub_opt);

    z_owned_subscriber_t liveliness_sub;
    if (ze_advanced_subscriber_detect_publishers(z_loan(sub), &liveliness_sub, z_move(liveliness_callback),
                                                 &liveliness_sub_opt) < 0) {
        printf("Unable to declare liveliness subscriber.\n");
        exit(-1);
    }

    printf("Press CTRL-C to quit...\n");
    while (1) {
        sleep(1);
    }

    // Clean up
    z_drop(z_move(miss_listener));
    z_drop(z_move(liveliness_sub));
    z_drop(z_move(sub));
    z_drop(z_move(s));
    return 0;
}

const char* kind_to_str(z_sample_kind_t kind) {
    switch (kind) {
        case Z_SAMPLE_KIND_PUT:
            return "PUT";
        case Z_SAMPLE_KIND_DELETE:
            return "DELETE";
        default:
            return "UNKNOWN";
    }
}

static int parse_args(int argc, char** argv, z_owned_config_t* config, char** ke) {
    int opt;
    while ((opt = getopt(argc, argv, "k:e:m:l:")) != -1) {
        switch (opt) {
            case 'k':
                *ke = optarg;
                break;
            case 'e':
                zp_config_insert(z_loan_mut(*config), Z_CONFIG_CONNECT_KEY, optarg);
                break;
            case 'm':
                zp_config_insert(z_loan_mut(*config), Z_CONFIG_MODE_KEY, optarg);
                break;
            case 'l':
                zp_config_insert(z_loan_mut(*config), Z_CONFIG_LISTEN_KEY, optarg);
                break;
            case '?':
                if (optopt == 'k' || optopt == 'e' || optopt == 'm' || optopt == 'l') {
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                } else {
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                }
                return 1;
            default:
                return -1;
        }
    }
    return 0;
}

#else
int main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_ADVANCED_SUBSCRIPTION but this example requires it.\n");
    return -2;
}
#endif
