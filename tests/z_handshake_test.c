//
// Copyright (c) 2026 Minkyu Kim
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   Minkyu Kim, <kmk6061602@cbnu.ac.kr>
//

#include <stdio.h>
#include <string.h>

#include "utils/assert_helpers.h"
#include "zenoh-pico.h"

#if (defined(ZENOH_LINUX) || defined(ZENOH_MACOS) || defined(ZENOH_BSD)) && Z_FEATURE_LINK_TCP == 1 && \
    Z_FEATURE_UNICAST_TRANSPORT == 1 && defined(Z_FEATURE_UNSTABLE_API)

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

#include "zenoh-pico/protocol/codec/transport.h"
#include "zenoh-pico/protocol/definitions/transport.h"
#include "zenoh-pico/protocol/iobuf.h"

#define HANDSHAKE_TEST_PORT 18131
#define HANDSHAKE_TEST_LOCATOR "tcp/127.0.0.1:18131"
#define HANDSHAKE_TEST_CONNECT_TIMEOUT "1500"
#define HANDSHAKE_TEST_POLL_MS 100
#define HANDSHAKE_TEST_IO_TIMEOUT_S 2

typedef struct {
    int listen_fd;
    _z_transport_message_t reply;  // sent in place of the OpenAck
    pthread_mutex_t mutex;         // guards the fields below
    bool stop;
    int connections;
} fake_router_t;

// Read one stream frame (2-byte little-endian length, then the body) and drop it
static bool fake_router_skip_frame(int fd) {
    uint8_t len_bytes[2];
    if (recv(fd, len_bytes, sizeof(len_bytes), MSG_WAITALL) != (ssize_t)sizeof(len_bytes)) {
        return false;
    }
    size_t len = (size_t)len_bytes[0] | ((size_t)len_bytes[1] << 8);
    uint8_t body[UINT16_MAX];
    return recv(fd, body, len, MSG_WAITALL) == (ssize_t)len;
}

static bool fake_router_send(int fd, const _z_transport_message_t *msg) {
    _z_wbuf_t wbf = _z_wbuf_make(Z_BATCH_UNICAST_SIZE, false);
    bool ok = _z_transport_message_encode(&wbf, msg) == _Z_RES_OK;
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    size_t len = _z_zbuf_len(&zbf);
    uint8_t frame[2 + Z_BATCH_UNICAST_SIZE];
    frame[0] = (uint8_t)(len & 0xFF);
    frame[1] = (uint8_t)((len >> 8) & 0xFF);
    memcpy(&frame[2], _z_zbuf_start(&zbf), len);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
    return ok && (send(fd, frame, 2 + len, 0) == (ssize_t)(2 + len));
}

static bool fake_router_stopping(fake_router_t *router) {
    pthread_mutex_lock(&router->mutex);
    bool stop = router->stop;
    pthread_mutex_unlock(&router->mutex);
    return stop;
}

static void *fake_router_task(void *arg) {
    fake_router_t *router = (fake_router_t *)arg;
    _z_id_t router_zid = {0};
    router_zid.id[0] = 0x42;

    while (!fake_router_stopping(router)) {
        // A receive timeout does not bound accept() on every platform
        struct pollfd pfd = {.fd = router->listen_fd, .events = POLLIN, .revents = 0};
        if (poll(&pfd, 1, HANDSHAKE_TEST_POLL_MS) <= 0) {
            continue;
        }
        int fd = accept(router->listen_fd, NULL, NULL);
        if (fd < 0) {
            continue;
        }
        struct timeval io_timeout = {.tv_sec = HANDSHAKE_TEST_IO_TIMEOUT_S, .tv_usec = 0};
        (void)setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &io_timeout, sizeof(io_timeout));
        pthread_mutex_lock(&router->mutex);
        router->connections++;
        pthread_mutex_unlock(&router->mutex);

        // InitSyn -> InitAck, OpenSyn -> the configured reply
        _z_transport_message_t init_ack = _z_t_msg_make_init_ack(Z_WHATAMI_ROUTER, router_zid, _z_slice_null());
        if (fake_router_skip_frame(fd) && fake_router_send(fd, &init_ack) && fake_router_skip_frame(fd)) {
            (void)fake_router_send(fd, &router->reply);
        }
        close(fd);
    }
    return NULL;
}

// Returns the z_open result and the number of connection attempts
static z_result_t open_against_fake_router(_z_transport_message_t reply, int *connections) {
    fake_router_t router = {.reply = reply, .stop = false, .connections = 0};
    ASSERT_TRUE(pthread_mutex_init(&router.mutex, NULL) == 0);
    router.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_TRUE(router.listen_fd >= 0);
    int one = 1;
    ASSERT_TRUE(setsockopt(router.listen_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) == 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(HANDSHAKE_TEST_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    ASSERT_TRUE(bind(router.listen_fd, (struct sockaddr *)&addr, sizeof(addr)) == 0);
    ASSERT_TRUE(listen(router.listen_fd, 4) == 0);
    pthread_t task;
    ASSERT_TRUE(pthread_create(&task, NULL, fake_router_task, &router) == 0);

    z_owned_config_t c;
    z_config_default(&c);
    zp_config_insert(z_loan_mut(c), Z_CONFIG_MODE_KEY, "client");
    zp_config_insert(z_loan_mut(c), Z_CONFIG_CONNECT_KEY, HANDSHAKE_TEST_LOCATOR);
    zp_config_insert(z_loan_mut(c), Z_CONFIG_CONNECT_TIMEOUT_KEY, HANDSHAKE_TEST_CONNECT_TIMEOUT);
    z_owned_session_t s;
    z_result_t ret = z_open(&s, z_move(c), NULL);
    if (ret == _Z_RES_OK) {
        z_drop(z_move(s));
    }

    pthread_mutex_lock(&router.mutex);
    router.stop = true;
    *connections = router.connections;
    pthread_mutex_unlock(&router.mutex);
    ASSERT_TRUE(pthread_join(task, NULL) == 0);
    close(router.listen_fd);
    pthread_mutex_destroy(&router.mutex);
    return ret;
}

static void test_close_instead_of_open_ack_is_retried(void) {
    printf("Running test_close_instead_of_open_ack_is_retried() ...\n");
    // e.g. a router that still holds the previous session of this Zenoh ID
    int connections = 0;
    ASSERT_ERR(open_against_fake_router(_z_t_msg_make_close(_Z_CLOSE_INVALID, false), &connections),
               _Z_ERR_TRANSPORT_OPEN_FAILED);
    ASSERT_TRUE(connections >= 2);
}

static void test_unexpected_reply_is_not_retried(void) {
    printf("Running test_unexpected_reply_is_not_retried() ...\n");
    int connections = 0;
    ASSERT_ERR(open_against_fake_router(_z_t_msg_make_keep_alive(), &connections), _Z_ERR_MESSAGE_UNEXPECTED);
    ASSERT_TRUE(connections == 1);
}

int main(void) {
    test_close_instead_of_open_ack_is_retried();
    test_unexpected_reply_is_not_retried();
    return 0;
}

#else
int main(void) {
    printf("Skipping handshake tests: they need POSIX TCP sockets, unicast transport and the unstable API.\n");
    return 0;
}
#endif
