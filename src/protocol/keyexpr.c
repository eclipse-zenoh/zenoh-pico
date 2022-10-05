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

#include "zenoh-pico/protocol/keyexpr.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "zenoh-pico/utils/pointers.h"

/*------------------ Canonize helpers ------------------*/
zp_keyexpr_canon_status_t __zp_canon_prefix(const char *start, size_t *len) {
    _Bool in_big_wild = false;
    char const *chunk_start = start;
    const char *end = _z_cptr_char_offset(start, *len);
    char const *next_slash;

    do {
        next_slash = strchr(chunk_start, '/');
        const char *chunk_end = next_slash ? next_slash : end;
        size_t chunk_len = _z_ptr_char_diff(chunk_end, chunk_start);
        switch (chunk_len) {
            case 0:
                return Z_KEYEXPR_CANON_EMPTY_CHUNK;

            case 1:
                if (in_big_wild && (chunk_start[0] == '*')) {
                    *len = _z_ptr_char_diff(chunk_start, start) - (size_t)3;
                    return Z_KEYEXPR_CANON_SINGLE_STAR_AFTER_DOUBLE_STAR;
                }
                chunk_start = _z_cptr_char_offset(chunk_end, 1);
                continue;
                break;  // It MUST never be here. Required by MISRA 16.3 rule

            case 2:
                if (chunk_start[1] == '*') {
                    switch (chunk_start[0]) {
                        case (char)'$':
                            *len = _z_ptr_char_diff(chunk_start, start);
                            return Z_KEYEXPR_CANON_LONE_DOLLAR_STAR;

                        case (char)'*':
                            if (in_big_wild) {
                                *len = _z_ptr_char_diff(chunk_start, start) - (size_t)3;
                                return Z_KEYEXPR_CANON_DOUBLE_STAR_AFTER_DOUBLE_STAR;
                            } else {
                                chunk_start = _z_cptr_char_offset(chunk_end, 1);
                                in_big_wild = true;
                                continue;
                            }
                            break;  // It MUST never be here. Required by MISRA 16.3 rule
                        default:
                            break;
                    }
                }
                break;
            default:
                break;
        }

        unsigned char in_dollar = 0;
        for (char const *c = chunk_start; c < chunk_end; c = _z_cptr_char_offset(c, 1)) {
            switch (c[0]) {
                case '#':
                case '?':
                    return Z_KEYEXPR_CANON_CONTAINS_SHARP_OR_QMARK;

                case '$':
                    if (in_dollar != (unsigned char)0) {
                        return Z_KEYEXPR_CANON_DOLLAR_AFTER_DOLLAR_OR_STAR;
                    }
                    in_dollar += (unsigned char)1;
                    break;

                case '*':
                    if (in_dollar != (unsigned char)1) {
                        return Z_KEYEXPR_CANON_STARS_IN_CHUNK;
                    }
                    in_dollar += (unsigned char)2;
                    break;

                default:
                    if (in_dollar == (unsigned char)1) {
                        return Z_KEYEXPR_CANON_CONTAINS_UNBOUND_DOLLAR;
                    }
                    in_dollar = 0;
                    break;
            }
        }

        if (in_dollar == (unsigned char)1) {
            return Z_KEYEXPR_CANON_CONTAINS_UNBOUND_DOLLAR;
        }

        chunk_start = _z_cptr_char_offset(chunk_end, 1);
        in_big_wild = false;
    } while (chunk_start < end);

    return (chunk_start > end) ? Z_KEYEXPR_CANON_SUCCESS : Z_KEYEXPR_CANON_EMPTY_CHUNK;
}

size_t __zp_starts_with(const char *s, const char *needle) {
    size_t i = 0;
    for (; needle[i]; i++) {
        if (s[i] != needle[i]) {
            return 0;
        }
    }

    return i;
}
void __zp_singleify(char *start, size_t *len, const char *needle) {
    const char *end = _z_cptr_char_offset(start, *len);
    _Bool right_after_needle = false;
    char *reader = start;
    while (reader < end) {
        size_t pos = __zp_starts_with(reader, needle);
        if (pos != (size_t)0) {
            if (right_after_needle) {
                break;
            }
            right_after_needle = true;
            reader = _z_ptr_char_offset(reader, pos);
        } else {
            right_after_needle = false;
            reader = _z_ptr_char_offset(reader, 1);
        }
    }

    char *writer = reader;
    while (reader < end) {
        size_t pos = __zp_starts_with(reader, needle);
        if (pos != (size_t)0) {
            if (!right_after_needle) {
                for (size_t i = 0; i < pos; i++) {
                    writer[i] = reader[i];
                }
                writer = _z_ptr_char_offset(writer, pos);
            }
            right_after_needle = true;
            reader = _z_ptr_char_offset(reader, pos);
        } else {
            right_after_needle = false;
            *writer = *reader;
            writer = _z_ptr_char_offset(writer, 1);
            reader = _z_ptr_char_offset(reader, 1);
        }
    }
    *len = _z_ptr_char_diff(writer, start);
}

void __zp_ke_write_chunk(char **writer, const char *chunk, size_t len, const char *write_start) {
    if (*writer != write_start) {
        writer[0][0] = '/';
        writer[0] = _z_ptr_char_offset(writer[0], 1);
    }

    (void)memcpy(*writer, chunk, len);
    writer[0] = _z_ptr_char_offset(writer[0], len);
}

/*------------------ Inclusion helpers ------------------*/
_Bool __zp_ke_includes_stardsl_chunk(char const *lstart, const char *lend, char const *rstart, const char *rend) {
    while ((lstart < lend) && (rstart < rend)) {
        char l = lstart[0];
        lstart = _z_cptr_char_offset(lstart, 1);
        char r = rstart[0];
        rstart = _z_cptr_char_offset(rstart, 1);
        if (l == '$') {
            lstart = _z_cptr_char_offset(lstart, 1);
            if (lstart == lend) {
                return true;
            }
            return __zp_ke_includes_stardsl_chunk(lstart, lend, _z_cptr_char_offset(rstart, -1), rend) ||
                   __zp_ke_includes_stardsl_chunk(_z_cptr_char_offset(lstart, -2), lend, rstart, rend);
        } else if (l != r) {
            return false;
        } else {
            // Do nothing. Continue...
            // Required to be compliant with MISRA 15.7 rule
        }
    }

    return ((lstart == lend) && (rstart == rend)) || ((_z_ptr_char_diff(lend, lstart) == 2) && (lstart[0] == '$'));
}

_Bool __zp_ke_includes_stardsl(char const *lstart, const size_t llen, char const *rstart, const size_t rlen) {
    size_t lclen;
    _Bool streq;
    const char *lend = _z_cptr_char_offset(lstart, llen);
    const char *rend = _z_cptr_char_offset(rstart, rlen);

    while (true) {
        const char *lns = strchr(lstart, '/');
        const char *lcend = lns ? lns : lend;
        const char *rns = strchr(rstart, '/');
        const char *rcend = rns ? rns : rend;
        char lwildness = 0;
        char rwildness = 0;

        if (*lstart == '*') {
            lwildness = _z_ptr_char_diff(lcend, lstart);
        }

        if (*rstart == '*') {
            rwildness = _z_ptr_char_diff(rcend, rstart);
        }

        if (rwildness > lwildness) {
            return false;
        }

        switch (lwildness) {
            case 2:
                return !lns ||
                       __zp_ke_includes_stardsl(_z_cptr_char_offset(lns, 1),
                                                _z_ptr_char_diff(lend, _z_cptr_char_offset(lns, 1)), rstart,
                                                _z_ptr_char_diff(rend, rstart)) ||
                       (rns &&
                        __zp_ke_includes_stardsl(lstart, _z_ptr_char_diff(lend, lstart), _z_cptr_char_offset(rns, 1),
                                                 _z_ptr_char_diff(rend, _z_cptr_char_offset(rns, 1))));

            case 1:
                break;  // if either chunk is a small wild, yet neither is a big wild, just skip this chunk inspection

            default:
                lclen = _z_ptr_char_diff(lcend, lstart);
                streq = (lclen == _z_ptr_char_diff(rcend, rstart)) && (strncmp(lstart, rstart, lclen) == 0);
                if (!streq && !__zp_ke_includes_stardsl_chunk(lstart, lcend, rstart, rcend)) {
                    return false;
                }
                break;
        }

        if (!lns) {
            return !rns;
        } else if (!rns) {
            return !lns || ((_z_ptr_char_diff(lend, lns) == 3) && (lns[1] == '*'));
        } else {
            // Do nothing. Continue...
            // Required to be compliant with MISRA 15.7 rule
        }

        lstart = _z_cptr_char_offset(lns, 1);
        rstart = _z_cptr_char_offset(rns, 1);
    }
}

_Bool __zp_ke_includes_nodsl(char const *lstart, const size_t llen, char const *rstart, const size_t rlen) {
    const char *lend = _z_cptr_char_offset(lstart, llen);
    const char *rend = _z_cptr_char_offset(rstart, rlen);
    while (true) {
        const char *lns = strchr(lstart, '/');
        const char *lcend = lns ? lns : lend;
        const char *rns = strchr(rstart, '/');
        const char *rcend = rns ? rns : rend;
        char lwildness = 0;
        char rwildness = 0;
        if (*lstart == '*') {
            lwildness = _z_ptr_char_diff(lcend, lstart);
        }

        if (*rstart == '*') {
            rwildness = _z_ptr_char_diff(rcend, rstart);
        }

        if (rwildness > lwildness) {
            return false;
        }

        switch (lwildness) {
            case 2:
                return !lns ||
                       __zp_ke_includes_nodsl(_z_cptr_char_offset(lns, 1),
                                              _z_ptr_char_diff(lend, _z_cptr_char_offset(lns, 1)), rstart,
                                              _z_ptr_char_diff(rend, rstart)) ||
                       (rns &&
                        __zp_ke_includes_nodsl(lstart, _z_ptr_char_diff(lend, lstart), _z_cptr_char_offset(rns, 1),
                                               _z_ptr_char_diff(rend, _z_cptr_char_offset(rns, 1))));

            case 1:
                break;  // if either chunk is a small wild, yet neither is a big wild, just skip this chunk inspection

            default:
                if ((_z_ptr_char_diff(lcend, lstart) != _z_ptr_char_diff(rcend, rstart)) ||
                    strncmp(lstart, rstart, _z_ptr_char_diff(lcend, lstart))) {
                    return false;
                }
                break;
        }

        if (!lns) {
            return !rns;
        } else if (!rns) {
            return !lns || ((_z_ptr_char_diff(lend, lns) == 3) && (lns[1] == '*'));
        } else {
            // Do nothing. Continue...
            // Required to be compliant with MISRA 15.7 rule
        }

        lstart = _z_cptr_char_offset(lns, 1);
        rstart = _z_cptr_char_offset(rns, 1);
    }
}

/*------------------ Intersection helpers ------------------*/
_Bool __zp_ke_intersects_stardsl_chunk(char const *lstart, const char *lend, char const *rstart, const char *rend) {
    while ((lstart < lend) && (rstart < rend)) {
        char l = *lstart;
        lstart = _z_cptr_char_offset(lstart, 1);
        char r = *rstart;
        rstart = _z_cptr_char_offset(rstart, 1);
        if (l == '$') {
            lstart = _z_cptr_char_offset(lstart, 1);
            if (lstart == lend) {
                return true;
            }

            return __zp_ke_intersects_stardsl_chunk(lstart, lend, _z_cptr_char_offset(rstart, -1), rend) ||
                   __zp_ke_intersects_stardsl_chunk(_z_cptr_char_offset(lstart, -2), lend, rstart, rend);
        } else if (r == '$') {
            rstart = _z_cptr_char_offset(rstart, 1);
            if (rstart == rend) {
                return true;
            }

            return __zp_ke_intersects_stardsl_chunk(_z_cptr_char_offset(lstart, -1), lend, rstart, rend) ||
                   __zp_ke_intersects_stardsl_chunk(lstart, lend, _z_cptr_char_offset(rstart, -2), rend);
        } else if (l != r) {
            return false;
        } else {
            // Do nothing. Continue...
            // Required to be compliant with MISRA 15.7 rule
        }
    }

    return ((lstart == lend) && (rstart == rend)) || ((_z_ptr_char_diff(lend, lstart) == 2) && (lstart[0] == '$')) ||
           (((_z_ptr_char_diff(rend, rstart)) == 2) && (rstart[0] == '$'));
}

_Bool __zp_ke_intersects_stardsl(char const *lstart, const size_t llen, char const *rstart, const size_t rlen) {
    size_t lclen;
    _Bool streq;
    const char *lend = _z_cptr_char_offset(lstart, llen);
    const char *rend = _z_cptr_char_offset(rstart, rlen);

    while (true) {
        const char *lns = strchr(lstart, '/');
        const char *lcend = lns ? lns : lend;
        const char *rns = strchr(rstart, '/');
        const char *rcend = rns ? rns : rend;
        unsigned char lwildness = 0;
        unsigned char rwildness = 0;
        if (*lstart == '*') {
            lwildness = _z_ptr_char_diff(lcend, lstart);
        }
        if (*rstart == '*') {
            rwildness = _z_ptr_char_diff(rcend, rstart);
        }
        switch (lwildness | rwildness) {
            case 2:
            case 3:
                if (lwildness == (unsigned char)2) {
                    return !lns ||
                           __zp_ke_intersects_stardsl(_z_cptr_char_offset(lns, 1),
                                                      _z_ptr_char_diff(lend, _z_cptr_char_offset(lns, 1)), rstart,
                                                      _z_ptr_char_diff(rend, rstart)) ||
                           (rns && __zp_ke_intersects_stardsl(lstart, _z_ptr_char_diff(lend, lstart),
                                                              _z_cptr_char_offset(rns, 1),
                                                              _z_ptr_char_diff(rend, _z_cptr_char_offset(rns, 1))));
                } else {
                    return !rns || __zp_ke_intersects_stardsl(
                                       lstart, _z_ptr_char_diff(lend, lstart), _z_cptr_char_offset(rns, 1),
                                       _z_ptr_char_diff(rend, _z_cptr_char_offset(rns, 1)) ||
                                           (lns && __zp_ke_intersects_stardsl(
                                                       _z_cptr_char_offset(lns, 1),
                                                       _z_ptr_char_diff(lend, _z_cptr_char_offset(lns, 1)), rstart,
                                                       _z_ptr_char_diff(rend, rstart))));
                }
                break;

            case 1:
                break;  // if either chunk is a small wild, yet neither is a big wild, just skip this chunk inspection

            default:
                lclen = _z_ptr_char_diff(lcend, lstart);
                streq = (lclen == _z_ptr_char_diff(rcend, rstart)) && (strncmp(lstart, rstart, lclen) == 0);
                if (!(streq || __zp_ke_intersects_stardsl_chunk(lstart, lcend, rstart, rcend))) {
                    return false;
                }
                break;
        }

        if (!lns) {
            return !rns || ((_z_ptr_char_diff(rend, rns) == 3) && (rns[1] == '*'));
        } else if (!rns) {
            return !lns || ((_z_ptr_char_diff(lend, lns) == 3) && (lns[1] == '*'));
        } else {
            // Do nothing. Continue...
            // Required to be compliant with MISRA 15.7 rule
        }

        lstart = _z_cptr_char_offset(lns, 1);
        rstart = _z_cptr_char_offset(rns, 1);
    }
}

_Bool __zp_ke_intersects_nodsl(char const *lstart, const size_t llen, char const *rstart, const size_t rlen) {
    const char *lend = _z_cptr_char_offset(lstart, llen);
    const char *rend = _z_cptr_char_offset(rstart, rlen);
    while (true) {
        const char *lns = strchr(lstart, '/');
        const char *lcend = lns ? lns : lend;
        const char *rns = strchr(rstart, '/');
        const char *rcend = rns ? rns : rend;
        unsigned char lwildness = 0;
        unsigned char rwildness = 0;

        if (*lstart == '*') {
            lwildness = _z_ptr_char_diff(lcend, lstart);
        }

        if (*rstart == '*') {
            rwildness = _z_ptr_char_diff(rcend, rstart);
        }

        switch (lwildness | rwildness) {
            case 2:
            case 3:
                if (lwildness == (unsigned char)2) {
                    return !lns ||
                           __zp_ke_intersects_nodsl(_z_cptr_char_offset(lns, 1),
                                                    _z_ptr_char_diff(lend, _z_cptr_char_offset(lns, 1)), rstart,
                                                    _z_ptr_char_diff(rend, rstart)) ||
                           (rns && __zp_ke_intersects_nodsl(lstart, _z_ptr_char_diff(lend, lstart),
                                                            _z_cptr_char_offset(rns, 1),
                                                            _z_ptr_char_diff(rend, _z_cptr_char_offset(rns, 1))));
                } else {
                    return !rns ||
                           __zp_ke_intersects_nodsl(
                               lstart, _z_ptr_char_diff(lend, lstart), _z_cptr_char_offset(rns, 1),
                               _z_ptr_char_diff(rend, _z_cptr_char_offset(rns, 1)) ||
                                   (lns && __zp_ke_intersects_nodsl(_z_cptr_char_offset(lns, 1),
                                                                    _z_ptr_char_diff(lend, _z_cptr_char_offset(lns, 1)),
                                                                    rstart, _z_ptr_char_diff(rend, rstart))));
                }
                break;

            case 1:
                break;  // if either chunk is a small wild, yet neither is a big wild, just skip this chunk inspection

            default:
                if ((_z_ptr_char_diff(lcend, lstart) != _z_ptr_char_diff(rcend, rstart)) ||
                    strncmp(lstart, rstart, _z_ptr_char_diff(lcend, lstart))) {
                    return false;
                }
                break;
        }

        if (!lns) {
            return !rns || ((_z_ptr_char_diff(rend, rns) == 3) && (rns[1] == '*'));
        } else if (!rns) {
            return !lns || ((_z_ptr_char_diff(lend, lns) == 3) && (lns[1] == '*'));
        } else {
            // Do nothing. Continue...
            // Required to be compliant with MISRA 15.7 rule
        }

        lstart = _z_cptr_char_offset(lns, 1);
        rstart = _z_cptr_char_offset(rns, 1);
    }
}

/*------------------ Zenoh-Core helpers ------------------*/
_Bool _z_keyexpr_includes(const char *lstart, const size_t llen, const char *rstart, const size_t rlen) {
    _Bool streq = (llen == rlen) && (strncmp(lstart, rstart, llen) == 0);
    if (streq) {
        return true;
    }

    int contains = 0;  // STAR: 1, DSL: 2
    char const *end = _z_cptr_char_offset(lstart, llen);
    for (char const *c = lstart; c < end; c = _z_cptr_char_offset(c, 1)) {
        if (*c == '*') {
            contains |= 1;
        }
        if (*c == '$') {
            contains |= 3;
            break;
        }
    }

    end = _z_cptr_char_offset(rstart, rlen);
    for (char const *c = rstart; (contains < 3) && (c < end); c = _z_cptr_char_offset(c, 1)) {
        if (*c == '*') {
            contains |= 1;
        }
        if (*c == '$') {
            contains |= 3;
        }
    }

    switch (contains) {
        case 0:
            return false;

        case 1:
            return __zp_ke_includes_nodsl(lstart, llen, rstart, rlen);

        default:
            return __zp_ke_includes_stardsl(lstart, llen, rstart, rlen);
    }
}

_Bool _z_keyexpr_intersects(const char *lstart, const size_t llen, const char *rstart, const size_t rlen) {
    _Bool streq = (llen == rlen) && (strncmp(lstart, rstart, llen) == 0);
    if (streq) {
        return true;
    }

    int contains = 0;  // STAR: 1, DSL: 2
    char const *end = _z_cptr_char_offset(lstart, llen);
    for (char const *c = lstart; c < end; c = _z_cptr_char_offset(c, 1)) {
        if (*c == '*') {
            contains |= 1;
        }

        if (*c == '$') {
            contains |= 3;
            break;
        }
    }

    end = _z_cptr_char_offset(rstart, rlen);
    for (char const *c = rstart; (contains < 3) && (c < end); c = _z_cptr_char_offset(c, 1)) {
        if (*c == '*') {
            contains |= 1;
        }
        if (*c == '$') {
            contains |= 3;
        }
    }

    switch (contains) {
        case 0:
            return false;

        case 1:
            return __zp_ke_intersects_nodsl(lstart, llen, rstart, rlen);

        default:
            return __zp_ke_intersects_stardsl(lstart, llen, rstart, rlen);
    }
}

zp_keyexpr_canon_status_t _z_keyexpr_canonize(char *start, size_t *len) {
    __zp_singleify(start, len, "$*");
    size_t canon_len = *len;
    zp_keyexpr_canon_status_t prefix_canon_state = __zp_canon_prefix(start, &canon_len);
    switch (prefix_canon_state) {
        case Z_KEYEXPR_CANON_LONE_DOLLAR_STAR:
        case Z_KEYEXPR_CANON_SINGLE_STAR_AFTER_DOUBLE_STAR:
        case Z_KEYEXPR_CANON_DOUBLE_STAR_AFTER_DOUBLE_STAR:
            break;  // Canonization only handles these errors, so finding any other error (or a success) means immediate
                    // return
        default:
            return prefix_canon_state;
    }

    const char *end = _z_cptr_char_offset(start, *len);
    char *reader = _z_ptr_char_offset(start, canon_len);
    const char *write_start = reader;
    char *writer = reader;
    char *next_slash = strchr(reader, '/');
    char const *chunk_end = next_slash ? next_slash : end;

    _Bool in_big_wild = false;
    if ((_z_ptr_char_diff(chunk_end, reader) == 2) && (reader[1] == '*')) {
        switch (*reader) {
            case '*':
                in_big_wild = true;
                break;

            case '$':  // if the chunk is $*, replace it with *.
                writer[0] = '*';
                writer = _z_ptr_char_offset(writer, 1);
                break;

            default:
                assert(false);  // anything before "$*" or "**" must be part of the canon prefix
                break;
        }
    } else {
        assert(false);  // anything before "$*" or "**" must be part of the canon prefix
    }

    while (next_slash != NULL) {
        reader = _z_ptr_char_offset(next_slash, 1);
        next_slash = strchr(reader, '/');
        chunk_end = next_slash ? next_slash : end;
        switch (_z_ptr_char_diff(chunk_end, reader)) {
            case 0:
                return Z_KEYEXPR_CANON_EMPTY_CHUNK;

            case 1:
                __zp_ke_write_chunk(&writer, "*", 1, write_start);
                continue;
                break;  // It MUST never be here. Required by MISRA 16.3 rule

            case 2:
                if (reader[1] == '*') {
                    if (reader[0] == '$') {
                        __zp_ke_write_chunk(&writer, "*", 1, write_start);
                        continue;
                    } else if (reader[0] == (char)'*') {
                        in_big_wild = true;
                        continue;
                    } else {
                        // Do nothing.
                        // Required to be compliant with MISRA 15.7 rule
                    }
                }
                break;

            default:
                break;
        }

        unsigned char in_dollar = 0;
        for (char const *c = reader; c < end; c = _z_cptr_char_offset(c, 1)) {
            switch (*c) {
                case '#':
                case '?':
                    return Z_KEYEXPR_CANON_CONTAINS_SHARP_OR_QMARK;

                case '$':
                    if (in_dollar != (unsigned char)0) {
                        return Z_KEYEXPR_CANON_DOLLAR_AFTER_DOLLAR_OR_STAR;
                    }
                    in_dollar += (unsigned char)1;
                    break;

                case '*':
                    if (in_dollar != (unsigned char)1) {
                        return Z_KEYEXPR_CANON_STARS_IN_CHUNK;
                    }
                    in_dollar += (unsigned char)2;
                    break;

                default:
                    if (in_dollar == (unsigned char)1) {
                        return Z_KEYEXPR_CANON_CONTAINS_UNBOUND_DOLLAR;
                    }
                    in_dollar = 0;
                    break;
            }
        }

        if (in_dollar == (unsigned char)1) {
            return Z_KEYEXPR_CANON_CONTAINS_UNBOUND_DOLLAR;
        }

        if (in_big_wild) {
            __zp_ke_write_chunk(&writer, "**", 2, write_start);
        }

        __zp_ke_write_chunk(&writer, reader, _z_ptr_char_diff(chunk_end, reader), write_start);
        in_big_wild = false;
    }

    if (in_big_wild) {
        __zp_ke_write_chunk(&writer, "**", 2, write_start);
    }
    *len = _z_ptr_char_diff(writer, start);

    return Z_KEYEXPR_CANON_SUCCESS;
}

zp_keyexpr_canon_status_t _z_keyexpr_is_canon(const char *start, size_t len) { return __zp_canon_prefix(start, &len); }
