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

//
// Copyright (c) 2025 ZettaScale Technology
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//

#include "tcp_proxy.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#ifndef TCP_MAX_CONNS
#define TCP_MAX_CONNS 8
#endif
#ifndef TCP_BUFSZ
#define TCP_BUFSZ 65536
#endif

typedef int sock_t;

typedef struct {
    sock_t cli;
    sock_t up;
} tcp_pair_t;

struct tcp_proxy {
    char lhost[64];
    char thost[64];
    uint16_t tport;

    volatile bool run;
    volatile bool enabled;
    pthread_t thr;

    sock_t lsock;  // listening socket
    tcp_pair_t conns[TCP_MAX_CONNS];
    int nconns;

    pthread_mutex_t mtx;  // guards enabled + conns[]
};

// ---- utilities ----
static void msleep_(int ms) {
    struct timespec ts = {ms / 1000, (ms % 1000) * 1000000L};
    nanosleep(&ts, NULL);
}

static int set_nonblock(sock_t s) {
    int fl = fcntl(s, F_GETFL, 0);
    return fcntl(s, F_SETFL, fl | O_NONBLOCK);
}

static sock_t tcp_listen4_ephemeral(const char* host) {
    struct sockaddr_in sa = {0};
    sa.sin_family = AF_INET;
    sa.sin_port = htons(0);  // ephemeral
    if (host == NULL) {
        host = "127.0.0.1";
    }
    if (inet_pton(AF_INET, host, &sa.sin_addr) != 1) {
        return -1;
    }
    sock_t s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        return -1;
    }
    int one = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
#ifdef SO_REUSEPORT
    setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));
#endif
    if (bind(s, (struct sockaddr*)&sa, sizeof sa) != 0) {
        close(s);
        return -1;
    }
    if (listen(s, 64) != 0) {
        close(s);
        return -1;
    }
    (void)set_nonblock(s);
    return s;
}

static int get_sock_port(sock_t s) {
    struct sockaddr_in sa;
    socklen_t sl = sizeof sa;
    if (getsockname(s, (struct sockaddr*)&sa, &sl) != 0) {
        return -1;
    }
    return (int)ntohs(sa.sin_port);
}

static int tcp_connect4(const char* host, uint16_t port) {
    char pbuf[16];
    snprintf(pbuf, sizeof pbuf, "%u", (unsigned)port);
    struct addrinfo hints = {0};
    struct addrinfo* res = NULL;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo((host != NULL) ? host : "127.0.0.1", pbuf, &hints, &res) != 0) {
        return -1;
    }
    sock_t s = -1;
    for (struct addrinfo* ai = res; ai != NULL; ai = ai->ai_next) {
        s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (s < 0) {
            continue;
        }
        if (connect(s, ai->ai_addr, ai->ai_addrlen) == 0) {
            break;
        }
        close(s);
        s = -1;
    }
    freeaddrinfo(res);
    if (s >= 0) {
        (void)set_nonblock(s);
    }
    return s;
}

static void pair_close(tcp_pair_t* pr) {
    if (pr->cli >= 0) {
        close(pr->cli);
        pr->cli = -1;
    }
    if (pr->up >= 0) {
        close(pr->up);
        pr->up = -1;
    }
}

// ---- thread ----
// When disabled, we still recv() (to ACK at TCP level) but DROP data instead of forwarding.
static void* proxy_thread(void* arg) {
    tcp_proxy_t* p = (tcp_proxy_t*)arg;

    while (p->run) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(p->lsock, &rfds);
        int maxfd = p->lsock;

        pthread_mutex_lock(&p->mtx);
        for (int i = 0; i < p->nconns; i++) {
            if (p->conns[i].cli >= 0) {
                FD_SET(p->conns[i].cli, &rfds);
                if (p->conns[i].cli > maxfd) {
                    maxfd = p->conns[i].cli;
                }
            }
            if (p->conns[i].up >= 0) {
                FD_SET(p->conns[i].up, &rfds);
                if (p->conns[i].up > maxfd) {
                    maxfd = p->conns[i].up;
                }
            }
        }
        bool forward_enabled = p->enabled;
        pthread_mutex_unlock(&p->mtx);

        struct timeval tv = (struct timeval){.tv_sec = 0, .tv_usec = 200 * 1000};  // 200 ms
        int ready = select(maxfd + 1, &rfds, NULL, NULL, &tv);
        if (ready <= 0) {
            continue;
        }

        // Accept new clients
        if (FD_ISSET(p->lsock, &rfds)) {
            struct sockaddr_in cli;
            socklen_t clen = sizeof cli;
            int cs = accept(p->lsock, (struct sockaddr*)&cli, &clen);
            if (cs >= 0) {
                (void)set_nonblock(cs);
                int ups = tcp_connect4(p->thost, p->tport);
                if (ups < 0) {
                    close(cs);
                } else {
                    pthread_mutex_lock(&p->mtx);
                    if (p->nconns < TCP_MAX_CONNS) {
                        p->conns[p->nconns].cli = cs;
                        p->conns[p->nconns].up = ups;
                        p->nconns++;
                    } else {
                        close(cs);
                        close(ups);
                    }
                    pthread_mutex_unlock(&p->mtx);
                }
            }
        }

        // Pump both directions
        pthread_mutex_lock(&p->mtx);
        for (int i = 0; i < p->nconns; i++) {
            tcp_pair_t* pr = &p->conns[i];
            char buf[TCP_BUFSZ];

            // client -> upstream
            if (pr->cli >= 0 && FD_ISSET(pr->cli, &rfds)) {
                ssize_t nread = recv(pr->cli, buf, sizeof buf, 0);
                if (nread <= 0) {
                    pair_close(pr);
                } else {
                    if (forward_enabled) {
                        size_t off = 0;
                        size_t total = (size_t)nread;
                        while (off < total) {
                            ssize_t nw = send(pr->up, buf + off, total - off, 0);
                            if (nw <= 0) {
                                pair_close(pr);
                                break;
                            }
                            off += (size_t)nw;
                        }
                    }
                    // else DROP: consumed but not forwarded (client sees ACKs)
                }
            }

            // upstream -> client
            if (pr->up >= 0 && FD_ISSET(pr->up, &rfds)) {
                ssize_t nread = recv(pr->up, buf, sizeof buf, 0);
                if (nread <= 0) {
                    pair_close(pr);
                } else {
                    if (forward_enabled) {
                        size_t off = 0;
                        size_t total = (size_t)nread;
                        while (off < total) {
                            ssize_t nw = send(pr->cli, buf + off, total - off, 0);
                            if (nw <= 0) {
                                pair_close(pr);
                                break;
                            }
                            off += (size_t)nw;
                        }
                    }
                    // else DROP: consumed but not forwarded
                }
            }
        }

        // Compact live pairs
        int w = 0;
        for (int r = 0; r < p->nconns; r++) {
            if (p->conns[r].cli >= 0) {
                p->conns[w++] = p->conns[r];
            }
        }
        p->nconns = w;
        pthread_mutex_unlock(&p->mtx);
    }

    return NULL;
}

// ---- API ----
tcp_proxy_t* tcp_proxy_create(const char* listen_host, const char* target_host, uint16_t target_port) {
    tcp_proxy_t* p = calloc(1, sizeof *p);
    if (p == NULL) {
        return NULL;
    }
    snprintf(p->lhost, sizeof p->lhost, "%s", (listen_host != NULL) ? listen_host : "127.0.0.1");
    snprintf(p->thost, sizeof p->thost, "%s", (target_host != NULL) ? target_host : "127.0.0.1");
    p->tport = target_port;

    p->run = false;
    p->enabled = true;
    p->lsock = -1;
    p->nconns = 0;
    for (int i = 0; i < TCP_MAX_CONNS; i++) {
        p->conns[i].cli = -1;
        p->conns[i].up = -1;
    }
    pthread_mutex_init(&p->mtx, NULL);
    return p;
}

int tcp_proxy_start(tcp_proxy_t* p, int ms_timeout) {
    if (p == NULL) {
        return -1;  // invalid arg
    }
    if (p->run) {
        int cur = (p->lsock >= 0) ? get_sock_port(p->lsock) : -3;
        return (cur >= 0) ? cur : -3;
    }

    // Create listening socket now so we know the ephemeral port.
    p->lsock = tcp_listen4_ephemeral(p->lhost);
    if (p->lsock < 0) {
        return -2;  // listen failed
    }
    int port = get_sock_port(p->lsock);
    if (port < 0) {
        close(p->lsock);
        p->lsock = -1;
        return -3;  // getsockname failed
    }

    p->run = true;
    if (pthread_create(&p->thr, NULL, proxy_thread, p) != 0) {
        p->run = false;
        close(p->lsock);
        p->lsock = -1;
        return -4;  // thread failed
    }

    // Optional grace period for the thread to enter its loop.
    if (ms_timeout > 0) {
        int waited = 0;
        const int step = 10;
        while (waited < ms_timeout) {
            msleep_(step);
            waited += step;
        }
    }

    return port;  // success: ephemeral port
}

int tcp_proxy_stop(tcp_proxy_t* p) {
    if (p == NULL) {
        return -1;
    }
    if (!p->run) {
        return 0;
    }
    p->run = false;
    pthread_join(p->thr, NULL);

    // Close listening socket and all pairs.
    if (p->lsock >= 0) {
        close(p->lsock);
        p->lsock = -1;
    }
    pthread_mutex_lock(&p->mtx);
    for (int i = 0; i < p->nconns; i++) {
        pair_close(&p->conns[i]);
    }
    p->nconns = 0;
    pthread_mutex_unlock(&p->mtx);

    return 0;
}

void tcp_proxy_destroy(tcp_proxy_t* p) {
    if (p == NULL) {
        return;
    }
    if (p->run) {
        (void)tcp_proxy_stop(p);
    }
    if (p->lsock >= 0) {
        close(p->lsock);
        p->lsock = -1;
    }
    pthread_mutex_destroy(&p->mtx);
    free(p);
}

void tcp_proxy_set_enabled(tcp_proxy_t* p, bool enabled) {
    if (p == NULL) {
        return;
    }
    pthread_mutex_lock(&p->mtx);
    p->enabled = enabled;
    pthread_mutex_unlock(&p->mtx);
}

bool tcp_proxy_get_enabled(const tcp_proxy_t* p) {
    if (p == NULL) {
        return false;
    }
    return p->enabled;
}

void tcp_proxy_close_all(tcp_proxy_t* p) {
    if (p == NULL) {
        return;
    }
    pthread_mutex_lock(&p->mtx);
    for (int i = 0; i < p->nconns; i++) {
        pair_close(&p->conns[i]);
    }
    p->nconns = 0;
    pthread_mutex_unlock(&p->mtx);
}

void tcp_proxy_drop_for_ms(tcp_proxy_t* p, int ms) {
    if (p == NULL) {
        return;
    }
    tcp_proxy_set_enabled(p, false);
    msleep_(ms);
    tcp_proxy_set_enabled(p, true);
}
