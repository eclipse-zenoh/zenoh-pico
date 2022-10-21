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
#include <stddef.h>
#include <string.h>

#include "zenoh-pico/utils/pointers.h"
#include "zenoh-pico/utils/string.h"

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

void __zp_singleify(char *start, size_t *len, const char *needle) {
    const char *end = _z_cptr_char_offset(start, *len);
    _Bool right_after_needle = false;
    char *reader = start;
    while (reader < end) {
        size_t pos = _z_str_startswith(reader, needle);
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
        size_t pos = _z_str_startswith(reader, needle);
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

/*------------------ Common helpers ------------------*/
typedef _Bool (*_z_ke_chunk_matcher)(_z_borrowed_str_t l, _z_borrowed_str_t r);

enum _zp_wildness_t { _ZP_WILDNESS_ANY = 1, _ZP_WILDNESS_SUPERCHUNKS = 2, _ZP_WILDNESS_SUBCHUNK_DSL = 4 };
char _zp_ke_wildness(_z_borrowed_str_t ke, size_t *n_segments) {
    const char *start = ke.start;
    const char *end = ke.end;
    char result = 0;
    char prev_char = 0;
    for (char const *c = start; c < end; c = _z_cptr_char_offset(c, 1)) {
        switch (*c) {
            case '*':
                result |= _ZP_WILDNESS_ANY;
                if (prev_char == '*') {
                    result |= _ZP_WILDNESS_SUPERCHUNKS;
                }
                break;
            case '$':
                result |= _ZP_WILDNESS_SUBCHUNK_DSL;
                break;
            case '/':
                *n_segments = *n_segments + 1;
                break;
        }
        prev_char = *c;
    }
    return result;
}

const char *_Z_DELIMITER = "/";
const char *_Z_DOUBLE_STAR = "**";
const char *_Z_DOLLAR_STAR = "$*";
bool _z_ke_isdoublestar(_z_borrowed_str_t s) { return s.end - s.start == 2 && s.start[0] == '*' && s.start[1] == '*'; }
/*------------------ Inclusion helpers ------------------*/
bool _z_ke_chunk_includes_nodsl(_z_borrowed_str_t l, _z_borrowed_str_t r) {
    size_t llen = l.end - l.start;
    bool result = llen == 1 && l.start[0] == '*' && !(r.end - r.start == 2 && r.start[0] == '*' && r.start[1] == '*');
    if (!result && llen == (size_t)(r.end - r.start)) {
        result = strncmp(l.start, r.start, llen) == 0;
    }
    return result;
}
bool _z_ke_chunk_includes_stardsl(_z_borrowed_str_t l, _z_borrowed_str_t r) {
    bool result = _z_ke_chunk_includes_nodsl(l, r);
    if (!result) {
        _z_splitstr_t lcs = {.s = l, .delimiter = _Z_DOLLAR_STAR};
        l = _z_splitstr_next(&lcs);
        result = r.end - r.start >= l.end - l.start;
        for (; result && l.start < l.end; l.start++, r.start++) {
            result = l.start[0] == r.start[0];
        }
        if (result) {
            l = _z_splitstr_nextback(&lcs);
            result = l.start && r.end - r.start >= l.end - l.start;
            for (--l.end, --r.end; result && l.start <= l.end; l.end--, r.end--) {
                result = *l.end == *r.end;
            }
            r.end++;
        }
        while (result) {
            l = _z_splitstr_next(&lcs);
            if (!l.start) {
                break;
            }
            r.start = _z_bstrstr_skipneedle(r, l);
            if (!r.start) {
                result = false;
            }
        }
    }
    return result;
}

/*------------------ Intersection helpers ------------------*/
_Bool _z_ke_chunk_intersect_nodsl(_z_borrowed_str_t l, _z_borrowed_str_t r) {
    _Bool result = l.start[0] == '*' || r.start[0] == '*';
    if (!result) {
        size_t lclen = _z_ptr_char_diff(l.end, l.start);
        result = (lclen == _z_ptr_char_diff(r.end, r.start)) && (strncmp(l.start, r.start, lclen) == 0);
    }
    return result;
}
_Bool _z_ke_chunk_intersect_rhasstardsl(_z_borrowed_str_t l, _z_borrowed_str_t r) {
    _Bool result = true;
    _z_splitstr_t rchunks = {.s = r, .delimiter = _Z_DOLLAR_STAR};
    r = _z_splitstr_next(&rchunks);
    result = r.end - r.start <= l.end - l.start;
    for (; result && r.start < r.end; l.start++, r.start++) {
        result = *l.start == *r.start;
    }
    if (result) {
        r = _z_splitstr_nextback(&rchunks);
        result = r.end - r.start <= l.end - l.start;
        for (--l.end, --r.end; result && r.start <= r.end; l.end--, r.end--) {
            result = *l.end == *r.end;
        }
        l.end++;
    }
    while (result) {
        r = _z_splitstr_next(&rchunks);
        if (!r.start) {
            break;
        }
        l.start = _z_bstrstr_skipneedle(l, r);
        if (!l.start) {
            result = false;
        }
    }
    return result;
}
_Bool _z_ke_chunk_intersect_stardsl(_z_borrowed_str_t l, _z_borrowed_str_t r) {
    _Bool result = _z_ke_chunk_intersect_nodsl(l, r);
    if (!result) {
        result = true;
        bool l_has_stardsl = _z_strstr(l.start, l.end, _Z_DOLLAR_STAR) != NULL;
        if (l_has_stardsl) {
            bool r_has_stardsl = _z_strstr(r.start, r.end, _Z_DOLLAR_STAR) != NULL;
            if (r_has_stardsl) {
                for (char const *lc = l.start, *rc = r.start;
                     result && lc < l.end && *lc != '$' && rc < r.end && *rc != '$'; lc++, rc++) {
                    result = *lc == *rc;
                }
                for (char const *lc = l.end - 1, *rc = r.end - 1;
                     result && lc >= l.start && *lc != '*' && rc >= r.start && *rc != '*'; lc--, rc--) {
                    result = *lc == *rc;
                }
            } else {
                result = _z_ke_chunk_intersect_rhasstardsl(r, l);
            }
        } else {
            // r is guaranteed to have a stardsl if l doesn't and this function has been called.
            result = _z_ke_chunk_intersect_rhasstardsl(l, r);
        }
    }
    return result;
}
_Bool _z_ke_intersect_rhassuperchunks(_z_borrowed_str_t l, _z_borrowed_str_t r, _z_ke_chunk_matcher chunk_intersector) {
    _Bool result = true;
    _z_splitstr_t lchunks = {.s = l, .delimiter = _Z_DELIMITER};
    _z_splitstr_t rsplitatsuperchunks = {.s = r, .delimiter = _Z_DOUBLE_STAR};
    _z_borrowed_str_t rnosuper = _z_splitstr_next(&rsplitatsuperchunks);
    if (rnosuper.start != rnosuper.end) {
        _z_splitstr_t prefix_chunks = {.s = {.start = rnosuper.start, .end = rnosuper.end - 1},
                                       .delimiter = _Z_DELIMITER};
        _z_borrowed_str_t pchunk = _z_splitstr_next(&prefix_chunks);
        while (result && pchunk.start) {
            _z_borrowed_str_t lchunk = _z_splitstr_next(&lchunks);
            if (lchunk.start) {
                result = chunk_intersector(lchunk, pchunk);
                pchunk = _z_splitstr_next(&prefix_chunks);
            } else {
                result = false;
            }
        }
    }
    if (result) {
        rnosuper = _z_splitstr_nextback(&rsplitatsuperchunks);
        if (rnosuper.start != rnosuper.end) {
            _z_splitstr_t suffix_chunks = {.s = {.start = rnosuper.start + 1, .end = rnosuper.end},
                                           .delimiter = _Z_DELIMITER};
            _z_borrowed_str_t schunk = _z_splitstr_nextback(&suffix_chunks);
            while (result && schunk.start) {
                _z_borrowed_str_t lchunk = _z_splitstr_nextback(&lchunks);
                if (lchunk.start) {
                    result = chunk_intersector(lchunk, schunk);
                    schunk = _z_splitstr_nextback(&suffix_chunks);
                } else {
                    result = false;
                }
            }
        }
    }
    while (result) {
        rnosuper = _z_splitstr_next(&rsplitatsuperchunks);
        if (!rnosuper.start) {
            break;
        }
        _z_splitstr_t needle = {.s = {.start = rnosuper.start + 1, .end = rnosuper.end - 1}, .delimiter = _Z_DELIMITER};
        _z_splitstr_t haystack = lchunks;
        _z_borrowed_str_t needle_start = _z_splitstr_next(&needle);
        bool needle_found = false;
        for (_z_borrowed_str_t h = _z_splitstr_next(&haystack); !needle_found && h.start;
             h = _z_splitstr_next(&haystack)) {
            if (chunk_intersector(needle_start, h)) {
                needle_found = true;
                _z_splitstr_t needlecp = needle;
                _z_borrowed_str_t n = _z_splitstr_next(&needlecp);
                _z_splitstr_t haystackcp = haystack;
                while (needle_found && n.start) {
                    h = _z_splitstr_next(&haystackcp);
                    if (h.start) {
                        if (!chunk_intersector(n, h)) {
                            needle_found = false;
                        } else {
                            n = _z_splitstr_next(&needlecp);
                        }
                    } else {
                        needle_found = false;
                        haystack.s = (_z_borrowed_str_t){.start = NULL, .end = NULL};
                    }
                }
                if (needle_found) {
                    lchunks = haystackcp;
                }
            } else {
                needle_found = false;
            }
        }
    }
    return result;
}

/*------------------ Zenoh-Core helpers ------------------*/
_Bool _z_keyexpr_includes(const char *lstart, const size_t llen, const char *rstart, const size_t rlen) {
    _Bool result = (llen == rlen) && (strncmp(lstart, rstart, llen) == 0);
    if (!result) {
        _z_borrowed_str_t l = {.start = lstart, .end = _z_cptr_char_offset(lstart, llen)};
        _z_borrowed_str_t r = {.start = rstart, .end = _z_cptr_char_offset(rstart, rlen)};
        _z_splitstr_t rchunks = {.s = r, .delimiter = _Z_DELIMITER};
        size_t ln_chunks = 0, rn_chunks = 0;
        char lwildness = _zp_ke_wildness(l, &ln_chunks);
        char rwildness = _zp_ke_wildness(r, &rn_chunks);
        char wildness = lwildness | rwildness;
        _z_ke_chunk_matcher chunk_intersector =
            wildness & _ZP_WILDNESS_SUBCHUNK_DSL ? _z_ke_chunk_includes_stardsl : _z_ke_chunk_includes_nodsl;
        if (lwildness & _ZP_WILDNESS_SUPERCHUNKS) {
            result = true;
            _z_splitstr_t lsplitatsuper = {.s = l, .delimiter = _Z_DOUBLE_STAR};
            _z_borrowed_str_t lnosuper = _z_splitstr_next(&lsplitatsuper);
            _z_borrowed_str_t rchunk;
            if (lnosuper.start != lnosuper.end) {
                _z_splitstr_t pchunks = {.s = {.start = lnosuper.start, .end = lnosuper.end - 1},
                                         .delimiter = _Z_DELIMITER};
                _z_borrowed_str_t lchunk = _z_splitstr_next(&pchunks);
                while (result && lchunk.start) {
                    rchunk = _z_splitstr_next(&rchunks);
                    result = rchunk.start && chunk_intersector(lchunk, rchunk);
                    lchunk = _z_splitstr_next(&pchunks);
                }
            }
            if (result) {
                lnosuper = _z_splitstr_nextback(&lsplitatsuper);
                if (lnosuper.start != lnosuper.end) {
                    _z_splitstr_t schunks = {.s = {.start = lnosuper.start + 1, .end = lnosuper.end},
                                             .delimiter = _Z_DELIMITER};
                    _z_borrowed_str_t lchunk = _z_splitstr_nextback(&schunks);
                    while (result && lchunk.start) {
                        rchunk = _z_splitstr_nextback(&rchunks);
                        result = rchunk.start && chunk_intersector(lchunk, rchunk);
                        lchunk = _z_splitstr_nextback(&schunks);
                    }
                }
            }
            while (result) {
                lnosuper = _z_splitstr_next(&lsplitatsuper);
                if (!lnosuper.start) {
                    break;
                }
                _z_splitstr_t needle = {.s = {.start = lnosuper.start + 1, .end = lnosuper.end - 1},
                                        .delimiter = _Z_DELIMITER};
                _z_splitstr_t haystack = rchunks;
                _z_borrowed_str_t needle_start = _z_splitstr_next(&needle);
                bool needle_found = false;
                for (_z_borrowed_str_t h = _z_splitstr_next(&haystack); !needle_found && h.start;
                     h = _z_splitstr_next(&haystack)) {
                    if (chunk_intersector(needle_start, h)) {
                        needle_found = true;
                        _z_splitstr_t needlecp = needle;
                        _z_borrowed_str_t n = _z_splitstr_next(&needlecp);
                        _z_splitstr_t haystackcp = haystack;
                        while (needle_found && n.start) {
                            h = _z_splitstr_next(&haystackcp);
                            if (h.start) {
                                if (!chunk_intersector(n, h)) {
                                    needle_found = false;
                                } else {
                                    n = _z_splitstr_next(&needlecp);
                                }
                            } else {
                                needle_found = false;
                                haystack.s = (_z_borrowed_str_t){.start = NULL, .end = NULL};
                            }
                        }
                        if (needle_found) {
                            rchunks = haystackcp;
                        }
                    } else {
                        needle_found = false;
                    }
                }
            }
        } else if ((rwildness & _ZP_WILDNESS_SUPERCHUNKS) == 0 && ln_chunks == rn_chunks) {
            _z_splitstr_t lchunks = {.s = l, .delimiter = _Z_DELIMITER};
            _z_splitstr_t rchunks = {.s = r, .delimiter = _Z_DELIMITER};
            _z_borrowed_str_t lchunk = _z_splitstr_next(&lchunks);
            _z_borrowed_str_t rchunk = _z_splitstr_next(&rchunks);
            result = true;
            while (result && lchunk.start) {
                result = chunk_intersector(lchunk, rchunk);
                lchunk = _z_splitstr_next(&lchunks);
                rchunk = _z_splitstr_next(&rchunks);
            }
        } else {
            // If l doesn't have superchunks, but r does, or they have different chunk counts, non-inclusion is
            // guaranteed
        }
    }
    return result;
}

_Bool _z_keyexpr_intersects(const char *lstart, const size_t llen, const char *rstart, const size_t rlen) {
    _Bool result = (llen == rlen) && (strncmp(lstart, rstart, llen) == 0);
    if (!result) {
        _z_borrowed_str_t l = {.start = lstart, .end = _z_cptr_char_offset(lstart, llen)};
        _z_borrowed_str_t r = {.start = rstart, .end = _z_cptr_char_offset(rstart, rlen)};
        size_t ln_chunks = 0, rn_chunks = 0;
        char lwildness = _zp_ke_wildness(l, &ln_chunks);
        char rwildness = _zp_ke_wildness(r, &rn_chunks);
        char wildness = lwildness | rwildness;
        _z_ke_chunk_matcher chunk_intersector =
            wildness & _ZP_WILDNESS_SUBCHUNK_DSL ? _z_ke_chunk_intersect_stardsl : _z_ke_chunk_intersect_nodsl;
        if (wildness) {
            if ((lwildness & rwildness & _ZP_WILDNESS_SUPERCHUNKS)) {
                // TODO: both expressions contain superchunks
                _z_splitstr_t lchunks = {.s = l, .delimiter = _Z_DELIMITER};
                _z_splitstr_t rchunks = {.s = r, .delimiter = _Z_DELIMITER};
                result = true;
                while (result) {
                    _z_borrowed_str_t lchunk = _z_splitstr_next(&lchunks);
                    _z_borrowed_str_t rchunk = _z_splitstr_next(&rchunks);
                    if (lchunk.start || rchunk.start) {
                        if (_z_ke_isdoublestar(lchunk) || _z_ke_isdoublestar(rchunk)) {
                            break;
                        } else if (lchunk.start && rchunk.start) {
                            result = chunk_intersector(lchunk, rchunk);
                        } else {
                            // Guaranteed not to happen, as both iterators contain a `**` which will break iteration.
                        }
                    } else {
                        // Guaranteed not to happen, as both iterators contain a `**` which will break iteration.
                    }
                }
                while (result) {
                    _z_borrowed_str_t lchunk = _z_splitstr_nextback(&lchunks);
                    _z_borrowed_str_t rchunk = _z_splitstr_nextback(&rchunks);
                    if (lchunk.start || rchunk.start) {
                        if (_z_ke_isdoublestar(lchunk) || _z_ke_isdoublestar(rchunk)) {
                            break;
                        } else if (lchunk.start && rchunk.start) {
                            result = chunk_intersector(lchunk, rchunk);
                        } else {
                            // Happens in cases such as `a/**/b` with `a/**/c/b`
                            break;
                        }
                    } else {
                        break;
                    }
                }
            } else if (lwildness & _ZP_WILDNESS_SUPERCHUNKS && ln_chunks <= rn_chunks * 2 + 1) {
                result = _z_ke_intersect_rhassuperchunks(r, l, chunk_intersector);
            } else if (rwildness & _ZP_WILDNESS_SUPERCHUNKS && rn_chunks <= ln_chunks * 2 + 1) {
                result = _z_ke_intersect_rhassuperchunks(l, r, chunk_intersector);
            } else if (ln_chunks == rn_chunks) {
                // no superchunks, just iterate and check chunk intersection
                _z_splitstr_t lchunks = {.s = l, .delimiter = _Z_DELIMITER};
                _z_splitstr_t rchunks = {.s = r, .delimiter = _Z_DELIMITER};
                _z_borrowed_str_t lchunk = _z_splitstr_next(&lchunks);
                _z_borrowed_str_t rchunk = _z_splitstr_next(&rchunks);
                result = true;
                while (result && lchunk.start) {
                    result = chunk_intersector(lchunk, rchunk);
                    lchunk = _z_splitstr_next(&lchunks);
                    rchunk = _z_splitstr_next(&rchunks);
                }
            } else {
                // No superchunks detected, and number of chunks differ: no intersection guaranteed.
            }
        } else {
            // No string equality and no wildness detected: no intersection guaranteed.
        }
    } else {
        // String equality guarantees intersection, no further process needed.
    }
    return result;
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
            __zp_ke_write_chunk(&writer, _Z_DOUBLE_STAR, 2, write_start);
        }

        __zp_ke_write_chunk(&writer, reader, _z_ptr_char_diff(chunk_end, reader), write_start);
        in_big_wild = false;
    }

    if (in_big_wild) {
        __zp_ke_write_chunk(&writer, _Z_DOUBLE_STAR, 2, write_start);
    }
    *len = _z_ptr_char_diff(writer, start);

    return Z_KEYEXPR_CANON_SUCCESS;
}

zp_keyexpr_canon_status_t _z_keyexpr_is_canon(const char *start, size_t len) { return __zp_canon_prefix(start, &len); }
