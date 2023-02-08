//
// Copyright (c) 2022 ZettaScale Technology
// Copyright (c) 2022 NXP
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
//   Peter van der Perk, <peter.vanderperk@nxp.com>
//

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <zenoh-pico.h>
#include <rcl_interfaces/msg/Log.h>

// CycloneDDS CDR Deserializer
#include <dds/cdr/dds_cdrstream.h>

// CDR Xtypes header {0x00, 0x01} indicates it's Little Endian (CDR_LE representation)
const uint8_t ros2_header[4] = {0x00, 0x01, 0x00, 0x00};

const size_t alloc_size = 4096; // Abitrary size 

int main(int argc, char **argv) {
    const char *keyexpr = "rt/zenoh_log_test";    
    const char *value = "Pub from Pico IDL!";
    const char *mode = "client";
    char *locator = NULL;
    rcl_interfaces_msg_Log msg;

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
    
    // Set HelloWorld IDL message
    msg.stamp.sec = 0;
    msg.stamp.nanosec = 0;
    msg.level = 20;
    msg.name = "zenoh_log_test";
    msg.msg = "Hello from Zenoh to ROS2 encoded with CycloneDDS dds_cdrstream serializer";
    msg.function = "z_publisher_put";
    msg.file = "z_pub_ros2.c";
    msg.line = 138;

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
    
    // Setup ostream for serializer
    dds_ostream_t os;
    struct dds_cdrstream_desc desc;
    
    // Allocate buffer for serialized message
    uint8_t *buf = malloc(alloc_size);
    
    struct timespec ts;
    

    for (int idx = 0; 1; ++idx) {
        sleep(1);
        printf("Putting Data ('%s')...\n", keyexpr);
        
        // Add ROS2 header
        memcpy(buf, ros2_header, sizeof(ros2_header));
      
        os.m_buffer = buf;
        os.m_index = sizeof(ros2_header); // Offset for CDR Xtypes header
        os.m_size = alloc_size;
        os.m_xcdr_version = DDSI_RTPS_CDR_ENC_VERSION_2;
        
        timespec_get(&ts, TIME_UTC);
        msg.stamp.sec = ts.tv_sec;
        msg.stamp.nanosec = ts.tv_nsec;
        
        dds_cdrstream_desc_from_topic_desc (&desc, &rcl_interfaces_msg_Log_desc);
        
        // Do serialization
        bool ret = dds_stream_write_sampleLE ((dds_ostreamLE_t *) &os, (void*)&msg, &desc);
        dds_cdrstream_desc_fini (&desc);
        
        if(ret) {
          z_publisher_put_options_t options = z_publisher_put_options_default();
          options.encoding = z_encoding(Z_ENCODING_PREFIX_TEXT_PLAIN, NULL);
          z_publisher_put(z_publisher_loan(&pub), (const uint8_t *)buf, os.m_index, &options);
        }
    }

    z_undeclare_publisher(z_publisher_move(&pub));

    // Stop read and lease tasks for zenoh-pico
    zp_stop_read_task(z_session_loan(&s));
    zp_stop_lease_task(z_session_loan(&s));

    z_close(z_session_move(&s));

    return 0;
}
