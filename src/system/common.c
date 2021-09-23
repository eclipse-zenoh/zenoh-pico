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
 *     ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#include "zenoh-pico/system/common.h"
#include "zenoh-pico/utils/private/logging.h"

/*------------------ Socket Receive ------------------*/
int _zn_recv_zbuf(_zn_socket_t sock, _z_zbuf_t *zbf)
{
    int rb = _zn_recv_bytes(sock, _z_zbuf_get_wptr(zbf), _z_zbuf_space_left(zbf));
    if (rb > 0)
        _z_zbuf_set_wpos(zbf, _z_zbuf_get_wpos(zbf) + rb);
    return rb;
}

int _zn_recv_exact_zbuf(_zn_socket_t sock, _z_zbuf_t *zbf, size_t len)
{
    int rb = _zn_recv_exact_bytes(sock, _z_zbuf_get_wptr(zbf), len);
    if (rb > 0)
        _z_zbuf_set_wpos(zbf, _z_zbuf_get_wpos(zbf) + rb);
    return rb;
}

/*------------------ Socket Send ------------------*/
int _zn_send_wbuf(_zn_socket_t sock, const _z_wbuf_t *wbf)
{
    for (size_t i = 0; i < _z_wbuf_len_iosli(wbf); i++)
    {
        z_bytes_t bs = _z_iosli_to_bytes(_z_wbuf_get_iosli(wbf, i));
        int n = bs.len;
        int wb;
        do
        {
            _Z_DEBUG("Sending wbuf on socket...");
            wb = _zn_send_bytes(sock, bs.val, n);
            _Z_DEBUG_VA(" sent %d bytes\n", wb);
            if (wb <= 0)
            {
                _Z_DEBUG_VA("Error while sending data over socket [%d]\n", wb);
                return -1;
            }
            n -= wb;
            bs.val += bs.len - n;
        } while (n > 0);
    }

    return 0;
}
