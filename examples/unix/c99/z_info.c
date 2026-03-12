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

#include <ctype.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <zenoh-pico.h>

void print_zid(const z_id_t *id, void *ctx) {
    (void)(ctx);
    z_owned_string_t id_str;
    z_id_to_string(id, &id_str);
    printf(" %.*s\n", (int)z_string_len(z_string_loan(&id_str)), z_string_data(z_string_loan(&id_str)));
    z_string_drop(z_string_move(&id_str));
}

#if Z_FEATURE_CONNECTIVITY == 1
static const char *bool_to_str(bool value) { return value ? "true" : "false"; }

static void print_transport(z_loaned_transport_t *transport, void *ctx) {
    (void)(ctx);
    z_id_t zid = z_transport_zid(transport);
    z_owned_string_t id_str;
    z_id_to_string(&zid, &id_str);
    z_view_string_t whatami;
    z_whatami_to_view_string(z_transport_whatami(transport), &whatami);

    printf("  transport{zid=%.*s, whatami=%.*s, is_qos=%s, is_multicast=%s, is_shm=%s}\n",
           (int)z_string_len(z_string_loan(&id_str)), z_string_data(z_string_loan(&id_str)),
           (int)z_string_len(z_view_string_loan(&whatami)), z_string_data(z_view_string_loan(&whatami)),
           bool_to_str(z_transport_is_qos(transport)), bool_to_str(z_transport_is_multicast(transport)),
           bool_to_str(z_transport_is_shm(transport)));
    z_string_drop(z_string_move(&id_str));
}

static void print_link(z_loaned_link_t *link, void *ctx) {
    (void)(ctx);
    z_id_t zid = z_link_zid(link);
    z_owned_string_t id_str;
    z_id_to_string(&zid, &id_str);
    z_owned_string_t src;
    z_owned_string_t dst;
    bool has_src = z_link_src(link, &src) == 0;
    bool has_dst = z_link_dst(link, &dst) == 0;

    printf("  link{zid=%.*s", (int)z_string_len(z_string_loan(&id_str)), z_string_data(z_string_loan(&id_str)));
    printf(", src=");
    if (has_src) {
        printf("%.*s", (int)z_string_len(z_string_loan(&src)), z_string_data(z_string_loan(&src)));
    } else {
        printf("<n/a>");
    }
    printf(", dst=");
    if (has_dst) {
        printf("%.*s", (int)z_string_len(z_string_loan(&dst)), z_string_data(z_string_loan(&dst)));
    } else {
        printf("<n/a>");
    }
    z_owned_string_t group;
    z_link_group(link, &group);
    z_owned_string_t auth_id;
    z_link_auth_identifier(link, &auth_id);
    z_owned_string_array_t interfaces;
    z_link_interfaces(link, &interfaces);
    uint8_t prio_min, prio_max;
    bool has_prio = z_link_priorities(link, &prio_min, &prio_max);
    z_reliability_t reliability;
    bool has_rel = z_link_reliability(link, &reliability);

    if (z_string_len(z_string_loan(&group)) > 0) {
        printf(", group=%.*s", (int)z_string_len(z_string_loan(&group)), z_string_data(z_string_loan(&group)));
    }
    if (z_string_len(z_string_loan(&auth_id)) > 0) {
        printf(", auth_id=%.*s", (int)z_string_len(z_string_loan(&auth_id)),
               z_string_data(z_string_loan(&auth_id)));
    }
    printf(", interfaces=[");
    size_t iface_len = z_string_array_len(z_string_array_loan(&interfaces));
    for (size_t i = 0; i < iface_len; i++) {
        const z_loaned_string_t *iface = z_string_array_get(z_string_array_loan(&interfaces), i);
        if (i > 0) printf(", ");
        printf("%.*s", (int)z_string_len(iface), z_string_data(iface));
    }
    printf("]");
    if (has_prio) {
        printf(", priorities=[%u, %u]", (unsigned)prio_min, (unsigned)prio_max);
    }
    if (has_rel) {
        printf(", reliability=%s", (reliability == Z_RELIABILITY_RELIABLE) ? "reliable" : "best_effort");
    }
    printf(", mtu=%u, is_streamed=%s, is_reliable=%s}\n", (unsigned)z_link_mtu(link),
           bool_to_str(z_link_is_streamed(link)), bool_to_str(z_link_is_reliable(link)));

    z_string_drop(z_string_move(&id_str));
    if (has_src) {
        z_string_drop(z_string_move(&src));
    }
    if (has_dst) {
        z_string_drop(z_string_move(&dst));
    }
    z_string_drop(z_string_move(&group));
    z_string_drop(z_string_move(&auth_id));
    z_string_array_drop(z_string_array_move(&interfaces));
}

static volatile bool running = true;

static void handle_signal(int signo) {
    (void)signo;
    running = false;
}

static void transport_event_handler(z_loaned_transport_event_t *event, void *ctx) {
    (void)ctx;
    z_sample_kind_t kind = z_transport_event_kind(event);
    const z_loaned_transport_t *transport = z_transport_event_transport(event);
    printf(">> [Transport Event] %s:\n", (kind == Z_SAMPLE_KIND_PUT) ? "Opened" : "Closed");
    print_transport((z_loaned_transport_t *)transport, NULL);
}

static void link_event_handler(z_loaned_link_event_t *event, void *ctx) {
    (void)ctx;
    z_sample_kind_t kind = z_link_event_kind(event);
    const z_loaned_link_t *link = z_link_event_link(event);
    printf(">> [Link Event] %s:\n", (kind == Z_SAMPLE_KIND_PUT) ? "Opened" : "Closed");
    print_link((z_loaned_link_t *)link, NULL);
}
#endif

int main(int argc, char **argv) {
    const char *mode = "client";
    char *clocator = NULL;
    char *llocator = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "e:m:l:")) != -1) {
        switch (opt) {
            case 'e':
                clocator = optarg;
                break;
            case 'm':
                mode = optarg;
                break;
            case 'l':
                llocator = optarg;
                break;
            case '?':
                if (optopt == 'e' || optopt == 'm' || optopt == 'l') {
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                } else {
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                }
                return 1;
            default:
                return -1;
        }
    }

    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_config_loan_mut(&config), Z_CONFIG_MODE_KEY, mode);
    if (clocator != NULL) {
        zp_config_insert(z_config_loan_mut(&config), Z_CONFIG_CONNECT_KEY, clocator);
    }
    if (llocator != NULL) {
        zp_config_insert(z_config_loan_mut(&config), Z_CONFIG_LISTEN_KEY, llocator);
    }

    printf("Opening session...\n");
    z_owned_session_t s;
    if (z_open(&s, z_config_move(&config), NULL) < 0) {
        printf("Unable to open session!\n");
        return -1;
    }

    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_session_loan_mut(&s), NULL) < 0 || zp_start_lease_task(z_session_loan_mut(&s), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        z_session_drop(z_session_move(&s));
        return -1;
    }

    z_id_t self_id = z_info_zid(z_session_loan(&s));
    printf("Own ID:");
    print_zid(&self_id, NULL);

    printf("Routers IDs:\n");
    z_owned_closure_zid_t callback;
    z_closure_zid(&callback, print_zid, NULL, NULL);
    z_info_routers_zid(z_session_loan(&s), z_closure_zid_move(&callback));

    // `callback` has been `z_move`d just above, so it's safe to reuse the variable,
    // we'll just have to make sure we `z_move` it again to avoid mem-leaks.
    printf("Peers IDs:\n");
    z_owned_closure_zid_t callback2;
    z_closure_zid(&callback2, print_zid, NULL, NULL);
    z_info_peers_zid(z_session_loan(&s), z_closure_zid_move(&callback2));

#if Z_FEATURE_CONNECTIVITY == 1
    printf("Connected transports:\n");
    z_owned_closure_transport_t transport_cb;
    z_closure_transport(&transport_cb, print_transport, NULL, NULL);
    if (z_info_transports(z_session_loan(&s), z_closure_transport_move(&transport_cb)) < 0) {
        printf("Unable to fetch connected transports\n");
        z_session_drop(z_session_move(&s));
        return -1;
    }

    printf("Connected links:\n");
    z_owned_closure_link_t link_cb;
    z_closure_link(&link_cb, print_link, NULL, NULL);
    if (z_info_links(z_session_loan(&s), z_closure_link_move(&link_cb), NULL) < 0) {
        printf("Unable to fetch connected links\n");
        z_session_drop(z_session_move(&s));
        return -1;
    }

    // Register transport event listener (background, no history to avoid duplicating already-printed items)
    printf("Declaring transport events listener...\n");
    z_owned_closure_transport_event_t te_cb;
    z_closure_transport_event(&te_cb, transport_event_handler, NULL, NULL);
    z_transport_events_listener_options_t te_opts;
    z_transport_events_listener_options_default(&te_opts);
    te_opts.history = false;
    if (z_declare_background_transport_events_listener(z_session_loan(&s), z_closure_transport_event_move(&te_cb),
                                                       &te_opts) < 0) {
        printf("Unable to declare transport events listener\n");
        z_session_drop(z_session_move(&s));
        return -1;
    }

    // Register link event listener (non-background so we can undeclare it)
    printf("Declaring link events listener...\n");
    z_owned_closure_link_event_t le_cb;
    z_closure_link_event(&le_cb, link_event_handler, NULL, NULL);
    z_link_events_listener_options_t le_opts;
    z_link_events_listener_options_default(&le_opts);
    le_opts.history = false;
    z_owned_link_events_listener_t le_listener;
    if (z_declare_link_events_listener(z_session_loan(&s), &le_listener, z_closure_link_event_move(&le_cb),
                                       &le_opts) < 0) {
        printf("Unable to declare link events listener\n");
        z_session_drop(z_session_move(&s));
        return -1;
    }

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    printf("Press CTRL-C to quit...\n");
    while (running) {
        z_sleep_s(1);
    }

    z_link_events_listener_drop(z_link_events_listener_move(&le_listener));
#endif

    z_session_drop(z_session_move(&s));
    return 0;
}
