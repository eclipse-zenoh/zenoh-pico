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

/*------------------ Canonize helpers ------------------*/
zp_keyexpr_canon_status_t __zp_canon_prefix(const char *start, size_t *len) {
    _Bool in_big_wild = false;
    char const *chunk_start = start;
    const char *end = start + *len;
    char const *next_slash;

    do {
        next_slash = memchr(chunk_start, '/', end - chunk_start);
        const char *chunk_end = next_slash ? next_slash : end;
        size_t chunk_len = chunk_end - chunk_start;
        switch (chunk_len) {
            case 0:
                return Z_KEYEXPR_CANON_EMPTY_CHUNK;

            case 1:
                if (in_big_wild && *chunk_start == '*') {
                    *len = chunk_start - start - 3;
                    return Z_KEYEXPR_CANON_SINGLE_STAR_AFTER_DOUBLE_STAR;
                }
                chunk_start = chunk_end + 1;
                continue;

            case 2:
                if (chunk_start[1] == '*') {
                    switch (*chunk_start) {
                        case '$':
                            *len = chunk_start - start;
                            return Z_KEYEXPR_CANON_LONE_DOLLAR_STAR;

                        case '*':
                            if (in_big_wild) {
                                *len = chunk_start - start - 3;
                                return Z_KEYEXPR_CANON_DOUBLE_STAR_AFTER_DOUBLE_STAR;
                            } else {
                                chunk_start = chunk_end + 1;
                                in_big_wild = true;
                                continue;
                            }
                    }
                }
                break;
        }

        unsigned char in_dollar = 0;
        for (char const *c = chunk_start; c < chunk_end; c++) {
            switch (*c) {
                case '#':
                case '?':
                    return Z_KEYEXPR_CANON_CONTAINS_SHARP_OR_QMARK;

                case '$':
                    if (in_dollar) {
                        return Z_KEYEXPR_CANON_DOLLAR_AFTER_DOLLAR_OR_STAR;
                    }
                    in_dollar += 1;
                    break;

                case '*':
                    if (in_dollar != 1) {
                        return Z_KEYEXPR_CANON_STARS_IN_CHUNK;
                    }
                    in_dollar += 2;
                    break;

                default:
                    if (in_dollar == 1) {
                        return Z_KEYEXPR_CANON_CONTAINS_UNBOUND_DOLLAR;
                    }
                    in_dollar = 0;
            }
        }

        if (in_dollar == 1) {
            return Z_KEYEXPR_CANON_CONTAINS_UNBOUND_DOLLAR;
        }

        chunk_start = chunk_end + 1;
        in_big_wild = false;
    } while (chunk_start < end);

    return chunk_start > end ? Z_KEYEXPR_CANON_SUCCESS : Z_KEYEXPR_CANON_EMPTY_CHUNK;
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
    const char *end = start + *len;
    _Bool right_after_needle = false;
    char *reader = start;
    while (reader < end) {
        size_t len = __zp_starts_with(reader, needle);
        if (len) {
            if (right_after_needle) {
                break;
            }
            right_after_needle = true;
            reader += len;
        } else {
            right_after_needle = false;
            reader++;
        }
    }

    char *writer = reader;
    while (reader < end) {
        size_t len = __zp_starts_with(reader, needle);
        if (len) {
            if (!right_after_needle) {
                for (size_t i = 0; i < len; i++) {
                    writer[i] = reader[i];
                }
                writer += len;
            }
            right_after_needle = true;
            reader += len;
        } else {
            right_after_needle = false;
            *writer++ = *reader++;
        }
    }
    *len = writer - start;
}

void __zp_ke_write_chunk(char **writer, const char *chunk, size_t len, const char *write_start) {
    if (*writer != write_start) {
        *(*writer)++ = '/';
    }

    memcpy(*writer, chunk, len);
    *writer += len;
}

/*------------------ Inclusion helpers ------------------*/
_Bool __zp_ke_includes_stardsl_chunk(char const *lstart, const char *lend, char const *rstart, const char *rend) {
    while (lstart < lend && rstart < rend) {
        char l = *lstart++;
        char r = *rstart++;
        if (l == '$') {
            if (++lstart == lend) {
                return true;
            }
            return __zp_ke_includes_stardsl_chunk(lstart, lend, rstart - 1, rend) ||
                   __zp_ke_includes_stardsl_chunk(lstart - 2, lend, rstart, rend);
        } else if (l != r) {
            return false;
        }
    }

    return (lstart == lend && rstart == rend) || (lend - lstart == 2 && lstart[0] == '$');
}

_Bool __zp_ke_includes_stardsl(char const *lstart, const size_t llen, char const *rstart, const size_t rlen) {
    size_t lclen;
    _Bool streq;
    const char *lend = lstart + llen;
    const char *rend = rstart + rlen;

    for (;;) {
        const char *lns = memchr(lstart, '/', lend - lstart);
        const char *lcend = lns ? lns : lend;
        const char *rns = memchr(rstart, '/', rend - rstart);
        const char *rcend = rns ? rns : rend;
        char lwildness = 0;
        char rwildness = 0;

        if (*lstart == '*') {
            lwildness = lcend - lstart;
        }

        if (*rstart == '*') {
            rwildness = rcend - rstart;
        }

        if (rwildness > lwildness) {
            return false;
        }

        switch (lwildness) {
            case 2:
                return !lns || __zp_ke_includes_stardsl(lns + 1, lend - (lns + 1), rstart, rend - rstart) ||
                       (rns && __zp_ke_includes_stardsl(lstart, lend - lstart, rns + 1, rend - (rns + 1)));

            case 1:
                break;  // if either chunk is a small wild, yet neither is a big wild, just skip this chunk inspection

            default:
                lclen = lcend - lstart;
                streq = (lclen == (size_t)(rcend - rstart)) && (strncmp(lstart, rstart, lclen) == 0);
                if (!streq && !__zp_ke_includes_stardsl_chunk(lstart, lcend, rstart, rcend)) {
                    return false;
                }
        }

        if (!lns) {
            return !rns;
        } else if (!rns) {
            return !lns || (lend - lns == 3 && lns[1] == '*');
        }

        lstart = lns + 1;
        rstart = rns + 1;
    }
}

_Bool __zp_ke_includes_nodsl(char const *lstart, const size_t llen, char const *rstart, const size_t rlen) {
    const char *lend = lstart + llen;
    const char *rend = rstart + rlen;
    for (;;) {
        const char *lns = memchr(lstart, '/', lend - lstart);
        const char *lcend = lns ? lns : lend;
        const char *rns = memchr(rstart, '/', rend - rstart);
        const char *rcend = rns ? rns : rend;
        char lwildness = 0;
        char rwildness = 0;
        if (*lstart == '*') {
            lwildness = lcend - lstart;
        }

        if (*rstart == '*') {
            rwildness = rcend - rstart;
        }

        if (rwildness > lwildness) {
            return false;
        }

        switch (lwildness) {
            case 2:
                return !lns || __zp_ke_includes_nodsl(lns + 1, lend - (lns + 1), rstart, rend - rstart) ||
                       (rns && __zp_ke_includes_nodsl(lstart, lend - lstart, rns + 1, rend - (rns + 1)));

            case 1:
                break;  // if either chunk is a small wild, yet neither is a big wild, just skip this chunk inspection

            default:
                if ((lcend - lstart != rcend - rstart) || strncmp(lstart, rstart, lcend - lstart)) {
                    return false;
                }
        }

        if (!lns) {
            return !rns;
        } else if (!rns) {
            return !lns || (lend - lns == 3 && lns[1] == '*');
        }

        lstart = lns + 1;
        rstart = rns + 1;
    }
}

/*------------------ Intersection helpers ------------------*/
_Bool __zp_ke_intersects_stardsl_chunk(char const *lstart, const char *lend, char const *rstart, const char *rend) {
    while (lstart < lend && rstart < rend) {
        char l = *lstart++;
        char r = *rstart++;
        if (l == '$') {
            if (++lstart == lend) {
                return true;
            }

            return __zp_ke_intersects_stardsl_chunk(lstart, lend, rstart - 1, rend) ||
                   __zp_ke_intersects_stardsl_chunk(lstart - 2, lend, rstart, rend);
        } else if (r == '$') {
            if (++rstart == rend) {
                return true;
            }

            return __zp_ke_intersects_stardsl_chunk(lstart - 1, lend, rstart, rend) ||
                   __zp_ke_intersects_stardsl_chunk(lstart, lend, rstart - 2, rend);
        } else if (l != r) {
            return false;
        }
    }

    return (lstart == lend && rstart == rend) || (lend - lstart == 2 && lstart[0] == '$') ||
           (rend - rstart == 2 && rstart[0] == '$');
}

_Bool __zp_ke_intersects_stardsl(char const *lstart, const size_t llen, char const *rstart, const size_t rlen) {
    size_t lclen;
    _Bool streq;
    const char *lend = lstart + llen;
    const char *rend = rstart + rlen;

    for (;;) {
        const char *lns = memchr(lstart, '/', lend - lstart);
        const char *lcend = lns ? lns : lend;
        const char *rns = memchr(rstart, '/', rend - rstart);
        const char *rcend = rns ? rns : rend;
        char lwildness = 0;
        char rwildness = 0;
        if (*lstart == '*') {
            lwildness = lcend - lstart;
        }
        if (*rstart == '*') {
            rwildness = rcend - rstart;
        }
        switch (lwildness | rwildness) {
            case 2:
            case 3:
                if (lwildness == 2) {
                    return !lns || __zp_ke_intersects_stardsl(lns + 1, lend - (lns + 1), rstart, rend - rstart) ||
                           (rns && __zp_ke_intersects_stardsl(lstart, lend - lstart, rns + 1, rend - (rns + 1)));
                } else {
                    return !rns || __zp_ke_intersects_stardsl(lstart, lend - lstart, rns + 1, rend - (rns + 1)) ||
                           (lns && __zp_ke_intersects_stardsl(lns + 1, lend - (lns + 1), rstart, rend - rstart));
                }
                break;

            case 1:
                break;  // if either chunk is a small wild, yet neither is a big wild, just skip this chunk inspection

            default:
                lclen = lcend - lstart;
                streq = lclen == (size_t)(rcend - rstart) && strncmp(lstart, rstart, lclen) == 0;
                if (!(streq || __zp_ke_intersects_stardsl_chunk(lstart, lcend, rstart, rcend))) {
                    return false;
                }
        }

        if (!lns) {
            return !rns || (rend - rns == 3 && rns[1] == '*');
        } else if (!rns) {
            return !lns || (lend - lns == 3 && lns[1] == '*');
        }
        lstart = lns + 1;
        rstart = rns + 1;
    }
}

_Bool __zp_ke_intersects_nodsl(char const *lstart, const size_t llen, char const *rstart, const size_t rlen) {
    const char *lend = lstart + llen;
    const char *rend = rstart + rlen;
    for (;;) {
        const char *lns = memchr(lstart, '/', lend - lstart);
        const char *lcend = lns ? lns : lend;
        const char *rns = memchr(rstart, '/', rend - rstart);
        const char *rcend = rns ? rns : rend;
        char lwildness = 0;
        char rwildness = 0;

        if (*lstart == '*') {
            lwildness = lcend - lstart;
        }

        if (*rstart == '*') {
            rwildness = rcend - rstart;
        }

        switch (lwildness | rwildness) {
            case 2:
            case 3:
                if (lwildness == 2) {
                    return !lns || __zp_ke_intersects_nodsl(lns + 1, lend - (lns + 1), rstart, rend - rstart) ||
                           (rns && __zp_ke_intersects_nodsl(lstart, lend - lstart, rns + 1, rend - (rns + 1)));
                } else {
                    return !rns || __zp_ke_intersects_nodsl(lstart, lend - lstart, rns + 1, rend - (rns + 1)) ||
                           (lns && __zp_ke_intersects_nodsl(lns + 1, lend - (lns + 1), rstart, rend - rstart));
                }
                break;

            case 1:
                break;  // if either chunk is a small wild, yet neither is a big wild, just skip this chunk inspection

            default:
                if ((lcend - lstart != rcend - rstart) || strncmp(lstart, rstart, lcend - lstart)) {
                    return false;
                }
        }

        if (!lns) {
            return !rns || (rend - rns == 3 && rns[1] == '*');
        } else if (!rns) {
            return !lns || (lend - lns == 3 && lns[1] == '*');
        }

        lstart = lns + 1;
        rstart = rns + 1;
    }
}

/*------------------ Zenoh-Core helpers ------------------*/
_Bool _z_keyexpr_includes(const char *lstart, const size_t llen, const char *rstart, const size_t rlen) {
    _Bool streq = (llen == rlen) && (strncmp(lstart, rstart, llen) == 0);
    if (streq) {
        return true;
    }

    int contains = 0;  // STAR: 1, DSL: 2
    char const *end = lstart + llen;
    for (char const *c = lstart; c < end; c++) {
        if (*c == '*') {
            contains |= 1;
        }
        if (*c == '$') {
            contains |= 3;
            break;
        }
    }

    end = rstart + rlen;
    for (char const *c = rstart; contains < 3 && c < end; c++) {
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
    char const *end = lstart + llen;
    for (char const *c = lstart; c < end; c++) {
        if (*c == '*') {
            contains |= 1;
        }

        if (*c == '$') {
            contains |= 3;
            break;
        }
    }

    end = rstart + rlen;
    for (char const *c = rstart; contains < 3 && c < end; c++) {
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

    const char *end = start + *len;
    char *reader = start + canon_len;
    const char *write_start = reader;
    char *writer = reader;
    char *next_slash = memchr(reader, '/', end - reader);
    char const *chunk_end = next_slash ? next_slash : end;

    _Bool in_big_wild = false;
    if (chunk_end - reader == 2 && reader[1] == '*') {
        switch (*reader) {
            case '*':
                in_big_wild = true;
                break;

            case '$':  // if the chunk is $*, replace it with *.
                *writer++ = '*';
                break;

            default:
                assert(false);  // anything before "$*" or "**" must be part of the canon prefix
                break;
        }
    } else {
        assert(false);  // anything before "$*" or "**" must be part of the canon prefix
    }

    while (next_slash) {
        reader = next_slash + 1;
        next_slash = memchr(reader, '/', end - reader);
        chunk_end = next_slash ? next_slash : end;
        switch (chunk_end - reader) {
            case 0:
                return Z_KEYEXPR_CANON_EMPTY_CHUNK;

            case 1:
                __zp_ke_write_chunk(&writer, "*", 1, write_start);
                continue;

            case 2:
                if (reader[1] == '*') {
                    switch (*reader) {
                        case '$':
                            __zp_ke_write_chunk(&writer, "*", 1, write_start);
                            continue;

                        case '*':
                            in_big_wild = true;
                            continue;
                    }
                }
                break;
        }

        unsigned char in_dollar = 0;
        for (char const *c = reader; c < end; c++) {
            switch (*c) {
                case '#':
                case '?':
                    return Z_KEYEXPR_CANON_CONTAINS_SHARP_OR_QMARK;

                case '$':
                    if (in_dollar) {
                        return Z_KEYEXPR_CANON_DOLLAR_AFTER_DOLLAR_OR_STAR;
                    }
                    in_dollar += 1;
                    break;

                case '*':
                    if (in_dollar != 1) {
                        return Z_KEYEXPR_CANON_STARS_IN_CHUNK;
                    }
                    in_dollar += 2;
                    break;

                default:
                    if (in_dollar == 1) {
                        return Z_KEYEXPR_CANON_CONTAINS_UNBOUND_DOLLAR;
                    }
                    in_dollar = 0;
            }
        }

        if (in_dollar == 1) {
            return Z_KEYEXPR_CANON_CONTAINS_UNBOUND_DOLLAR;
        }

        if (in_big_wild) {
            __zp_ke_write_chunk(&writer, "**", 2, write_start);
        }

        __zp_ke_write_chunk(&writer, reader, chunk_end - reader, write_start);
        in_big_wild = false;
    }

    if (in_big_wild) {
        __zp_ke_write_chunk(&writer, "**", 2, write_start);
    }
    *len = writer - start;

    return Z_KEYEXPR_CANON_SUCCESS;
}

zp_keyexpr_canon_status_t _z_keyexpr_is_canon(const char *start, size_t len) { return __zp_canon_prefix(start, &len); }
