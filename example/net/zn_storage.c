/*
 * Copyright (c) 2017, 2020 ADLINK Technology Inc.
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
#include <unistd.h>
#include "zenoh.h"

#define MAX_LEN 256

typedef struct sample {
  char *rname;
  char *data;
  size_t length;
} sample_t;

z_list_t *stored = 0;

int remove_data(void *elem, void*args){
  sample_t *sample = (sample_t*)elem;
  if(strcmp(sample->rname, (char *)args) == 0){
    free(sample->rname);
    free(sample->data);
    return 1;
  }
  return 0;
}

void data_handler(const zn_resource_key_t *rkey, const unsigned char *data, size_t length, const zn_data_info_t *info, void *arg) {    
  Z_UNUSED_ARG_2(info, arg);
  char str[MAX_LEN];
  memcpy(&str, data, length < MAX_LEN ? length : MAX_LEN - 1);
  str[length < MAX_LEN ? length : MAX_LEN - 1] = 0;
  printf(">> [Storage listener] Received ('%20s' : '%s')\n", rkey->key.rname, str);
  stored = z_list_remove(stored, remove_data, rkey->key.rname);

  sample_t *sample = (sample_t *)malloc(sizeof(sample_t));
  sample->rname = strdup(rkey->key.rname);
  sample->data = malloc(length);
  memcpy(sample->data, data, length);
  sample->length = length;

  stored = z_list_cons(stored, sample);
}

void query_handler(const char *rname, const char *predicate, zn_replies_sender_t send_replies, void *query_handle, void *arg) {
  Z_UNUSED_ARG(arg);
  printf(">> [Query handler   ] Handling '%s?%s'\n", rname, predicate);
  zn_resource_p_array_t replies;
  z_list_t *matching_samples = 0;

  z_list_t *samples = stored;
  sample_t *sample;
  while (samples != z_list_empty) {
    sample = (sample_t *) z_list_head(samples);
    if(zn_rname_intersect((char *)rname, sample->rname))
    {
      matching_samples = z_list_cons(matching_samples, sample);
    }
    samples = z_list_tail(samples);
  }
  replies.length = z_list_len(matching_samples);

  zn_resource_t resources[replies.length];
  zn_resource_t *p_resources[replies.length];
  
  samples = matching_samples; 
  int i =0;
  while (samples != z_list_empty) {
    sample = (sample_t *) z_list_head(samples);
    resources[i].rname = sample->rname;
    resources[i].data = (const unsigned char *)sample->data;
    resources[i].length = sample->length;
    resources[i].encoding = 0;
    resources[i].kind = 0;
    p_resources[i] = &resources[i];
    samples = z_list_tail(samples);
    ++i;
  }
  z_list_free(&matching_samples);

  replies.elem = &p_resources[0];

  send_replies(query_handle, replies);
}

int main(int argc, char **argv) {
  char *selector = "/zenoh/examples/**";
  char *locator = 0;

  if ((argc > 1) && ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0))) {
    printf("USAGE:\n\tzn_storage [<selector>=%s] [<locator>=auto]\n\n", selector);
    return 0;
  }
  if (argc > 1) {
    selector = argv[1];
  }
  if (argc > 2) {
    locator = argv[2];
  }

  printf("Openning session...\n");
  zn_session_p_result_t r_z = zn_open(locator, 0, 0);
  ASSERT_RESULT(r_z, "Unable to open session.\n")
  zn_session_t *z = r_z.value.session;
  zn_start_recv_loop(z);

  printf("Declaring Storage on '%s'...\n", selector);
  zn_sto_p_result_t r = zn_declare_storage(z, selector, data_handler, query_handler, NULL);
  ASSERT_P_RESULT(r, "Unable to declare storage.\n");  
  zn_sto_t *sto = r.value.sto;

  char c = 0;
  while (c != 'q') {
    c = fgetc(stdin);
  }

  zn_undeclare_storage(sto);
  zn_close(z);
  zn_stop_recv_loop(z);
  return 0;
}
