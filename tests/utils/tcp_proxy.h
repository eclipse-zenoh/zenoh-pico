//
// Copyright (c) 2025 ZettaScale Technology
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

#ifndef TCP_PROXY_H
#define TCP_PROXY_H

#include <stdbool.h>
#include <stdint.h>

typedef struct tcp_proxy tcp_proxy_t;

// Create a proxy (not started). The proxy always binds to an ephemeral port.
// listen_host may be NULL => "127.0.0.1"
// target_host may be NULL => "127.0.0.1"
tcp_proxy_t* tcp_proxy_create(const char* listen_host, const char* target_host, uint16_t target_port);

// Start/stop the background proxy thread.
// On success, returns the ephemeral TCP port bound on listen_host (>= 0).
// On failure, returns a negative error code.
int tcp_proxy_start(tcp_proxy_t* p, int ms_timeout);
int tcp_proxy_stop(tcp_proxy_t* p);

// Destroy the proxy (safe if already stopped).
void tcp_proxy_destroy(tcp_proxy_t* p);

// Enable/disable forwarding (true = forward, false = drop/blackhole).
void tcp_proxy_set_enabled(tcp_proxy_t* p, bool enabled);
bool tcp_proxy_get_enabled(const tcp_proxy_t* p);

// Hard disconnect: close all client<->upstream pairs.
void tcp_proxy_close_all(tcp_proxy_t* p);

// Convenience: drop (blackhole) for ms, then restore to enabled = true.
void tcp_proxy_drop_for_ms(tcp_proxy_t* p, int ms);

#endif  // TCP_PROXY_H
