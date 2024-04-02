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
//

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico/protocol/iobuf.h"

#undef NDEBUG
#include <assert.h>

#define RUNS 1000

/*=============================*/
/*     Printing functions      */
/*=============================*/
void print_zbuf_overview(_z_zbuf_t *zbf) { printf("    ZBuf => Capacity: %zu\n", zbf->_ios._capacity); }

void print_wbuf_overview(_z_wbuf_t *wbf) {
    printf("    WBuf => Expandable: %zu, Capacity: %zu\n", wbf->_expansion_step, wbf->_capacity);
}

void print_iosli(_z_iosli_t *ios) {
    printf("IOSli: Capacity: %zu, Rpos: %zu, Wpos: %zu, Buffer: [ ", ios->_capacity, ios->_r_pos, ios->_w_pos);
    for (size_t i = 0; i < ios->_w_pos; i++) {
        printf("%02x", ios->_buf[i]);
        if (i < ios->_capacity - 1) printf(" ");
    }
    printf(" ]");
}

/*=============================*/
/*    Generating functions     */
/*=============================*/
int gen_bool(void) { return z_random_u8() % 2; }

uint8_t gen_uint8(void) { return z_random_u8() % 255; }

size_t gen_size_t(void) {
    size_t ret = 0;
    z_random_fill(&ret, sizeof(ret));
    return ret;
}

_z_zbuf_t gen_zbuf(size_t len) { return _z_zbuf_make(len); }

_z_wbuf_t gen_wbuf(size_t len) {
    _Bool is_expandable = false;

    if (gen_bool() == true) {
        is_expandable = true;
        len = 1 + (gen_size_t() % len);
    }

    return _z_wbuf_make(len, is_expandable);
}

/*=============================*/
/*           Tests             */
/*=============================*/
void iosli_writable_readable(void) {
    size_t len = 128;
    _z_iosli_t ios = _z_iosli_make(len);
    _z_iosli_t *pios = _z_iosli_new(len);

    printf("\n>>> IOSli => Writable and Readable\n");

    printf("  - IOSli write\n");
    assert(_z_iosli_writable(&ios) == _z_iosli_writable(pios));
    assert(_z_iosli_readable(&ios) == _z_iosli_readable(pios));

    size_t writable = _z_iosli_writable(&ios);
    size_t readable = _z_iosli_readable(&ios);

    printf("    Writable: %zu\tReadable: %zu\n", writable, readable);
    assert(writable == len);
    assert(readable == 0);

    for (size_t i = 0; i < len; i++) {
        uint8_t byte = gen_uint8();
        _z_iosli_write(&ios, byte);
        _z_iosli_write(pios, byte);

        assert(_z_iosli_writable(&ios) == _z_iosli_writable(pios));
        assert(_z_iosli_readable(&ios) == _z_iosli_readable(pios));

        writable = _z_iosli_writable(&ios);
        readable = _z_iosli_readable(&ios);

        printf("    Writable: %zu\tReadable: %zu\n", writable, readable);
        assert(writable == len - (i + 1));
        assert(readable == i + 1);

        uint8_t b1 = _z_iosli_get(&ios, i);
        uint8_t b2 = _z_iosli_get(pios, i);

        assert(b1 == b2);
        (void)(b1);
        (void)(b2);
    }

    _z_iosli_t *cios = _z_iosli_clone(pios);
    assert(_z_iosli_writable(pios) == _z_iosli_writable(cios));
    assert(_z_iosli_readable(pios) == _z_iosli_readable(cios));

    printf("  - IOSli read\n");
    for (size_t i = 0; i < len; i++) {
        uint8_t b1 = _z_iosli_read(&ios);
        uint8_t b2 = _z_iosli_read(pios);
        uint8_t b3 = _z_iosli_read(cios);

        assert(b1 == b2 && b2 == b3);
        (void)(b1);
        (void)(b2);
        (void)(b3);

        assert(_z_iosli_writable(&ios) == _z_iosli_writable(pios) &&
               _z_iosli_writable(pios) == _z_iosli_writable(cios));
        assert(_z_iosli_readable(&ios) == _z_iosli_readable(pios) &&
               _z_iosli_readable(pios) == _z_iosli_readable(cios));

        writable = _z_iosli_writable(&ios);
        readable = _z_iosli_readable(&ios);

        printf("    Writable: %zu\tReadable: %zu\n", writable, readable);
        assert(writable == 0);
        assert(readable == len - (i + 1));
    }

    _z_iosli_reset(&ios);
    assert(_z_iosli_writable(&ios) == len);
    assert(_z_iosli_readable(&ios) == 0);

    _z_iosli_reset(pios);
    assert(_z_iosli_writable(pios) == len);
    assert(_z_iosli_readable(pios) == 0);

    _z_iosli_clear(cios);
    assert(_z_iosli_writable(cios) == 0);
    assert(_z_iosli_readable(cios) == 0);

    printf("  - IOSli bytes\n");
    uint8_t *payload = (uint8_t *)z_malloc(len);
    memset((uint8_t *)payload, 1, len);

    for (size_t i = 0; i < len; i++) {
        payload[i] = gen_uint8();
        _z_iosli_put(pios, payload[i], i);

        assert(payload[i] == _z_iosli_get(pios, i));
        pios->_w_pos++;

        writable = _z_iosli_writable(pios);
        readable = _z_iosli_readable(pios);

        printf("    Writable: %zu\tReadable: %zu\n", writable, readable);
        assert(writable == len - (i + 1));
        assert(readable == i + 1);
    }

    uint8_t *buffer = (uint8_t *)z_malloc(len);
    memset((uint8_t *)buffer, 1, len);

    _z_iosli_write_bytes(&ios, payload, 0, len);
    _z_iosli_read_bytes(&ios, buffer, 0, len);
    assert(memcmp(payload, buffer, len) == 0);

    memset(buffer, 0, len);

    _z_iosli_read_bytes(pios, buffer, 0, len);
    assert(memcmp(payload, buffer, len) == 0);

    memset(buffer, 0, len);

    _z_iosli_t wios = _z_iosli_wrap(payload, len, 0, len);
    _z_iosli_read_bytes(&wios, buffer, 0, len);
    assert(memcmp(payload, buffer, len) == 0);

    _z_iosli_clear(&ios);
    assert(_z_iosli_writable(&ios) == 0);
    assert(_z_iosli_readable(&ios) == 0);

    _z_iosli_clear(pios);
    assert(_z_iosli_writable(pios) == 0);
    assert(_z_iosli_readable(pios) == 0);

    _z_iosli_clear(&wios);
    assert(_z_iosli_writable(&wios) == 0);
    assert(_z_iosli_readable(&wios) == 0);

    _z_iosli_clear(pios);
    z_free(pios);
    _z_iosli_clear(cios);
    z_free(cios);

    z_free(buffer);
    z_free(payload);
}

void zbuf_writable_readable(void) {
    size_t len = 128;
    _z_zbuf_t zbf = _z_zbuf_make(len);
    printf("\n>>> ZBuf => Writable and Readable\n");

    size_t writable = _z_zbuf_space_left(&zbf);
    printf("    Writable: %zu\n", writable);
    assert(writable == len);

    size_t readable = _z_zbuf_len(&zbf);
    printf("    Readable: %zu\n", readable);
    assert(readable == 0);

    _z_zbuf_set_wpos(&zbf, len);

    size_t read = 0;
    while (read < len) {
        size_t to_read = 1 + gen_size_t() % (len - read);
        for (size_t i = 0; i < to_read; i++) {
            _z_zbuf_read(&zbf);
        }
        read = read + to_read;

        writable = _z_zbuf_space_left(&zbf);
        printf("    Writable: %zu\n", writable);
        assert(writable == 0);

        readable = _z_zbuf_len(&zbf);
        printf("    Readable: %zu\n", readable);
        assert(readable == len - read);
    }

    _z_zbuf_clear(&zbf);
}

void zbuf_compact(void) {
    uint8_t len = 128;
    _z_zbuf_t zbf = _z_zbuf_make(len);
    printf("\n>>> ZBuf => Compact\n");

    for (uint8_t i = 0; i < len; i++) {
        _z_iosli_write(&zbf._ios, i);
    }

    uint8_t counter = 0;

    while (counter < len) {
        size_t len01 = _z_zbuf_len(&zbf);
        _z_zbuf_compact(&zbf);
        assert(_z_zbuf_get_rpos(&zbf) == 0);
        assert(_z_zbuf_get_wpos(&zbf) == len01);
        assert(_z_zbuf_len(&zbf) == len01);
        printf("    Len: %zu, Rpos: %zu, Wpos: %zu\n", len01, _z_zbuf_get_rpos(&zbf), _z_zbuf_get_wpos(&zbf));

        uint8_t vs = (uint8_t)(1 + gen_uint8() % (len - counter));
        printf("    Read %u bytes => [", vs);
        for (uint8_t i = 0; i < vs; i++) {
            uint8_t l = counter++;
            uint8_t r = _z_zbuf_read(&zbf);
            printf(" %02x:%02x", l, r);
            assert(l == r);
        }
        printf(" ]\n");
    }

    _z_zbuf_clear(&zbf);
}

void zbuf_view(void) {
    uint8_t len = 128;
    _z_zbuf_t zbf = _z_zbuf_make(len);
    printf("\n>>> ZBuf => View\n");

    for (uint8_t i = 0; i < len; i++) {
        _z_iosli_write(&zbf._ios, i);
    }

    uint8_t counter = 0;

    while (counter < len) {
        uint8_t vs = (uint8_t)(1 + gen_uint8() % (len - counter));
        _z_zbuf_t rv = _z_zbuf_view(&zbf, vs);

        printf("    View of %u bytes: ", vs);
        assert(_z_zbuf_capacity(&rv) == vs);
        assert(_z_zbuf_len(&rv) == vs);
        assert(_z_zbuf_space_left(&rv) == 0);

        printf("[");
        for (uint8_t i = 0; i < vs; i++) {
            uint8_t l = counter++;
            uint8_t r = _z_zbuf_read(&rv);
            printf(" %02x:%02x", l, r);
            assert(l == r);
        }
        printf(" ]\n");

        _z_zbuf_set_rpos(&zbf, _z_zbuf_get_rpos(&zbf) + vs);
    }

    _z_zbuf_clear(&zbf);
}

void wbuf_writable_readable(void) {
    size_t len = 128;
    _z_wbuf_t wbf = _z_wbuf_make(len, false);
    printf("\n>>> WBuf => Writable and Readable\n");

    size_t writable = _z_wbuf_space_left(&wbf);
    printf("    Writable: %zu\n", writable);
    assert(writable == len);

    size_t readable = _z_wbuf_len(&wbf);
    printf("    Readable: %zu\n", readable);
    assert(readable == 0);

    size_t written = 0;
    while (written < len) {
        size_t to_write = 1 + gen_size_t() % (len - written);
        for (size_t i = 0; i < to_write; i++) {
            _z_wbuf_write(&wbf, 0);
        }
        written = written + to_write;

        writable = _z_wbuf_space_left(&wbf);
        printf("    Writable: %zu\n", writable);
        assert(writable == len - written);

        readable = _z_wbuf_len(&wbf);
        printf("    Readable: %zu\n", readable);
        assert(readable == written);
    }

    _z_wbuf_clear(&wbf);
}

void wbuf_write_zbuf_read(void) {
    size_t len = 128;
    _z_wbuf_t wbf = gen_wbuf(len);
    printf("\n>>> WBuf => Write and Read\n");
    print_wbuf_overview(&wbf);
    printf("    Writing %zu bytes\n", len);
    for (size_t i = 0; i < len; i++) _z_wbuf_write(&wbf, (uint8_t)i % 255);

    printf("    IOSlices: %zu, RIdx: %zu, WIdx: %zu\n", _z_wbuf_len_iosli(&wbf), wbf._r_idx, wbf._w_idx);
    printf("    Written: %zu, Readable: %zu\n", len, _z_wbuf_len(&wbf));
    assert(_z_wbuf_len(&wbf) == len);

    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    assert(_z_zbuf_len(&zbf) == len);
    printf("    Reading %zu bytes\n", len);
    printf("    [");
    for (uint8_t i = 0; i < len; i++) {
        uint8_t l = (uint8_t)i % 255;
        uint8_t r = _z_zbuf_read(&zbf);
        printf(" %02x:%02x", l, r);
        assert(l == r);
    }
    printf("]\n");

    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

void wbuf_write_zbuf_read_bytes(void) {
    size_t len = 128;
    _z_wbuf_t wbf = gen_wbuf(len);
    printf("\n>>> WBuf => Write and Read bytes\n");
    print_wbuf_overview(&wbf);

    printf("    Writing %zu bytes\n", len);
    uint8_t *buf01 = (uint8_t *)z_malloc(len);
    for (size_t i = 0; i < len; i++) {
        buf01[i] = (uint8_t)i % 255;
    }
    _z_wbuf_write_bytes(&wbf, buf01, 0, len);

    printf("    IOSlices: %zu, RIdx: %zu, WIdx: %zu\n", _z_wbuf_len_iosli(&wbf), wbf._r_idx, wbf._w_idx);
    printf("    Written: %zu, Readable: %zu\n", len, _z_wbuf_len(&wbf));
    assert(_z_wbuf_len(&wbf) == len);

    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    printf("    [");
    for (size_t i = 0; i < len; i++) {
        uint8_t l = buf01[i];
        uint8_t r = _z_zbuf_read(&zbf);
        printf(" %02x:%02x", l, r);
        assert(l == r);
    }
    printf(" ]\n");

    z_free(buf01);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

void wbuf_put_zbuf_get(void) {
    size_t len = 128;
    _z_wbuf_t wbf = gen_wbuf(len);
    printf("\n>>> WBuf => Put and Get\n");
    print_wbuf_overview(&wbf);
    // Initialize to 0
    for (size_t i = 0; i < len; i++) {
        _z_wbuf_write(&wbf, 0);
    }
    printf("    IOSlices: %zu, RIdx: %zu, WIdx: %zu\n", _z_wbuf_len_iosli(&wbf), wbf._r_idx, wbf._w_idx);

    printf("    Putting %zu bytes\n", len);
    // Put data
    for (size_t i = 0; i < len; i++) {
        _z_wbuf_put(&wbf, (uint8_t)i % 255, i);
    }

    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    printf("    Getting %zu bytes\n", len);
    printf("    [");
    for (uint8_t i = 0; i < len; i++) {
        uint8_t l = (uint8_t)i % 255;
        uint8_t r = _z_zbuf_read(&zbf);
        printf(" %02x:%02x", l, r);
        assert(l == r);
    }
    printf(" ]\n");

    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

void wbuf_reusable_write_zbuf_read(void) {
    _z_wbuf_t wbf = gen_wbuf(128);
    for (int i = 1; i <= 10; i++) {
        size_t len = z_random_u8() % 128;
        printf("\n>>> WBuf => Write and Read round %d\n", i);
        print_wbuf_overview(&wbf);
        printf("    Writing %zu bytes\n", len);
        for (size_t z = 0; z < len; z++) {
            size_t prev_len = _z_wbuf_len(&wbf);
            assert(_z_wbuf_write(&wbf, (uint8_t)(z % 255)) == 0);
            assert(_z_wbuf_len(&wbf) == prev_len + 1);
        }

        printf("    IOSlices: %zu, RIdx: %zu, WIdx: %zu\n", _z_wbuf_len_iosli(&wbf), wbf._r_idx, wbf._w_idx);
        printf("    Written: %zu, Readable: %zu\n", len, _z_wbuf_len(&wbf));
        assert(_z_wbuf_len(&wbf) == len);

        _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
        assert(_z_zbuf_len(&zbf) == len);
        printf("    Reading %zu bytes\n", len);
        printf("    [");
        for (uint8_t j = 0; j < len; j++) {
            uint8_t l = j % (uint8_t)255;
            uint8_t r = _z_zbuf_read(&zbf);
            printf(" %02x:%02x", l, r);
            assert(l == r);
        }
        printf("]\n");
        _z_zbuf_clear(&zbf);
        _z_wbuf_reset(&wbf);
    }

    _z_wbuf_clear(&wbf);
}

void wbuf_set_pos_wbuf_get_pos(void) {
    size_t len = 128;
    _z_wbuf_t wbf = gen_wbuf(len);
    printf("\n>>> WBuf => SetPos and GetPos\n");
    print_wbuf_overview(&wbf);
    // Initialize to 0
    for (size_t i = 0; i < len; i++) {
        _z_wbuf_write(&wbf, 0);
    }
    assert(_z_wbuf_get_rpos(&wbf) == 0);
    assert(_z_wbuf_get_wpos(&wbf) == len);
    printf("    IOSlices: %zu, RIdx: %zu, WIdx: %zu\n", _z_wbuf_len_iosli(&wbf), wbf._r_idx, wbf._w_idx);

    for (size_t i = 0; i < 10; i++) {
        _z_wbuf_reset(&wbf);
        assert(_z_wbuf_get_rpos(&wbf) == 0);
        assert(_z_wbuf_get_wpos(&wbf) == 0);

        size_t lw_pos = gen_size_t() % (len + 1);
        printf("    Setting WPos: %zu\n", lw_pos);
        _z_wbuf_set_wpos(&wbf, lw_pos);

        size_t rw_pos = _z_wbuf_get_wpos(&wbf);
        printf("    Getting WPos: %zu\n", rw_pos);
        assert(lw_pos == rw_pos);

        size_t lr_pos = gen_size_t() % (lw_pos + 1);
        printf("    Setting RPos: %zu\n", lr_pos);
        _z_wbuf_set_rpos(&wbf, lr_pos);

        size_t rr_pos = _z_wbuf_get_rpos(&wbf);
        printf("    Getting RPos: %zu\n", rr_pos);
        assert(lr_pos == rr_pos);
    }

    _z_wbuf_clear(&wbf);
}

void wbuf_add_iosli(void) {
    uint8_t len = 16;
    _z_wbuf_t wbf = _z_wbuf_make(len, true);
    printf("\n>>> WBuf => Add IOSli\n");
    print_wbuf_overview(&wbf);

    uint8_t written = 0;
    uint8_t counter = 0;
    while (written < 255) {
        uint8_t remaining = 255 - written;
        uint8_t range = remaining < len ? remaining : len;
        uint8_t to_write = 1 + gen_uint8() % range;
        printf("    Writing %u bytes\n", to_write);
        for (uint8_t i = 0; i < to_write; i++) {
            _z_wbuf_write(&wbf, counter);
            counter++;
        }
        written = written + to_write;
    }

    printf("    IOSlices: %zu, RIdx: %zu, WIdx: %zu\n", _z_wbuf_len_iosli(&wbf), wbf._r_idx, wbf._w_idx);
    for (size_t i = 0; i < _z_wbuf_len_iosli(&wbf); i++) {
        _z_iosli_t *ios = _z_wbuf_get_iosli(&wbf, i);
        printf("    Idx: %zu => ", i);
        print_iosli(ios);
        printf("\n");
    }

    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    printf("    [");
    for (uint8_t i = 0; i < counter; i++) {
        uint8_t l = i;
        uint8_t r = _z_zbuf_read(&zbf);
        printf(" %02x:%02x", l, r);
        assert(l == r);
    }
    printf(" ]\n");

    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*=============================*/
/*            Main             */
/*=============================*/
int main(void) {
    for (unsigned int i = 0; i < RUNS; i++) {
        printf("\n\n== RUN %u\n", i);
        // IOsli
        iosli_writable_readable();
        // ZBuf
        zbuf_writable_readable();
        zbuf_compact();
        zbuf_view();
        // WBuf
        wbuf_writable_readable();
        wbuf_set_pos_wbuf_get_pos();
        wbuf_add_iosli();
        // WBuf and ZBuf
        wbuf_write_zbuf_read();
        wbuf_write_zbuf_read_bytes();
        wbuf_put_zbuf_get();

        // Reusable WBuf
        wbuf_reusable_write_zbuf_read();
    }
}
