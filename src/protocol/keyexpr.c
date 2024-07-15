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

#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/utils/pointers.h"
#include "zenoh-pico/utils/string.h"

_z_keyexpr_t _z_rname(const char *rname) { return _z_rid_with_suffix(0, rname); }

_z_keyexpr_t _z_rid_with_suffix(uint16_t rid, const char *suffix) {
    return (_z_keyexpr_t){
        ._id = rid,
        ._mapping = _z_keyexpr_mapping(_Z_KEYEXPR_MAPPING_LOCAL, false),
        ._suffix = (char *)suffix,
    };
}

int8_t _z_keyexpr_copy(_z_keyexpr_t *dst, const _z_keyexpr_t *src) {
    *dst = _z_keyexpr_null();
    dst->_suffix = src->_suffix ? _z_str_clone(src->_suffix) : NULL;
    if (dst->_suffix == NULL && src->_suffix != NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    dst->_id = src->_id;
    dst->_mapping = src->_mapping;
    _z_keyexpr_set_owns_suffix(dst, true);
    return _Z_RES_OK;
}

_z_keyexpr_t _z_keyexpr_duplicate(_z_keyexpr_t src) {
    _z_keyexpr_t dst;
    _z_keyexpr_copy(&dst, &src);
    return dst;
}

_z_keyexpr_t _z_keyexpr_to_owned(_z_keyexpr_t src) {
    return _z_keyexpr_owns_suffix(&src) ? src : _z_keyexpr_duplicate(src);
}

_z_keyexpr_t _z_keyexpr_steal(_Z_MOVE(_z_keyexpr_t) src) {
    _z_keyexpr_t stolen = *src;
    *src = _z_keyexpr_null();
    return stolen;
}

void _z_keyexpr_clear(_z_keyexpr_t *rk) {
    rk->_id = 0;
    if (rk->_suffix != NULL && _z_keyexpr_owns_suffix(rk)) {
        _z_str_clear((char *)rk->_suffix);
        _z_keyexpr_set_owns_suffix(rk, false);
    }
    rk->_suffix = NULL;
}

void _z_keyexpr_free(_z_keyexpr_t **rk) {
    _z_keyexpr_t *ptr = *rk;

    if (ptr != NULL) {
        _z_keyexpr_clear(ptr);

        z_free(ptr);
        *rk = NULL;
    }
}
_z_keyexpr_t _z_keyexpr_alias(_z_keyexpr_t src) {
    _z_keyexpr_t alias = {
        ._id = src._id,
        ._mapping = src._mapping,
        ._suffix = src._suffix,
    };
    _z_keyexpr_set_owns_suffix(&alias, false);
    return alias;
}

_z_keyexpr_t _z_keyexpr_alias_from_user_defined(_z_keyexpr_t src, _Bool try_declared) {
    if ((try_declared && src._id != Z_RESOURCE_ID_NONE) || src._suffix == NULL) {
        return (_z_keyexpr_t){
            ._id = src._id,
            ._mapping = src._mapping,
            ._suffix = NULL,
        };
    } else {
        return _z_rname(src._suffix);
    }
}

/*------------------ Canonize helpers ------------------*/
zp_keyexpr_canon_status_t __zp_canon_prefix(const char *start, size_t *len) {
    zp_keyexpr_canon_status_t ret = Z_KEYEXPR_CANON_SUCCESS;

    _Bool in_big_wild = false;
    char const *chunk_start = start;
    const char *end = _z_cptr_char_offset(start, (ptrdiff_t)(*len));
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
    const char *end = _z_cptr_char_offset(start, (ptrdiff_t)(*len));
    _Bool right_after_needle = false;
    char *reader = start;

    while (reader < end) {
        size_t pos = _z_str_startswith(reader, needle);
        if (pos != (size_t)0) {
            if (right_after_needle == true) {
                break;
            }
            right_after_needle = true;
            reader = _z_ptr_char_offset(reader, (ptrdiff_t)pos);
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
                writer = _z_ptr_char_offset(writer, (ptrdiff_t)pos);
            }
            right_after_needle = true;
            reader = _z_ptr_char_offset(reader, (ptrdiff_t)pos);
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
    writer[0] = _z_ptr_char_offset(writer[0], (ptrdiff_t)len);
}

/*------------------ Common helpers ------------------*/
typedef _Bool (*_z_ke_chunk_matcher)(_z_str_se_t l, _z_str_se_t r);

enum _zp_wildness_t { _ZP_WILDNESS_ANY = 1, _ZP_WILDNESS_SUPERCHUNKS = 2, _ZP_WILDNESS_SUBCHUNK_DSL = 4 };
int8_t _zp_ke_wildness(_z_str_se_t ke, size_t *n_segments, size_t *n_verbatims) {
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
            case '@': {
                *n_verbatims = *n_verbatims + (size_t)1;
            }
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
char _Z_VERBATIM = '@';
_Bool _z_ke_isdoublestar(_z_str_se_t s) {
    return ((_z_ptr_char_diff(s.end, s.start) == 2) && (s.start[0] == '*') && (s.start[1] == '*'));
}

/*------------------ Inclusion helpers ------------------*/
_Bool _z_ke_chunk_includes_nodsl(_z_str_se_t l, _z_str_se_t r) {
    size_t llen = (size_t)(l.end - l.start);
    _Bool result =
        !(r.start[0] == _Z_VERBATIM) && ((llen == (size_t)1) && (l.start[0] == '*') &&
                                         (((_z_ptr_char_diff(r.end, r.start) == 2) && (r.start[0] == '*')) == false));
    if ((result == false) && (llen == _z_ptr_char_diff(r.end, r.start))) {
        result = strncmp(l.start, r.start, llen) == 0;
    }

    return result;
}

_Bool _z_ke_chunk_includes_stardsl(_z_str_se_t l1, _z_str_se_t r) {
    _Bool result = _z_ke_chunk_includes_nodsl(l1, r);
    if (result == false && l1.start[0] != _Z_VERBATIM && r.start[0] != _Z_VERBATIM) {
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

_Bool _z_keyexpr_is_wild_chunk(_z_str_se_t s) { return _z_ptr_char_diff(s.end, s.start) == 1 && s.start[0] == '*'; }

_Bool _z_keyexpr_is_superwild_chunk(_z_str_se_t s) {
    return _z_ptr_char_diff(s.end, s.start) == 2 && s.start[0] == '*';
}

_Bool _z_keyexpr_has_verbatim(_z_str_se_t s) {
    _z_splitstr_t it = {.s = s, .delimiter = _Z_DELIMITER};
    _z_str_se_t chunk = _z_splitstr_next(&it);
    while (chunk.start != NULL) {
        if (chunk.start[0] == _Z_VERBATIM) {
            return true;
        }
        chunk = _z_splitstr_next(&it);
    }
    return false;
}

_Bool _z_keyexpr_includes_superwild(_z_str_se_t left, _z_str_se_t right, _z_ke_chunk_matcher chunk_includer) {
    for (;;) {
        _z_str_se_t lchunk = {0};
        _z_str_se_t lrest = _z_splitstr_split_once((_z_splitstr_t){.s = left, .delimiter = _Z_DELIMITER}, &lchunk);
        _Bool lempty = lrest.start == NULL;
        if (_z_keyexpr_is_superwild_chunk(lchunk)) {
            if (lempty ? !_z_keyexpr_has_verbatim(right)
                       : _z_keyexpr_includes_superwild(lrest, right, chunk_includer)) {
                return true;
            }
            if (right.start[0] == _Z_VERBATIM) {
                return false;
            }
            right = _z_splitstr_split_once((_z_splitstr_t){.s = right, .delimiter = _Z_DELIMITER}, &lrest);
            if (right.start == NULL) {
                return false;
            }
        } else {
            _z_str_se_t rchunk = {0};
            _z_str_se_t rrest = _z_splitstr_split_once((_z_splitstr_t){.s = right, .delimiter = _Z_DELIMITER}, &rchunk);
            if (rchunk.start == NULL || _z_keyexpr_is_superwild_chunk(rchunk) || !chunk_includer(lchunk, rchunk)) {
                return false;
            }
            if (lempty) {
                return rrest.start == NULL;
            }
            left = lrest;
            right = rrest;
        }
    }
}

/*------------------ Zenoh-Core helpers ------------------*/
_Bool _z_keyexpr_includes(const char *lstart, const size_t llen, const char *rstart, const size_t rlen) {
    _Bool result = ((llen == rlen) && (strncmp(lstart, rstart, llen) == 0));
    if (result == false) {
        _z_str_se_t l = {.start = lstart, .end = _z_cptr_char_offset(lstart, (ptrdiff_t)llen)};
        _z_str_se_t r = {.start = rstart, .end = _z_cptr_char_offset(rstart, (ptrdiff_t)rlen)};
        size_t ln_chunks = 0, ln_verbatim = 0;
        size_t rn_chunks = 0, rn_verbatim = 0;
        int8_t lwildness = _zp_ke_wildness(l, &ln_chunks, &ln_verbatim);
        int8_t rwildness = _zp_ke_wildness(r, &rn_chunks, &rn_verbatim);
        int8_t wildness = lwildness | rwildness;
        _z_ke_chunk_matcher chunk_includer =
            ((wildness & (int8_t)_ZP_WILDNESS_SUBCHUNK_DSL) == (int8_t)_ZP_WILDNESS_SUBCHUNK_DSL)
                ? _z_ke_chunk_includes_stardsl
                : _z_ke_chunk_includes_nodsl;
        if ((lwildness & (int8_t)_ZP_WILDNESS_SUPERCHUNKS) == (int8_t)_ZP_WILDNESS_SUPERCHUNKS) {
            return _z_keyexpr_includes_superwild(l, r, chunk_includer);
        } else if (((rwildness & (int8_t)_ZP_WILDNESS_SUPERCHUNKS) == 0) && (ln_chunks == rn_chunks)) {
            _z_splitstr_t lchunks = {.s = l, .delimiter = _Z_DELIMITER};
            _z_splitstr_t rchunks = {.s = r, .delimiter = _Z_DELIMITER};
            _z_str_se_t lchunk = _z_splitstr_next(&lchunks);
            _z_str_se_t rchunk = _z_splitstr_next(&rchunks);
            result = true;
            while ((result == true) && (lchunk.start != NULL)) {
                result = chunk_includer(lchunk, rchunk);
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

/*------------------ Intersection helpers ------------------*/
_Bool _z_ke_chunk_intersect_nodsl(_z_str_se_t l, _z_str_se_t r) {
    _Bool result =
        ((l.start[0] == '*' && r.start[0] != _Z_VERBATIM) || (r.start[0] == '*' && l.start[0] != _Z_VERBATIM));
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
    if (result == false && !(l.start[0] == '@' || r.start[0] == '@')) {
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
            needle_found = chunk_intersector(needle_start, h);
            if (needle_found == true) {
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
            } else if (h.start[0] == _Z_VERBATIM) {
                return false;
            }
            h = _z_splitstr_next(&haystack);
        }
    }
    for (_z_str_se_t chunk = _z_splitstr_next(&lchunks); chunk.start != NULL && result == true;
         chunk = _z_splitstr_next(&lchunks)) {
        if (chunk.start[0] == _Z_VERBATIM) {
            return false;
        }
    }
    return result;
}

_Bool _z_keyexpr_intersect_bothsuper(_z_str_se_t l, _z_str_se_t r, _z_ke_chunk_matcher chunk_intersector) {
    _z_splitstr_t it1 = {.s = l, .delimiter = _Z_DELIMITER};
    _z_splitstr_t it2 = {.s = r, .delimiter = _Z_DELIMITER};
    _z_str_se_t current1 = {0};
    _z_str_se_t current2 = {0};
    while (!_z_splitstr_is_empty(&it1) && !_z_splitstr_is_empty(&it2)) {
        _z_str_se_t advanced1 = _z_splitstr_split_once(it1, &current1);
        _z_str_se_t advanced2 = _z_splitstr_split_once(it2, &current2);
        if (_z_keyexpr_is_superwild_chunk(current1)) {
            if (advanced1.start == NULL) {
                return !_z_keyexpr_has_verbatim(it2.s);
            }
            return _z_keyexpr_intersect_bothsuper(advanced1, it2.s, chunk_intersector) ||
                   (current2.start[0] != _Z_VERBATIM &&
                    _z_keyexpr_intersect_bothsuper(it1.s, advanced2, chunk_intersector));
        } else if (_z_keyexpr_is_superwild_chunk(current2)) {
            if (advanced2.start == NULL) {
                return !_z_keyexpr_has_verbatim(it1.s);
            }
            return _z_keyexpr_intersect_bothsuper(advanced2, it1.s, chunk_intersector) ||
                   (current1.start[0] != _Z_VERBATIM &&
                    _z_keyexpr_intersect_bothsuper(it2.s, advanced1, chunk_intersector));
        } else if (chunk_intersector(current1, current2)) {
            it1.s = advanced1;
            it2.s = advanced2;
        } else {
            return false;
        }
    }
    return (_z_splitstr_is_empty(&it1) || _z_keyexpr_is_superwild_chunk(it1.s)) &&
           (_z_splitstr_is_empty(&it2) || _z_keyexpr_is_superwild_chunk(it2.s));
}

_Bool _z_keyexpr_intersects(const char *lstart, const size_t llen, const char *rstart, const size_t rlen) {
    _Bool result = ((llen == rlen) && (strncmp(lstart, rstart, llen) == 0));
    if (result == false) {
        _z_str_se_t l = {.start = lstart, .end = _z_cptr_char_offset(lstart, (ptrdiff_t)llen)};
        _z_str_se_t r = {.start = rstart, .end = _z_cptr_char_offset(rstart, (ptrdiff_t)rlen)};
        size_t ln_chunks = 0, ln_verbatim = 0;
        size_t rn_chunks = 0, rn_verbatim = 0;
        int8_t lwildness = _zp_ke_wildness(l, &ln_chunks, &ln_verbatim);
        int8_t rwildness = _zp_ke_wildness(r, &rn_chunks, &rn_verbatim);
        int8_t wildness = lwildness | rwildness;
        _z_ke_chunk_matcher chunk_intersector =
            ((wildness & (int8_t)_ZP_WILDNESS_SUBCHUNK_DSL) == (int8_t)_ZP_WILDNESS_SUBCHUNK_DSL)
                ? _z_ke_chunk_intersect_stardsl
                : _z_ke_chunk_intersect_nodsl;
        if (wildness != (int8_t)0 && rn_verbatim == ln_verbatim) {
            if ((lwildness & rwildness & (int8_t)_ZP_WILDNESS_SUPERCHUNKS) == (int8_t)_ZP_WILDNESS_SUPERCHUNKS) {
                result = _z_keyexpr_intersect_bothsuper(l, r, chunk_intersector);
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
            // No string equality and no wildness detected, or different count of verbatim chunks: no intersection
            // guaranteed.
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

        const char *end = _z_cptr_char_offset(start, (ptrdiff_t)(*len));
        char *reader = _z_ptr_char_offset(start, (ptrdiff_t)canon_len);
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
