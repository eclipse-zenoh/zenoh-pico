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
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <zenoh-pico.h>

static void fprint_zid(FILE *stream, const z_id_t *id) {
    for (int i = 15; i >= 0; i--) {
        fprintf(stream, "%02X", id->id[i]);
    }
}

void print_zid(const z_id_t *id, void *ctx) {
    (void)(ctx);
    printf(" ");
    fprint_zid(stdout, id);
    printf("\n");
}

#if Z_FEATURE_CONNECTIVITY == 1
static const char *bool_to_str(bool value) { return value ? "true" : "false"; }

static void print_transport(z_loaned_transport_t *transport, void *ctx) {
    (void)(ctx);
    z_id_t zid = z_transport_zid(transport);
    z_view_string_t whatami;
    z_whatami_to_view_string(z_transport_whatami(transport), &whatami);

    printf("  transport{zid=");
    fprint_zid(stdout, &zid);
    printf(", whatami=%.*s, is_qos=%s, is_multicast=%s, is_shm=%s}\n", (int)z_string_len(z_view_string_loan(&whatami)),
           z_string_data(z_view_string_loan(&whatami)), bool_to_str(z_transport_is_qos(transport)),
           bool_to_str(z_transport_is_multicast(transport)), bool_to_str(z_transport_is_shm(transport)));
}

static void print_link(z_loaned_link_t *link, void *ctx) {
    (void)(ctx);
    z_id_t zid = z_link_zid(link);
    z_owned_string_t src;
    z_owned_string_t dst;
    bool has_src = z_link_src(link, &src) == 0;
    bool has_dst = z_link_dst(link, &dst) == 0;

    printf("  link{zid=");
    fprint_zid(stdout, &zid);
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
    printf(", mtu=%u, is_streamed=%s, is_reliable=%s}\n", (unsigned)z_link_mtu(link),
           bool_to_str(z_link_is_streamed(link)), bool_to_str(z_link_is_reliable(link)));

    if (has_src) {
        z_string_drop(z_string_move(&src));
    }
    if (has_dst) {
        z_string_drop(z_string_move(&dst));
    }
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
    if (z_closure_transport(&transport_cb, print_transport, NULL, NULL) < 0 ||
        z_info_transports(z_session_loan(&s), z_closure_transport_move(&transport_cb)) < 0) {
        printf("Unable to fetch connected transports\n");
        z_session_drop(z_session_move(&s));
        return -1;
    }

    printf("Connected links:\n");
    z_owned_closure_link_t link_cb;
    if (z_closure_link(&link_cb, print_link, NULL, NULL) < 0 ||
        z_info_links(z_session_loan(&s), z_closure_link_move(&link_cb), NULL) < 0) {
        printf("Unable to fetch connected links\n");
        z_session_drop(z_session_move(&s));
        return -1;
    }
#endif

    z_session_drop(z_session_move(&s));
    return 0;
}
