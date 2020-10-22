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
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include "zenoh/iobuf.h"

#define RUNS 1000

/*=============================*/
/*     Printing functions      */
/*=============================*/
void print_rbuf_overview(z_rbuf_t *rbf)
{
    printf("    RBuf => Expandable: %u, Capacity: %zu\n", rbf->is_expandable, rbf->ios.capacity);
}

void print_wbuf_overview(z_wbuf_t *wbf)
{
    printf("    WBuf => Expandable: %u, Capacity: %zu\n", wbf->is_expandable, wbf->capacity);
}

/*=============================*/
/*    Generating functions     */
/*=============================*/
int gen_bool()
{
    return rand() % 2;
}

uint8_t gen_uint8()
{
    return (uint8_t)rand() % 255;
}

size_t gen_size_t()
{
    return (size_t)rand();
}

z_rbuf_t gen_rbuf(size_t len)
{
    int is_expandable = 0;

    if (gen_bool())
    {
        is_expandable = 1;
        len = 1 + gen_uint8();
    }

    return z_rbuf_make(len, is_expandable);
}

z_wbuf_t gen_wbuf(size_t len)
{
    int is_expandable = 0;

    if (gen_bool())
    {
        is_expandable = 1;
        len = 1 + gen_uint8();
    }

    return z_wbuf_make(len, is_expandable);
}

/*=============================*/
/*      Helper functions       */
/*=============================*/
z_rbuf_t wbuf_to_rbuf(const z_wbuf_t *wbf)
{
    z_rbuf_t rbf = z_rbuf_make(z_wbuf_readable(wbf), 0);
    for (size_t i = 0; i < wbf->ioss.length; i++)
    {
        z_iosli_t *ios = &wbf->ioss.elem[i];
        z_rbuf_write_bytes(&rbf, ios->buf, ios->r_pos, z_iosli_readable(ios));
    }
    return rbf;
}

/*=============================*/
/*           Tests             */
/*=============================*/
void wbuf_write_wbuf_read()
{
    size_t len = 128;
    z_wbuf_t wbf = gen_wbuf(len);
    printf(">>> Write and Read\n");
    print_wbuf_overview(&wbf);
    printf("    Writing %zu bytes\n", len);
    for (size_t i = 0; i < len; i++)
    {
        z_wbuf_write(&wbf, (uint8_t)i % 255);
    }

    printf("    Written: %zu, Readable: %zu\n", len, z_wbuf_readable(&wbf));
    assert(z_wbuf_readable(&wbf) == len);

    printf("    Reading %zu bytes\n", len);
    for (size_t i = 0; i < len; i++)
    {
        uint8_t r = z_wbuf_read(&wbf);
        assert(r == (uint8_t)i % 255);
    }

    z_wbuf_free(&wbf);
}

// void iobuf_write_read_bytes()
// {
//     size_t len = 128;
//     z_iobuf_t iob = gen_iobuf(len);
//     printf(">>> Write and Read bytes\n");
//     print_iobuf_overview(&iob);

//     printf("    Writing %zu bytes\n", len);
//     uint8_t *buf01 = (uint8_t *)malloc(len * sizeof(uint8_t));
//     for (size_t i = 0; i < len; i++)
//     {
//         buf01[i] = (uint8_t)i % 255;
//     }
//     z_iobuf_write_bytes(&iob, buf01, 0, len);
//     printf("    Written: %zu, Readable: %zu\n", len, z_iobuf_readable(&iob));
//     assert(z_iobuf_readable(&iob) == len);

//     printf("    Reading %zu bytes\n", len);
//     uint8_t *buf02 = (uint8_t *)malloc(len * sizeof(uint8_t));
//     z_iobuf_read_bytes(&iob, buf02, 0, len);

//     for (size_t i = 0; i < len; i++)
//     {
//         assert(buf01[1] == buf02[1]);
//     }

//     free(buf01);
//     free(buf02);
//     z_iobuf_free(&iob);
// }

// void iobuf_write_read_mark()
// {
//     size_t len = 128;
//     z_iobuf_t iob = z_iobuf_make(len, Z_IOBUF_MODE_CONTIGOUS);
//     printf(">>> Write and Read mark\n");
//     print_iobuf_overview(&iob);

//     size_t to_write = len;
//     do
//     {
//         size_t amount = 1 + gen_size_t() % to_write;
//         to_write -= amount;

//         printf("    Marking %zu bytes\n", amount);
//         z_iobuf_mark_written(&iob, amount);

//         printf("    Readable: %zu, Amount: %zu\n", z_iobuf_readable(&iob), amount);
//         assert(z_iobuf_readable(&iob) == amount);
//         printf("    Writable: %zu, To write: %zu\n", z_iobuf_writable(&iob), to_write);
//         assert(z_iobuf_writable(&iob) == to_write);

//         z_iobuf_mark_read(&iob, amount);
//         assert(z_iobuf_readable(&iob) == 0);
//     } while (to_write > 0);

//     z_iobuf_free(&iob);
// }

/*=============================*/
/*            Main             */
/*=============================*/
int main()
{
    for (unsigned int i = 0; i < RUNS; ++i)
    {
        wbuf_write_wbuf_read();
    }
}
