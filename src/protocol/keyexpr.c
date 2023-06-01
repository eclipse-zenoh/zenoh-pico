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
    zp_keyexpr_canon_status_t ret = Z_KEYEXPR_CANON_SUCCESS;

    _Bool in_big_wild = false;
    char const *chunk_start = start;
    const char *end = _z_cptr_char_offset(start, *len);
    char const *next_slash;

    do {
        next_slash = strchr(chunk_start, '/');
        const char *chunk_end = next_slash ? next_slash : end;
        size_t chunk_len = _z_ptr_char_diff(chunk_end, chunk_start);
        switch (chunk_len) {
            case 0: {
                ret = Z_KEYEXPR_CANON_EMPTY_CHUNK;
            } break;

            case 1: {
                if (in_big_wild && (chunk_start[0] == '*')) {
                    *len = _z_ptr_char_diff(chunk_start, start) - (size_t)3;
                    ret = Z_KEYEXPR_CANON_SINGLE_STAR_AFTER_DOUBLE_STAR;
                } else {
                    chunk_start = _z_cptr_char_offset(chunk_end, 1);
                    continue;
                }
            } break;

            case 2:
                if (chunk_start[1] == '*') {
                    if (chunk_start[0] == '$') {
                        *len = _z_ptr_char_diff(chunk_start, start);
                        ret = Z_KEYEXPR_CANON_LONE_DOLLAR_STAR;
                    } else if (chunk_start[0] == '*') {
                        if (in_big_wild) {
                            *len = _z_ptr_char_diff(chunk_start, start) - (size_t)3;
                            ret = Z_KEYEXPR_CANON_DOUBLE_STAR_AFTER_DOUBLE_STAR;
                        } else {
                            chunk_start = _z_cptr_char_offset(chunk_end, 1);
                            in_big_wild = true;
                            continue;
                        }
                    } else {
                        // Do nothing. Required to be compliant with MISRA 15.7 rule
                    }
                } else {
                    // Do nothing. Required to be compliant with MISRA 15.7 rule
                }
                break;

            default:
                break;
        }

        unsigned char in_dollar = 0;
        for (char const *c = chunk_start; (c < chunk_end) && (ret == Z_KEYEXPR_CANON_SUCCESS);
             c = _z_cptr_char_offset(c, 1)) {
            switch (c[0]) {
                case '#':
                case '?': {
                    ret = Z_KEYEXPR_CANON_CONTAINS_SHARP_OR_QMARK;
                } break;

                case '$': {
                    if (in_dollar != (unsigned char)0) {
                        ret = Z_KEYEXPR_CANON_DOLLAR_AFTER_DOLLAR_OR_STAR;
                    } else {
                        in_dollar = in_dollar + (unsigned char)1;
                    }
                } break;

                case '*': {
                    if (in_dollar != (unsigned char)1) {
                        ret = Z_KEYEXPR_CANON_STARS_IN_CHUNK;
                    } else {
                        in_dollar = in_dollar + (unsigned char)2;
                    }
                } break;

                default: {
                    if (in_dollar == (unsigned char)1) {
                        ret = Z_KEYEXPR_CANON_CONTAINS_UNBOUND_DOLLAR;
                    } else {
                        in_dollar = 0;
                    }
                } break;
            }
        }

        if (ret == Z_KEYEXPR_CANON_SUCCESS) {
            if (in_dollar == (unsigned char)1) {
                ret = Z_KEYEXPR_CANON_CONTAINS_UNBOUND_DOLLAR;
            } else {
                chunk_start = _z_cptr_char_offset(chunk_end, 1);
                in_big_wild = false;
            }
        }
    } while ((chunk_start < end) && (ret == Z_KEYEXPR_CANON_SUCCESS));

    if (ret == Z_KEYEXPR_CANON_SUCCESS) {
        if (chunk_start <= end) {
            ret = Z_KEYEXPR_CANON_EMPTY_CHUNK;
        }
    }

    return ret;
}

void __zp_singleify(char *start, size_t *len, const char *needle) {
    const char *end = _z_cptr_char_offset(start, *len);
    _Bool right_after_needle = false;
    char *reader = start;

    while (reader < end) {
        size_t pos = _z_str_startswith(reader, needle);
        if (pos != (size_t)0) {
            if (right_after_needle == true) {
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
            if (right_after_needle == false) {
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
    if (writer[0] != write_start) {
        writer[0][0] = '/';
        writer[0] = _z_ptr_char_offset(writer[0], 1);
    }

    (void)memcpy(writer[0], chunk, len);
    writer[0] = _z_ptr_char_offset(writer[0], len);
}

/*------------------ Common helpers ------------------*/
typedef _Bool (*_z_ke_chunk_matcher)(_z_str_se_t l, _z_str_se_t r);

enum _zp_wildness_t { _ZP_WILDNESS_ANY = 1, _ZP_WILDNESS_SUPERCHUNKS = 2, _ZP_WILDNESS_SUBCHUNK_DSL = 4 };
int8_t _zp_ke_wildness(_z_str_se_t ke, size_t *n_segments) {
    const char *start = ke.start;
    const char *end = ke.end;
    int8_t result = 0;
    char prev_char = 0;
    for (char const *c = start; c < end; c = _z_cptr_char_offset(c, 1)) {
        switch (c[0]) {
            case '*': {
                result = result | (int8_t)_ZP_WILDNESS_ANY;
                if (prev_char == '*') {
                    result = result | (int8_t)_ZP_WILDNESS_SUPERCHUNKS;
                }
            } break;

            case '$': {
                result = result | (int8_t)_ZP_WILDNESS_SUBCHUNK_DSL;
            } break;

            case '/': {
                *n_segments = *n_segments + (size_t)1;
            } break;

            default: {
                // Do nothing
            } break;
        }

        prev_char = *c;
    }

    return result;
}

const char *_Z_DELIMITER = "/";
const char *_Z_DOUBLE_STAR = "**";
const char *_Z_DOLLAR_STAR = "$*";
_Bool _z_ke_isdoublestar(_z_str_se_t s) {
    return ((_z_ptr_char_diff(s.end, s.start) == 2) && (s.start[0] == '*') && (s.start[1] == '*'));
}

/*------------------ Inclusion helpers ------------------*/
_Bool _z_ke_chunk_includes_nodsl(_z_str_se_t l, _z_str_se_t r) {
    size_t llen = l.end - l.start;
    _Bool result = ((llen == (size_t)1) && (l.start[0] == '*') &&
                    (((_z_ptr_char_diff(r.end, r.start) == 2) && (r.start[0] == '*') && (r.start[1] == '*')) == false));
    if ((result == false) && (llen == _z_ptr_char_diff(r.end, r.start))) {
        result = strncmp(l.start, r.start, llen) == 0;
    }

    return result;
}

_Bool _z_ke_chunk_includes_stardsl(_z_str_se_t l1, _z_str_se_t r) {
    _Bool result = _z_ke_chunk_includes_nodsl(l1, r);
    if (result == false) {
        _z_splitstr_t lcs = {.s = l1, .delimiter = _Z_DOLLAR_STAR};
        _z_str_se_t split_l = _z_splitstr_next(&lcs);
        result = (_z_ptr_char_diff(r.end, r.start) >= _z_ptr_char_diff(split_l.end, split_l.start));
        while ((result == true) && (split_l.start < split_l.end)) {
            result = (split_l.start[0] == r.start[0]);
            split_l.start = _z_cptr_char_offset(split_l.start, 1);
            r.start = _z_cptr_char_offset(r.start, 1);
        }
        if (result == true) {
            split_l = _z_splitstr_nextback(&lcs);
            result = ((split_l.start != NULL) &&
                      (_z_ptr_char_diff(r.end, r.start) >= _z_ptr_char_diff(split_l.end, split_l.start)));

            split_l.end = _z_cptr_char_offset(split_l.end, -1);
            r.end = _z_cptr_char_offset(r.end, -1);
            while ((result == true) && (split_l.start <= split_l.end)) {
                result = (split_l.end[0] == r.end[0]);
                split_l.end = _z_cptr_char_offset(split_l.end, -1);
                r.end = _z_cptr_char_offset(r.end, -1);
            }
            r.end = _z_cptr_char_offset(r.end, 1);
        }
        while (result == true) {
            split_l = _z_splitstr_next(&lcs);
            if (split_l.start == NULL) {
                break;
            }
            r.start = _z_bstrstr_skipneedle(r, split_l);
            if (r.start == NULL) {
                result = false;
            }
        }
    }

    return result;
}

/*------------------ Intersection helpers ------------------*/
_Bool _z_ke_chunk_intersect_nodsl(_z_str_se_t l, _z_str_se_t r) {
    _Bool result = ((l.start[0] == '*') || (r.start[0] == '*'));
    if (result == false) {
        size_t lclen = _z_ptr_char_diff(l.end, l.start);
        result = ((lclen == _z_ptr_char_diff(r.end, r.start)) && (strncmp(l.start, r.start, lclen) == 0));
    }

    return result;
}
_Bool _z_ke_chunk_intersect_rhasstardsl(_z_str_se_t l, _z_str_se_t r) {
    _Bool result = true;
    _z_splitstr_t rchunks = {.s = r, .delimiter = _Z_DOLLAR_STAR};
    _z_str_se_t split_r = _z_splitstr_next(&rchunks);
    result = _z_ptr_char_diff(split_r.end, split_r.start) <= _z_ptr_char_diff(l.end, l.start);
    while ((result == true) && (split_r.start < split_r.end)) {
        result = (l.start[0] == split_r.start[0]);
        l.start = _z_cptr_char_offset(l.start, 1);
        split_r.start = _z_cptr_char_offset(split_r.start, 1);
    }
    if (result == true) {
        split_r = _z_splitstr_nextback(&rchunks);
        result = (_z_ptr_char_diff(split_r.end, split_r.start) <= _z_ptr_char_diff(l.end, l.start));
        l.end = _z_cptr_char_offset(l.end, -1);
        split_r.end = _z_cptr_char_offset(split_r.end, -1);
        while ((result == true) && (split_r.start <= split_r.end)) {
            result = (l.end[0] == split_r.end[0]);
            l.end = _z_cptr_char_offset(l.end, -1);
            split_r.end = _z_cptr_char_offset(split_r.end, -1);
        }
        l.end = _z_cptr_char_offset(l.end, 1);
    }
    while (result == true) {
        split_r = _z_splitstr_next(&rchunks);
        if (split_r.start == NULL) {
            break;
        }
        l.start = _z_bstrstr_skipneedle(l, split_r);
        if (l.start == NULL) {
            result = false;
        }
    }

    return result;
}
_Bool _z_ke_chunk_intersect_stardsl(_z_str_se_t l, _z_str_se_t r) {
    _Bool result = _z_ke_chunk_intersect_nodsl(l, r);
    if (result == false) {
        result = true;
        _Bool l_has_stardsl = (_z_strstr(l.start, l.end, _Z_DOLLAR_STAR) != NULL);
        if (l_has_stardsl == true) {
            _Bool r_has_stardsl = (_z_strstr(r.start, r.end, _Z_DOLLAR_STAR) != NULL);
            if (r_has_stardsl == true) {
                char const *lc = l.start;
                char const *rc = r.start;
                while ((result == true) && (lc < l.end) && (lc[0] != '$') && (rc < r.end) && (rc[0] != '$')) {
                    result = (lc[0] == rc[0]);
                    lc = _z_cptr_char_offset(lc, 1);
                    rc = _z_cptr_char_offset(rc, 1);
                }
                lc = _z_cptr_char_offset(l.end, -1);
                rc = _z_cptr_char_offset(r.end, -1);
                while ((result == true) && (lc >= l.start) && (lc[0] != '*') && (rc >= r.start) && (rc[0] != '*')) {
                    result = (lc[0] == rc[0]);
                    lc = _z_cptr_char_offset(lc, -1);
                    rc = _z_cptr_char_offset(rc, -1);
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
_Bool _z_ke_intersect_rhassuperchunks(_z_str_se_t l, _z_str_se_t r, _z_ke_chunk_matcher chunk_intersector) {
    _Bool result = true;
    _z_splitstr_t lchunks = {.s = l, .delimiter = _Z_DELIMITER};
    _z_splitstr_t rsplitatsuperchunks = {.s = r, .delimiter = _Z_DOUBLE_STAR};
    _z_str_se_t rnosuper = _z_splitstr_next(&rsplitatsuperchunks);
    if (rnosuper.start != rnosuper.end) {
        _z_splitstr_t prefix_chunks = {.s = {.start = rnosuper.start, .end = _z_cptr_char_offset(rnosuper.end, -1)},
                                       .delimiter = _Z_DELIMITER};
        _z_str_se_t pchunk = _z_splitstr_next(&prefix_chunks);
        while ((result == true) && (pchunk.start != NULL)) {
            _z_str_se_t lchunk = _z_splitstr_next(&lchunks);
            if (lchunk.start != NULL) {
                result = chunk_intersector(lchunk, pchunk);
                pchunk = _z_splitstr_next(&prefix_chunks);
            } else {
                result = false;
            }
        }
    }
    if (result == true) {
        rnosuper = _z_splitstr_nextback(&rsplitatsuperchunks);
        if (rnosuper.start != rnosuper.end) {
            _z_splitstr_t suffix_chunks = {.s = {.start = _z_cptr_char_offset(rnosuper.start, 1), .end = rnosuper.end},
                                           .delimiter = _Z_DELIMITER};
            _z_str_se_t schunk = _z_splitstr_nextback(&suffix_chunks);
            while ((result == true) && (schunk.start != NULL)) {
                _z_str_se_t lchunk = _z_splitstr_nextback(&lchunks);
                if (lchunk.start != NULL) {
                    result = chunk_intersector(lchunk, schunk);
                    schunk = _z_splitstr_nextback(&suffix_chunks);
                } else {
                    result = false;
                }
            }
        }
    }
    while (result == true) {
        rnosuper = _z_splitstr_next(&rsplitatsuperchunks);
        if (rnosuper.start == NULL) {
            break;
        }
        _z_splitstr_t needle = {
            .s = {.start = _z_cptr_char_offset(rnosuper.start, 1), .end = _z_cptr_char_offset(rnosuper.end, -1)},
            .delimiter = _Z_DELIMITER};
        _z_splitstr_t haystack = lchunks;
        _z_str_se_t needle_start = _z_splitstr_next(&needle);
        _Bool needle_found = false;

        _z_str_se_t h = _z_splitstr_next(&haystack);
        while ((needle_found == false) && (h.start != NULL)) {
            if (chunk_intersector(needle_start, h) == true) {
                needle_found = true;
                _z_splitstr_t needlecp = needle;
                _z_str_se_t n = _z_splitstr_next(&needlecp);
                _z_splitstr_t haystackcp = haystack;
                while ((needle_found == true) && (n.start != NULL)) {
                    h = _z_splitstr_next(&haystackcp);
                    if (h.start != NULL) {
                        if (chunk_intersector(n, h) == false) {
                            needle_found = false;
                        } else {
                            n = _z_splitstr_next(&needlecp);
                        }
                    } else {
                        needle_found = false;
                        haystack.s = (_z_str_se_t){.start = NULL, .end = NULL};
                    }
                }
                if (needle_found == true) {
                    lchunks = haystackcp;
                }
            } else {
                needle_found = false;
            }
            h = _z_splitstr_next(&haystack);
        }
    }
    return result;
}

/*------------------ Zenoh-Core helpers ------------------*/
_Bool _z_keyexpr_includes(const char *lstart, const size_t llen, const char *rstart, const size_t rlen) {
    _Bool result = ((llen == rlen) && (strncmp(lstart, rstart, llen) == 0));
    if (result == false) {
        _z_str_se_t l = {.start = lstart, .end = _z_cptr_char_offset(lstart, llen)};
        _z_str_se_t r = {.start = rstart, .end = _z_cptr_char_offset(rstart, rlen)};
        size_t ln_chunks = (size_t)0;
        size_t rn_chunks = (size_t)0;
        int8_t lwildness = _zp_ke_wildness(l, &ln_chunks);
        int8_t rwildness = _zp_ke_wildness(r, &rn_chunks);
        int8_t wildness = lwildness | rwildness;
        _z_ke_chunk_matcher chunk_intersector =
            ((wildness & (int8_t)_ZP_WILDNESS_SUBCHUNK_DSL) == (int8_t)_ZP_WILDNESS_SUBCHUNK_DSL)
                ? _z_ke_chunk_includes_stardsl
                : _z_ke_chunk_includes_nodsl;
        if ((lwildness & (int8_t)_ZP_WILDNESS_SUPERCHUNKS) == (int8_t)_ZP_WILDNESS_SUPERCHUNKS) {
            result = true;
            _z_splitstr_t rchunks = {.s = r, .delimiter = _Z_DELIMITER};
            _z_splitstr_t lsplitatsuper = {.s = l, .delimiter = _Z_DOUBLE_STAR};
            _z_str_se_t lnosuper = _z_splitstr_next(&lsplitatsuper);
            _z_str_se_t rchunk;
            if (lnosuper.start != lnosuper.end) {
                _z_splitstr_t pchunks = {.s = {.start = lnosuper.start, .end = _z_cptr_char_offset(lnosuper.end, -1)},
                                         .delimiter = _Z_DELIMITER};
                _z_str_se_t lchunk = _z_splitstr_next(&pchunks);
                while ((result == true) && (lchunk.start != NULL)) {
                    rchunk = _z_splitstr_next(&rchunks);
                    result = ((rchunk.start != NULL) && (chunk_intersector(lchunk, rchunk) == true));
                    lchunk = _z_splitstr_next(&pchunks);
                }
            }
            if (result == true) {
                lnosuper = _z_splitstr_nextback(&lsplitatsuper);
                if (lnosuper.start != lnosuper.end) {
                    _z_splitstr_t schunks = {
                        .s = {.start = _z_cptr_char_offset(lnosuper.start, 1), .end = lnosuper.end},
                        .delimiter = _Z_DELIMITER};
                    _z_str_se_t lchunk = _z_splitstr_nextback(&schunks);
                    while ((result == true) && (lchunk.start != NULL)) {
                        rchunk = _z_splitstr_nextback(&rchunks);
                        result = rchunk.start && chunk_intersector(lchunk, rchunk);
                        lchunk = _z_splitstr_nextback(&schunks);
                    }
                }
            }
            while (result == true) {
                lnosuper = _z_splitstr_next(&lsplitatsuper);
                if (lnosuper.start == NULL) {
                    break;
                }
                _z_splitstr_t needle = {.s = {.start = _z_cptr_char_offset(lnosuper.start, 1),
                                              .end = _z_cptr_char_offset(lnosuper.end, -1)},
                                        .delimiter = _Z_DELIMITER};
                _z_splitstr_t haystack = rchunks;
                _z_str_se_t needle_start = _z_splitstr_next(&needle);
                _Bool needle_found = false;
                _z_str_se_t h = _z_splitstr_next(&haystack);
                while ((needle_found == false) && (h.start != NULL)) {
                    if (chunk_intersector(needle_start, h) == true) {
                        needle_found = true;
                        _z_splitstr_t needlecp = needle;
                        _z_str_se_t n = _z_splitstr_next(&needlecp);
                        _z_splitstr_t haystackcp = haystack;
                        while ((needle_found == true) && (n.start != NULL)) {
                            h = _z_splitstr_next(&haystackcp);
                            if (h.start != NULL) {
                                if (chunk_intersector(n, h) == false) {
                                    needle_found = false;
                                } else {
                                    n = _z_splitstr_next(&needlecp);
                                }
                            } else {
                                needle_found = false;
                                haystack.s = (_z_str_se_t){.start = NULL, .end = NULL};
                            }
                        }
                        if (needle_found == true) {
                            rchunks = haystackcp;
                        }
                    } else {
                        needle_found = false;
                    }
                    h = _z_splitstr_next(&haystack);
                }
            }
        } else if (((rwildness & (int8_t)_ZP_WILDNESS_SUPERCHUNKS) == 0) && (ln_chunks == rn_chunks)) {
            _z_splitstr_t lchunks = {.s = l, .delimiter = _Z_DELIMITER};
            _z_splitstr_t rchunks = {.s = r, .delimiter = _Z_DELIMITER};
            _z_str_se_t lchunk = _z_splitstr_next(&lchunks);
            _z_str_se_t rchunk = _z_splitstr_next(&rchunks);
            result = true;
            while ((result == true) && (lchunk.start != NULL)) {
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
    _Bool result = ((llen == rlen) && (strncmp(lstart, rstart, llen) == 0));
    if (result == false) {
        _z_str_se_t l = {.start = lstart, .end = _z_cptr_char_offset(lstart, llen)};
        _z_str_se_t r = {.start = rstart, .end = _z_cptr_char_offset(rstart, rlen)};
        size_t ln_chunks = 0;
        size_t rn_chunks = 0;
        int8_t lwildness = _zp_ke_wildness(l, &ln_chunks);
        int8_t rwildness = _zp_ke_wildness(r, &rn_chunks);
        int8_t wildness = lwildness | rwildness;
        _z_ke_chunk_matcher chunk_intersector =
            ((wildness & (int8_t)_ZP_WILDNESS_SUBCHUNK_DSL) == (int8_t)_ZP_WILDNESS_SUBCHUNK_DSL)
                ? _z_ke_chunk_intersect_stardsl
                : _z_ke_chunk_intersect_nodsl;
        if (wildness != (int8_t)0) {
            if ((lwildness & rwildness & (int8_t)_ZP_WILDNESS_SUPERCHUNKS) == (int8_t)_ZP_WILDNESS_SUPERCHUNKS) {
                // TODO: both expressions contain superchunks
                _z_splitstr_t lchunks = {.s = l, .delimiter = _Z_DELIMITER};
                _z_splitstr_t rchunks = {.s = r, .delimiter = _Z_DELIMITER};
                result = true;
                while (result == true) {
                    _z_str_se_t lchunk = _z_splitstr_next(&lchunks);
                    _z_str_se_t rchunk = _z_splitstr_next(&rchunks);
                    if ((lchunk.start != NULL) || (rchunk.start != NULL)) {
                        if ((_z_ke_isdoublestar(lchunk) == true) || (_z_ke_isdoublestar(rchunk) == true)) {
                            break;
                        } else if ((lchunk.start != NULL) && (rchunk.start != NULL)) {
                            result = chunk_intersector(lchunk, rchunk);
                        } else {
                            // Guaranteed not to happen, as both iterators contain a `**` which will break
                            // iteration.
                        }
                    } else {
                        // Guaranteed not to happen, as both iterators contain a `**` which will break iteration.
                    }
                }
                while (result == true) {
                    _z_str_se_t lchunk = _z_splitstr_nextback(&lchunks);
                    _z_str_se_t rchunk = _z_splitstr_nextback(&rchunks);
                    if ((lchunk.start != NULL) || (rchunk.start != NULL)) {
                        if ((_z_ke_isdoublestar(lchunk) == true) || (_z_ke_isdoublestar(rchunk) == true)) {
                            break;
                        } else if ((lchunk.start != NULL) && (rchunk.start != NULL)) {
                            result = chunk_intersector(lchunk, rchunk);
                        } else {
                            // Happens in cases such as `a / ** / b` with `a / ** / c / b`
                            break;
                        }
                    } else {
                        break;
                    }
                }
            } else if (((lwildness & (int8_t)_ZP_WILDNESS_SUPERCHUNKS) == (int8_t)_ZP_WILDNESS_SUPERCHUNKS) &&
                       (ln_chunks <= (rn_chunks * (size_t)2 + (size_t)1))) {
                result = _z_ke_intersect_rhassuperchunks(r, l, chunk_intersector);
            } else if (((rwildness & (int8_t)_ZP_WILDNESS_SUPERCHUNKS) == (int8_t)_ZP_WILDNESS_SUPERCHUNKS) &&
                       (rn_chunks <= (ln_chunks * (size_t)2 + (size_t)1))) {
                result = _z_ke_intersect_rhassuperchunks(l, r, chunk_intersector);
            } else if (ln_chunks == rn_chunks) {
                // no superchunks, just iterate and check chunk intersection
                _z_splitstr_t lchunks = {.s = l, .delimiter = _Z_DELIMITER};
                _z_splitstr_t rchunks = {.s = r, .delimiter = _Z_DELIMITER};
                _z_str_se_t lchunk = _z_splitstr_next(&lchunks);
                _z_str_se_t rchunk = _z_splitstr_next(&rchunks);
                result = true;
                while ((result == true) && (lchunk.start != NULL)) {
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
    zp_keyexpr_canon_status_t ret = __zp_canon_prefix(start, &canon_len);

    if ((ret == Z_KEYEXPR_CANON_LONE_DOLLAR_STAR) || (ret == Z_KEYEXPR_CANON_SINGLE_STAR_AFTER_DOUBLE_STAR) ||
        (ret == Z_KEYEXPR_CANON_DOUBLE_STAR_AFTER_DOUBLE_STAR)) {
        ret = Z_KEYEXPR_CANON_SUCCESS;

        const char *end = _z_cptr_char_offset(start, *len);
        char *reader = _z_ptr_char_offset(start, canon_len);
        const char *write_start = reader;
        char *writer = reader;
        char *next_slash = strchr(reader, '/');
        char const *chunk_end = (next_slash != NULL) ? next_slash : end;

        _Bool in_big_wild = false;
        if ((_z_ptr_char_diff(chunk_end, reader) == 2) && (reader[1] == '*')) {
            if (reader[0] == '*') {
                in_big_wild = true;
            } else if (reader[0] == '$') {
                writer[0] = '*';
                writer = _z_ptr_char_offset(writer, 1);
            } else {
                assert(false);  // anything before "$*" or "**" must be part of the canon prefix
            }
        } else {
            assert(false);  // anything before "$*" or "**" must be part of the canon prefix
        }

        while (next_slash != NULL) {
            reader = _z_ptr_char_offset(next_slash, 1);
            next_slash = strchr(reader, '/');
            chunk_end = next_slash ? next_slash : end;
            switch (_z_ptr_char_diff(chunk_end, reader)) {
                case 0: {
                    ret = Z_KEYEXPR_CANON_EMPTY_CHUNK;
                } break;

                case 1: {
                    if (reader[0] == '*') {
                        __zp_ke_write_chunk(&writer, "*", 1, write_start);
                        continue;
                    }
                } break;

                case 2: {
                    if (reader[1] == '*') {
                        if (reader[0] == '$') {
                            __zp_ke_write_chunk(&writer, "*", 1, write_start);
                            continue;
                        } else if (reader[0] == '*') {
                            in_big_wild = true;
                            continue;
                        } else {
                            // Do nothing. Required to be compliant with MISRA 15.7 rule
                        }
                    }
                } break;

                default:
                    break;
            }

            unsigned char in_dollar = 0;
            for (char const *c = reader; (c < end) && (ret == Z_KEYEXPR_CANON_SUCCESS); c = _z_cptr_char_offset(c, 1)) {
                switch (*c) {
                    case '#':
                    case '?': {
                        ret = Z_KEYEXPR_CANON_CONTAINS_SHARP_OR_QMARK;
                    } break;

                    case '$': {
                        if (in_dollar != (unsigned char)0) {
                            ret = Z_KEYEXPR_CANON_DOLLAR_AFTER_DOLLAR_OR_STAR;
                        } else {
                            in_dollar = in_dollar + (unsigned char)1;
                        }
                    } break;

                    case '*': {
                        if (in_dollar != (unsigned char)1) {
                            ret = Z_KEYEXPR_CANON_STARS_IN_CHUNK;
                        } else {
                            in_dollar = in_dollar + (unsigned char)2;
                        }
                    } break;

                    default: {
                        if (in_dollar == (unsigned char)1) {
                            ret = Z_KEYEXPR_CANON_CONTAINS_UNBOUND_DOLLAR;
                        } else {
                            in_dollar = 0;
                        }
                    } break;
                }
            }

            if ((ret == Z_KEYEXPR_CANON_SUCCESS) && (in_dollar == (unsigned char)1)) {
                ret = Z_KEYEXPR_CANON_CONTAINS_UNBOUND_DOLLAR;
            }

            if (ret == Z_KEYEXPR_CANON_SUCCESS) {
                if (in_big_wild) {
                    __zp_ke_write_chunk(&writer, _Z_DOUBLE_STAR, 2, write_start);
                }

                __zp_ke_write_chunk(&writer, reader, _z_ptr_char_diff(chunk_end, reader), write_start);
                in_big_wild = false;
            }
        }

        if (ret == Z_KEYEXPR_CANON_SUCCESS) {
            if (in_big_wild) {
                __zp_ke_write_chunk(&writer, _Z_DOUBLE_STAR, 2, write_start);
            }
            *len = _z_ptr_char_diff(writer, start);
        }
    }

    return ret;
}

zp_keyexpr_canon_status_t _z_keyexpr_is_canon(const char *start, size_t len) { return __zp_canon_prefix(start, &len); }
