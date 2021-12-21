/*
 * Copyright (c) 2017, 2021 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#include <stdio.h>
#include <assert.h>
#include "zenoh-pico.h"

#define MSG 10
#define MSG_LEN 1024
#define QRY 10
#define QRY_CLT 10
#define SET 10
#define SLEEP 1
#define TIMEOUT 60

z_str_t uri = "/demo/example/";
unsigned int idx[SET];

// The active resource, subscriber, queryable declarations
_z_list_t *subs2 = NULL; // @TODO: use type-safe list

volatile unsigned int total = 0;

volatile unsigned int datas = 0;
void data_handler(const zn_sample_t *sample, const void *arg)
{
    char res[64];
    sprintf(res, "%s%u", uri, *(unsigned int *)arg);
    printf(">> Received data: %s\t(%u/%u)\n", res, datas, total);

    assert(sample->value.len == MSG_LEN);
    assert(sample->key.len == strlen(res));
    assert(strncmp(res, sample->key.val, sample->key.len) == 0);

    datas++;
}

int main(int argc, z_str_t *argv)
{
    assert(argc == 2);

    setbuf(stdout, NULL);

    zn_properties_t *config = zn_config_default();
    zn_properties_insert(config, ZN_CONFIG_MODE_KEY, z_string_make("peer"));
    zn_properties_insert(config, ZN_CONFIG_PEER_KEY, z_string_make(argv[1]));
    int is_reliable = strncmp(argv[1], "tcp", 3) == 0;

    for (unsigned int i = 0; i < SET; i++)
        idx[i] = i;

    zn_session_t *s1 = zn_open(config);
    assert(s1 != NULL);
    z_string_t pid1 = _z_string_from_bytes(&s1->tp_manager->local_pid);
    printf("Session 1 with PID: %s\n", pid1.val);
    _z_string_clear(&pid1);

    // Start the read session session lease loops
    znp_start_read_task(s1);
    znp_start_lease_task(s1);
    znp_start_join_task(s1);

    z_sleep_s(SLEEP);

    zn_session_t *s2 = zn_open(config);
    assert(s2 != NULL);
    z_string_t pid2 = _z_string_from_bytes(&s2->tp_manager->local_pid);
    printf("Session 2 with PID: %s\n", pid2.val);
    _z_string_clear(&pid2);

    // Start the read session session lease loops
    znp_start_read_task(s2);
    znp_start_lease_task(s2);
    znp_start_join_task(s2);

    z_sleep_s(SLEEP);

    // Declare subscribers on second session
    char s1_res[64];
    for (unsigned int i = 0; i < SET; i++)
    {
        sprintf(s1_res, "%s%d", uri, i);
        zn_reskey_t rk = zn_rname(s1_res);
        zn_subscriber_t *sub = zn_declare_subscriber(s2, rk, zn_subinfo_default(), data_handler, &idx[i]);
        assert(sub != NULL);
        printf("Declared subscription on session 2: %zu %lu %s\n", sub->id, rk.rid, rk.rname);
        subs2 = _z_list_push(subs2, sub); // @TODO: use type-safe list
    }

    // Write data from firt session
    size_t len = MSG_LEN;
    const uint8_t *payload = (uint8_t *)malloc(len * sizeof(uint8_t));

    total = MSG * SET;
    for (unsigned int n = 0; n < MSG; n++)
    {
        for (unsigned int i = 0; i < SET; i++)
        {
            sprintf(s1_res, "%s%d", uri, i);
            zn_reskey_t rk = zn_rname(s1_res);
            zn_write_ext(s1, rk, payload, len, Z_ENCODING_DEFAULT, Z_DATA_KIND_DEFAULT, zn_congestion_control_t_BLOCK);
            printf("Wrote data from session 1: %lu %zu b\t(%u/%u)\n", rk.rid, len, n * SET + (i + 1), total);
            _zn_reskey_clear(&rk);
        }
    }

    // Wait to receive all the data
    z_clock_t now = z_clock_now();
    unsigned int expected = is_reliable ? total : 1;
    while (datas < expected)
    {
        assert(z_clock_elapsed_s(&now) < TIMEOUT);
        printf("Waiting for datas... %u/%u\n", datas, expected);
        z_sleep_s(SLEEP);
    }
    if (is_reliable)
        assert(datas == expected);
    else
        assert(datas >= expected);
    datas = 0;

    z_sleep_s(SLEEP);

    // Undeclare subscribers and queryables on second session
    while (subs2)
    {
        zn_subscriber_t *sub = _z_list_head(subs2); // @TODO: use type-safe list
        printf("Undeclared subscriber on session 2: %zu\n", sub->id);
        zn_undeclare_subscriber(sub);
        subs2 = _z_list_pop(subs2, _zn_noop_elem_free); // @TODO: use type-safe list
    }

    z_sleep_s(SLEEP);

    // Stop both sessions
    printf("Stopping threads on session 1\n");
    znp_stop_join_task(s1);
    znp_stop_lease_task(s1);
    znp_stop_read_task(s1);

    printf("Stopping threads on session 2\n");
    znp_stop_join_task(s2);
    znp_stop_lease_task(s2);
    znp_stop_read_task(s2);

    // Close both sessions
    printf("Closing session 1\n");
    zn_close(s1);

    z_sleep_s(SLEEP);

    printf("Closing session 2\n");
    zn_close(s2);

    // Cleanup properties
    zn_properties_free(&config);

    free((uint8_t *)payload);
    payload = NULL;

    return 0;
}
