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

#include "zenoh-pico/session/keyexpr.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "zenoh-pico/net/primitives.h"
#include "zenoh-pico/net/session.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"
#include "zenoh-pico/utils/string.h"

z_result_t _z_keyexpr_wire_declaration_new(_z_keyexpr_wire_declaration_t *declaration, const _z_string_t *keyexpr,
                                           const _z_session_rc_t *session) {
    *declaration = _z_keyexpr_wire_declaration_null();
    z_result_t ret = _z_declare_resource(_Z_RC_IN_VAL(session), keyexpr, &declaration->_id);
    if (ret == _Z_RES_OK) {
        declaration->_prefix_len = (uint16_t)_z_string_len(keyexpr);
        declaration->_session = _z_session_rc_clone_as_weak(session);
    }
    return ret;
}

z_result_t _z_keyexpr_wire_declaration_undeclare(_z_keyexpr_wire_declaration_t *declaration) {
    z_result_t ret = _Z_RES_OK;
    if (_Z_RC_IS_NULL(&declaration->_session)) {
        return ret;
    }

    _z_session_rc_t session_rc = _z_session_weak_upgrade_if_open(&declaration->_session);

    if (!_Z_RC_IS_NULL(&session_rc)) {
        ret = _z_undeclare_resource(_Z_RC_IN_VAL(&session_rc), declaration->_id);
        _z_session_rc_drop(&session_rc);
    }
    declaration->_id = Z_RESOURCE_ID_NONE;
    declaration->_prefix_len = 0;
    _z_session_weak_drop(&declaration->_session);
    return ret;
}

void _z_keyexpr_wire_declaration_clear(_z_keyexpr_wire_declaration_t *declaration) {
    _z_keyexpr_wire_declaration_undeclare(declaration);
}

_z_keyexpr_t _z_keyexpr_alias_from_string(const _z_string_t *str) {
    _z_keyexpr_t ke = _z_keyexpr_null();
    ke._keyexpr = _z_string_alias(*str);
    return ke;
}

_z_keyexpr_t _z_keyexpr_alias_from_substr(const char *str, size_t len) {
    _z_keyexpr_t ke = _z_keyexpr_null();
    ke._keyexpr = _z_string_alias_substr(str, len);
    return ke;
}

_z_declared_keyexpr_t _z_declared_keyexpr_alias_from_string(const _z_string_t *str) {
    _z_declared_keyexpr_t ke = _z_declared_keyexpr_null();
    ke._inner = _z_keyexpr_alias_from_string(str);
    return ke;
}

_z_declared_keyexpr_t _z_declared_keyexpr_alias_from_substr(const char *str, size_t len) {
    _z_declared_keyexpr_t ke = _z_declared_keyexpr_null();
    ke._inner = _z_keyexpr_alias_from_substr(str, len);
    return ke;
}

z_result_t _z_keyexpr_from_string(_z_keyexpr_t *dst, const _z_string_t *str) {
    return _z_keyexpr_from_substr(dst, _z_string_data(str), _z_string_len(str));
}

z_result_t _z_keyexpr_from_substr(_z_keyexpr_t *dst, const char *str, size_t len) {
    *dst = _z_keyexpr_null();
    dst->_keyexpr = _z_string_copy_from_substr(str, len);
    return _z_string_check(&dst->_keyexpr) ? _Z_RES_OK : _Z_ERR_SYSTEM_OUT_OF_MEMORY;
}

z_result_t _z_declared_keyexpr_copy(_z_declared_keyexpr_t *dst, const _z_declared_keyexpr_t *src) {
    *dst = _z_declared_keyexpr_null();
    _Z_RETURN_IF_ERR(_z_keyexpr_copy(&dst->_inner, &src->_inner));
    if (!_Z_RC_IS_NULL(&src->_declaration)) {
        dst->_declaration = _z_keyexpr_wire_declaration_rc_clone(&src->_declaration);
    }
    return _Z_RES_OK;
}

z_result_t _z_declared_keyexpr_move(_z_declared_keyexpr_t *dst, _z_declared_keyexpr_t *src) {
    *dst = _z_declared_keyexpr_null();
    _Z_RETURN_IF_ERR(_z_keyexpr_move(&dst->_inner, &src->_inner));
    dst->_declaration = src->_declaration;
    src->_declaration = _z_keyexpr_wire_declaration_rc_null();
    return _Z_RES_OK;
}

/*------------------ Canonize helpers ------------------*/
zp_keyexpr_canon_status_t __zp_canon_prefix(const char *start, size_t *len) {
    zp_keyexpr_canon_status_t ret = Z_KEYEXPR_CANON_SUCCESS;

    bool in_big_wild = false;
    char const *chunk_start = start;
    const char *end = _z_cptr_char_offset(start, (ptrdiff_t)(*len));
    char const *next_slash;

    do {
        next_slash = memchr(chunk_start, '/', _z_ptr_char_diff(end, chunk_start));
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
    bool right_after_needle = false;
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

    (void)memmove(writer[0], chunk, len);
    writer[0] = _z_ptr_char_offset(writer[0], (ptrdiff_t)len);
}

/*------------------ Common helpers ------------------*/
typedef bool (*_z_ke_chunk_matcher)(_z_str_se_t l, _z_str_se_t r);

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
const char _Z_VERBATIM = '@';
const char _Z_SEPARATOR = '/';
const char _Z_STAR = '*';
const char _Z_DSL0 = '$';
const char _Z_DSL1 = '*';
const size_t _Z_DELIMITER_LEN = 1;
const size_t _Z_DSL_LEN = 2;
const size_t _Z_DOUBLE_STAR_LEN = 2;

bool _z_ke_isdoublestar(_z_str_se_t s) {
    return ((_z_ptr_char_diff(s.end, s.start) == 2) && (s.start[0] == '*') && (s.start[1] == '*'));
}

/*------------------ Inclusion helpers ------------------*/
bool _z_ke_chunk_includes_nodsl(_z_str_se_t l, _z_str_se_t r) {
    size_t llen = (size_t)(l.end - l.start);
    bool result =
        !(r.start[0] == _Z_VERBATIM) && ((llen == (size_t)1) && (l.start[0] == '*') &&
                                         (((_z_ptr_char_diff(r.end, r.start) == 2) && (r.start[0] == '*')) == false));
    if ((result == false) && (llen == _z_ptr_char_diff(r.end, r.start))) {
        result = strncmp(l.start, r.start, llen) == 0;
    }

    return result;
}

bool _z_ke_chunk_includes_stardsl(_z_str_se_t l1, _z_str_se_t r) {
    bool result = _z_ke_chunk_includes_nodsl(l1, r);
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

bool _z_declared_keyexpr_is_wild_chunk(_z_str_se_t s) {
    return _z_ptr_char_diff(s.end, s.start) == 1 && s.start[0] == '*';
}

bool _z_declared_keyexpr_is_superwild_chunk(_z_str_se_t s) {
    return _z_ptr_char_diff(s.end, s.start) == 2 && s.start[0] == '*';
}

bool _z_declared_keyexpr_has_verbatim(_z_str_se_t s) {
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

bool _z_declared_keyexpr_includes_superwild(_z_str_se_t left, _z_str_se_t right, _z_ke_chunk_matcher chunk_includer) {
    for (;;) {
        _z_str_se_t lchunk = {0};
        _z_str_se_t lrest = _z_splitstr_split_once((_z_splitstr_t){.s = left, .delimiter = _Z_DELIMITER}, &lchunk);
        bool lempty = lrest.start == NULL;
        if (_z_declared_keyexpr_is_superwild_chunk(lchunk)) {
            if (lempty ? !_z_declared_keyexpr_has_verbatim(right)
                       : _z_declared_keyexpr_includes_superwild(lrest, right, chunk_includer)) {
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
            if (rchunk.start == NULL || _z_declared_keyexpr_is_superwild_chunk(rchunk) ||
                !chunk_includer(lchunk, rchunk)) {
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

        bool in_big_wild = false;
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
            next_slash = memchr(reader, '/', _z_ptr_char_diff(end, reader));
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

z_result_t _z_keyexpr_concat(_z_keyexpr_t *key, const _z_keyexpr_t *left, const char *right, size_t len) {
    *key = _z_keyexpr_null();
    size_t left_len = _z_string_len(&left->_keyexpr);
    if (left_len == 0) {
        return _z_keyexpr_from_substr(key, right, len);
    }
    const char *left_data = _z_string_data(&left->_keyexpr);

    if (left_data[left_len - 1] == '*' && len > 0 && right[0] == '*') {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }
    _Z_RETURN_IF_ERR(_z_string_concat_substr(&key->_keyexpr, &left->_keyexpr, right, len, NULL, 0));

    return _Z_RES_OK;
}

z_result_t _z_declared_keyexpr_concat(_z_declared_keyexpr_t *key, const _z_declared_keyexpr_t *left, const char *right,
                                      size_t len) {
    *key = _z_declared_keyexpr_null();
    _Z_RETURN_IF_ERR(_z_keyexpr_concat(&key->_inner, &left->_inner, right, len));
    if (!_Z_RC_IS_NULL(&left->_declaration)) {
        key->_declaration = _z_keyexpr_wire_declaration_rc_clone(&left->_declaration);
    }
    return _Z_RES_OK;
}

z_result_t _z_keyexpr_join(_z_keyexpr_t *key, const _z_keyexpr_t *left, const _z_keyexpr_t *right) {
    *key = _z_keyexpr_null();

    _Z_RETURN_IF_ERR(_z_string_concat_substr(&key->_keyexpr, &left->_keyexpr, _z_string_data(&right->_keyexpr),
                                             _z_string_len(&right->_keyexpr), "/", 1));
    _Z_CLEAN_RETURN_IF_ERR(_z_keyexpr_canonize((char *)key->_keyexpr._slice.start, &key->_keyexpr._slice.len),
                           _z_keyexpr_clear(key));
    return _Z_RES_OK;
}

z_result_t _z_declared_keyexpr_join(_z_declared_keyexpr_t *key, const _z_declared_keyexpr_t *left,
                                    const _z_declared_keyexpr_t *right) {
    *key = _z_declared_keyexpr_null();
    _Z_RETURN_IF_ERR(_z_keyexpr_join(&key->_inner, &left->_inner, &right->_inner));
    if (!_Z_RC_IS_NULL(&left->_declaration)) {
        key->_declaration = _z_keyexpr_wire_declaration_rc_clone(&left->_declaration);
    }
    return _Z_RES_OK;
}

_z_wireexpr_t _z_declared_keyexpr_alias_to_wire(const _z_declared_keyexpr_t *key, const _z_session_t *session) {
    _z_wireexpr_t expr = _z_wireexpr_null();
    const char *suffix_str = _z_string_data(&key->_inner._keyexpr);
    size_t suffix_len = _z_string_len(&key->_inner._keyexpr);
    if (!_Z_RC_IS_NULL(&key->_declaration) &&
        _z_keyexpr_wire_declaration_is_declared_on_session(_Z_RC_IN_VAL(&key->_declaration), session)) {
        expr._id = _Z_RC_IN_VAL(&key->_declaration)->_id;
        expr._mapping = _Z_KEYEXPR_MAPPING_LOCAL;
        suffix_str = suffix_str + _Z_RC_IN_VAL(&key->_declaration)->_prefix_len;
        suffix_len -= _Z_RC_IN_VAL(&key->_declaration)->_prefix_len;
    }
    if (suffix_len > 0) {
        expr._suffix = _z_string_alias_substr(suffix_str, suffix_len);
    }
    return expr;
}

_z_wireexpr_t _z_keyexpr_alias_to_wire(const _z_keyexpr_t *key) {
    _z_wireexpr_t expr = _z_wireexpr_null();
    expr._suffix = _z_string_alias(key->_keyexpr);
    return expr;
}

z_result_t _z_keyexpr_declare_prefix(const _z_session_rc_t *zs, _z_declared_keyexpr_t *out, const _z_keyexpr_t *keyexpr,
                                     size_t prefix_len) {
    assert(prefix_len <= _z_string_len(&keyexpr->_keyexpr));
    *out = _z_declared_keyexpr_null();
    _Z_RETURN_IF_ERR(_z_string_copy(&out->_inner._keyexpr, &keyexpr->_keyexpr));
    if (prefix_len == 0) {
        return _Z_RES_OK;
    }
#if Z_FEATURE_MULTICAST_DECLARATIONS == 0
    if (_Z_RC_IN_VAL(zs)->_tp._type == _Z_TRANSPORT_MULTICAST_TYPE) {
        // Skip declaration since declaring a keyexpr without Z_FEATURE_MULTICAST_DECLARATIONS might generate unknown
        // key expression errors.
        return _Z_RES_OK;
    }
#endif
    _z_keyexpr_wire_declaration_t declaration = _z_keyexpr_wire_declaration_null();
    out->_declaration = _z_keyexpr_wire_declaration_rc_new_from_val(&declaration);
    if (_Z_RC_IS_NULL(&out->_declaration)) {
        _z_declared_keyexpr_clear(out);
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    _z_string_t prefix = _z_string_alias_substr(_z_string_data(&out->_inner._keyexpr), prefix_len);
    _Z_CLEAN_RETURN_IF_ERR(_z_keyexpr_wire_declaration_new(_Z_RC_IN_VAL(&out->_declaration), &prefix, zs),
                           _z_declared_keyexpr_clear(out));
    return _Z_RES_OK;
}

z_result_t _z_declared_keyexpr_declare(const _z_session_rc_t *zs, _z_declared_keyexpr_t *out,
                                       const _z_declared_keyexpr_t *keyexpr) {
    if (_z_declared_keyexpr_is_fully_optimized(keyexpr, _Z_RC_IN_VAL(zs))) {
        return _z_declared_keyexpr_copy(out, keyexpr);
    } else {
        return _z_keyexpr_declare_prefix(zs, out, &keyexpr->_inner, _z_string_len(&keyexpr->_inner._keyexpr));
    }
}

size_t _z_keyexpr_non_wild_prefix_len(const _z_keyexpr_t *key) {
    const char *data = _z_string_data(&key->_keyexpr);
    size_t len = _z_string_len(&key->_keyexpr);
    char *pos = (char *)memchr(data, '*', len);
    if (pos == NULL) {
        return len;
    }
    while (pos != data && *pos != '/') {
        pos--;
    }

    return _z_ptr_char_diff(pos, data);
}

z_result_t _z_declared_keyexpr_declare_non_wild_prefix(const _z_session_rc_t *zs, _z_declared_keyexpr_t *out,
                                                       const _z_declared_keyexpr_t *keyexpr) {
    if (_z_declared_keyexpr_is_non_wild_prefix_optimized(keyexpr, _Z_RC_IN_VAL(zs))) {
        return _z_declared_keyexpr_copy(out, keyexpr);
    } else {
        return _z_keyexpr_declare_prefix(zs, out, &keyexpr->_inner, _z_keyexpr_non_wild_prefix_len(&keyexpr->_inner));
    }
}

typedef enum _z_chunk_match_result_t {
    _Z_CHUNK_MATCH_RESULT_NO,
    _Z_CHUNK_MATCH_RESULT_YES,
    _Z_CHUNK_MATCH_RESULT_LEFT_SUPERWILD,
    _Z_CHUNK_MATCH_RESULT_RIGHT_SUPERWILD,
} _z_chunk_match_result_t;

typedef struct _z_chunk_forward_match_data_t {
    _z_chunk_match_result_t result;
    const char *lend;  // only valid if result is _Z_CHUNK_MATCH_RESULT_YES
    const char *rend;  // only valid if result is _Z_CHUNK_MATCH_RESULT_YES
} _z_chunk_forward_match_data_t;

typedef struct _z_chunk_backward_match_data_t {
    _z_chunk_match_result_t result;
    const char *lbegin;  // only valid if result is _Z_CHUNK_MATCH_RESULT_YES
    const char *rbegin;  // only valid if result is _Z_CHUNK_MATCH_RESULT_YES
} _z_chunk_backward_match_data_t;

static inline const char *_z_chunk_end(const char *begin, const char *end) {
    const char *sep = (const char *)memchr(begin, _Z_SEPARATOR, (size_t)(end - begin));
    return sep ? sep : end;
}

static inline const char *_z_chunk_begin(const char *begin, const char *end) {
    const char *last = end - 1;
    while (begin <= last && *last != _Z_SEPARATOR) {
        last--;
    }
    return last + 1;
}

static inline bool _z_keyexpr_is_double_star(const char *begin, const char *end) {
    size_t len = (size_t)(end - begin);
    return len == 2 && begin[0] == _Z_STAR && begin[1] == _Z_STAR;
}

static inline bool _z_chunk_contains_stardsl(const char *begin, const char *end) {
    return memchr(begin, _Z_DSL0, (size_t)(end - begin)) != NULL;
}

static bool _z_chunk_right_contains_all_stardsl_subchunks_of_left(const char *lbegin, const char *lend,
                                                                  const char *rbegin, const char *rend) {
    // Check if right chunk contains all stardsl subchunks of left chunk
    const char *next_dsl = NULL;
    bool matched = false;
    do {
        size_t llen = (size_t)(lend - lbegin);
        next_dsl = memchr(lbegin, _Z_DSL0, llen);
        size_t clen = next_dsl ? (size_t)(next_dsl - lbegin) : llen;
        // TODO: Should we consider a more optimial substring search algorithm, like Boyer-Moore ? The simple version
        // below looks good enough if the chunks are short.
        matched = false;
        while (rbegin + clen <= rend) {
            if (memcmp(rbegin, lbegin, clen) == 0) {
                rbegin += clen;
                matched = true;
                break;
            }
            rbegin++;
        }
        lbegin += clen + _Z_DSL_LEN;  // skip the matched part and the following stardsl
    } while (matched && next_dsl != NULL);
    return matched;
}

bool _z_chunk_intersects_special(const char *lbegin, const char *lend, const char *rbegin, const char *rend) {
    // left is a non-empty part of a chunk following a stardsl and preceeding a stardsl i.e. $*lllll$*
    // right is chunk that does not start, nor end with a stardsl(s), but can contain stardsl in the middle
    // original left chunk has the following form $*L1$*L2$*...$*LN$*
    // so there are only 2 cases where it can intersect with right:
    // 1. right contains a stardsl in the middle, in this case bound stardsls in left can match parts before and after
    // stardsl in the right, while right's stardsl can match the middle part of left
    //
    // 2. right does not contain a stardsl, but every subchunk of left (L1, L2, ..., LN) is present in right in the
    // correct order (and they do not overlap).

    return _z_chunk_contains_stardsl(rbegin, rend) ||
           _z_chunk_right_contains_all_stardsl_subchunks_of_left(lbegin, lend, rbegin, rend);
}

static inline bool _z_chunk_is_stardsl(const char *begin, const char *kend) {
    return *begin == _Z_DSL0 && (begin + _Z_DSL_LEN == kend || *(begin + 2) == _Z_SEPARATOR);
}

static inline bool _z_chunk_is_stardsl_reverse(const char *kbegin, const char *last) {
    // does not test for **
    return *last == _Z_DSL1 && (last == kbegin + 1 || (last > kbegin + 1 && *(last - 2) == _Z_SEPARATOR));
}

_z_chunk_forward_match_data_t _z_chunk_intersects_forward_backward(const char *lbegin, const char *lend,
                                                                   const char *rbegin, const char *rend) {
    // left is a part of a chunk following a stardsl. i.e $*llllll
    _z_chunk_forward_match_data_t result = {0};
    result.lend = _z_chunk_end(lbegin, lend);
    result.rend = _z_chunk_end(rbegin, rend);

    lend = result.lend;
    rend = result.rend;

    while (lend > lbegin && rend > rbegin) {
        lend--;
        rend--;
        if (*rend == _Z_DSL1) {
            result.result = _Z_CHUNK_MATCH_RESULT_YES;
            return result;
        } else if (*lend == _Z_DSL1) {
            result.result = _z_chunk_intersects_special(lbegin, lend + 1 - _Z_DSL_LEN, rbegin, rend + 1)
                                ? _Z_CHUNK_MATCH_RESULT_YES
                                : _Z_CHUNK_MATCH_RESULT_NO;
            return result;
        } else if (*lend != *rend) {
            result.result = _Z_CHUNK_MATCH_RESULT_NO;
            return result;
        }
    }
    if (lbegin == lend) {  // remainder of left is entirely matched, leading stardsl can match the rest of right
        result.result = _Z_CHUNK_MATCH_RESULT_YES;
    } else {  // rbegin == rend remainder of right is entirely matched, but there are still chars in left (which are not
              // strardsl due to canonicalization), so no match
        result.result = _Z_CHUNK_MATCH_RESULT_NO;
    }
    return result;
}

_z_chunk_forward_match_data_t _z_chunk_intersects_forward(const char *lbegin, const char *lkend, const char *rbegin,
                                                          const char *rkend) {
    // left and right are guaranteed to be non-empty canonized chunks
    _z_chunk_forward_match_data_t result = {0};
    // assume canonized chunks
    bool left_is_wild = *lbegin == _Z_STAR;
    bool right_is_wild = *rbegin == _Z_STAR;

    bool left_is_superwild = left_is_wild && (lbegin + 2 <= lkend) && *(lbegin + 1) == _Z_STAR;
    bool right_is_superwild = right_is_wild && (rbegin + 2 <= rkend) && *(rbegin + 1) == _Z_STAR;

    if (left_is_superwild) {
        result.result = _Z_CHUNK_MATCH_RESULT_LEFT_SUPERWILD;
        return result;
    } else if (right_is_superwild) {
        result.result = _Z_CHUNK_MATCH_RESULT_RIGHT_SUPERWILD;
        return result;
    }
    bool left_is_verbatim = *lbegin == _Z_VERBATIM;
    bool right_is_verbatim = *rbegin == _Z_VERBATIM;

    if (left_is_verbatim != right_is_verbatim) {
        result.result = _Z_CHUNK_MATCH_RESULT_NO;
        return result;
    } else if (left_is_wild) {
        result.result = _Z_CHUNK_MATCH_RESULT_YES;
        result.lend = lbegin + 1;
        result.rend = _z_chunk_end(rbegin, rkend);
        return result;
    } else if (right_is_wild) {
        result.result = _Z_CHUNK_MATCH_RESULT_YES;
        result.lend = _z_chunk_end(lbegin, lkend);
        result.rend = rbegin + 1;
        return result;
    }
    // at this stage we should only care about stardsl, as the presence of verbatim or wild is already checked.
    while (lbegin < lkend && rbegin < rkend && *lbegin != _Z_SEPARATOR && *rbegin != _Z_SEPARATOR) {
        if (*lbegin == _Z_DSL0) {
            return _z_chunk_intersects_forward_backward(lbegin + _Z_DSL_LEN, lkend, rbegin, rkend);
        } else if (*rbegin == _Z_DSL0) {
            _z_chunk_forward_match_data_t res =
                _z_chunk_intersects_forward_backward(rbegin + _Z_DSL_LEN, rkend, lbegin, lkend);
            result.lend = res.lend;
            result.rend = res.rend;
            result.result = res.result;
            return result;
        }
        if (*lbegin != *rbegin) {
            result.result = _Z_CHUNK_MATCH_RESULT_NO;
            return result;
        }
        lbegin++;
        rbegin++;
    }
    bool left_exhausted = lbegin == lkend || *lbegin == _Z_SEPARATOR;
    bool right_exhausted = rbegin == rkend || *rbegin == _Z_SEPARATOR;
    if (left_exhausted && right_exhausted) {
        result.result = _Z_CHUNK_MATCH_RESULT_YES;
        result.lend = lbegin;
        result.rend = rbegin;
    } else if (left_exhausted && _z_chunk_is_stardsl(rbegin, rkend)) {
        result.result = _Z_CHUNK_MATCH_RESULT_YES;
        result.lend = lbegin;
        result.rend = rbegin + _Z_DSL_LEN;
    } else if (right_exhausted && _z_chunk_is_stardsl(lbegin, lkend)) {
        result.result = _Z_CHUNK_MATCH_RESULT_YES;
        result.lend = lbegin + _Z_DSL_LEN;
        result.rend = rbegin;
    } else {
        result.result = _Z_CHUNK_MATCH_RESULT_NO;
    }
    return result;
}

_z_chunk_backward_match_data_t _z_chunk_intersects_backward_forward(const char *lbegin, const char *lend,
                                                                    const char *rbegin, const char *rend) {
    // left is a part of a chunk preceeding a stardsl i.e lllll$*
    _z_chunk_backward_match_data_t result = {0};
    lbegin = _z_chunk_begin(lbegin, lend);
    rbegin = _z_chunk_begin(rbegin, rend);
    result.lbegin = lbegin;
    result.rbegin = rbegin;

    // chunks can not be star, nor doublestar
    while (lbegin < lend && rbegin < rend) {
        if (*rbegin == _Z_DSL0) {
            result.result = _Z_CHUNK_MATCH_RESULT_YES;
            return result;
        } else if (*lbegin == _Z_DSL0) {
            result.result = _z_chunk_intersects_special(lbegin + 2, lend, rbegin, rend) ? _Z_CHUNK_MATCH_RESULT_YES
                                                                                        : _Z_CHUNK_MATCH_RESULT_NO;
            return result;
        } else if (*lbegin != *rbegin) {
            result.result = _Z_CHUNK_MATCH_RESULT_NO;
            return result;
        }
        lbegin++;
        rbegin++;
    }

    if (lbegin == lend) {  // remainder of left is entirely matched, suffix stardsl can match the rest of right
        result.result = _Z_CHUNK_MATCH_RESULT_YES;
    }
    return result;
}

_z_chunk_backward_match_data_t _z_chunk_intersects_backward(const char *lkbegin, const char *lend, const char *rkbegin,
                                                            const char *rend) {
    _z_chunk_backward_match_data_t result = {0};
    const char *llast = lend - 1;
    const char *rlast = rend - 1;
    // assume canonized chunks
    bool left_is_wild = *llast == _Z_STAR;
    bool right_is_wild = *rlast == _Z_STAR;

    bool left_is_superwild = left_is_wild && (lkbegin + 2 <= lend) && *(llast - 1) == _Z_STAR;
    bool right_is_superwild = right_is_wild && (rkbegin + 2 <= rend) && *(rlast - 1) == _Z_STAR;

    if (left_is_superwild) {
        result.result = _Z_CHUNK_MATCH_RESULT_LEFT_SUPERWILD;
        return result;
    } else if (right_is_superwild) {
        result.result = _Z_CHUNK_MATCH_RESULT_RIGHT_SUPERWILD;
        return result;
    }
    left_is_wild = left_is_wild && (lkbegin == llast || *(llast - 1) == _Z_SEPARATOR);
    right_is_wild = right_is_wild && (rkbegin == rlast || *(rlast - 1) == _Z_SEPARATOR);
    if (left_is_wild) {
        result.lbegin = llast;
        result.rbegin = _z_chunk_begin(rkbegin, rend);
        result.result = *result.rbegin == _Z_VERBATIM ? _Z_CHUNK_MATCH_RESULT_NO : _Z_CHUNK_MATCH_RESULT_YES;
        return result;
    }
    if (right_is_wild) {
        result.lbegin = _z_chunk_begin(lkbegin, lend);
        result.rbegin = rlast;
        result.result = *result.lbegin == _Z_VERBATIM ? _Z_CHUNK_MATCH_RESULT_NO : _Z_CHUNK_MATCH_RESULT_YES;
        return result;
    }

    while (llast >= lkbegin && rlast >= rkbegin && *llast != _Z_SEPARATOR && *rlast != _Z_SEPARATOR) {
        if (*llast == _Z_DSL1) {
            return _z_chunk_intersects_backward_forward(lkbegin, llast + 1 - _Z_DSL_LEN, rkbegin, rlast + 1);
        } else if (*rlast == _Z_DSL1) {
            _z_chunk_backward_match_data_t res =
                _z_chunk_intersects_backward_forward(rkbegin, rlast + 1 - _Z_DSL_LEN, lkbegin, llast + 1);
            result.lbegin = res.rbegin;
            result.rbegin = res.lbegin;
            result.result = res.result;
            return result;
        }
        if (*llast != *rlast) {
            result.result = _Z_CHUNK_MATCH_RESULT_NO;
            return result;
        }
        llast--;
        rlast--;
    }
    bool left_exhausted = lkbegin == llast + 1 || *llast == _Z_SEPARATOR;
    bool right_exhausted = rkbegin == rlast + 1 || *rlast == _Z_SEPARATOR;
    if (left_exhausted && right_exhausted) {
        result.result = _Z_CHUNK_MATCH_RESULT_YES;
        result.lbegin = llast + 1;
        result.rbegin = rlast + 1;
    } else if (left_exhausted && _z_chunk_is_stardsl_reverse(rkbegin, rlast)) {
        result.result = _Z_CHUNK_MATCH_RESULT_YES;
        result.lbegin = llast + 1;
        result.rbegin = rlast + 1 - _Z_DSL_LEN;
    } else if (right_exhausted && _z_chunk_is_stardsl_reverse(lkbegin, llast)) {
        result.result = _Z_CHUNK_MATCH_RESULT_YES;
        result.lbegin = llast + 1 - _Z_DSL_LEN;
        result.rbegin = rlast + 1;
    }
    return result;
}

static inline const char *_z_keyexpr_get_next_double_star_chunk(const char *begin, const char *end) {
    while (begin + 1 < end) {
        const char *c = (const char *)memchr(begin, _Z_STAR, (size_t)(end - begin - 1));
        if (c == NULL) {
            return NULL;
        }
        if (*(c + 1) == _Z_STAR) {
            return c;
        }
        begin = c + _Z_DOUBLE_STAR_LEN;
    }
    return NULL;
}

static inline const char *_z_keyexpr_get_next_verbatim_chunk(const char *begin, const char *end) {
    return (const char *)memchr(begin, _Z_VERBATIM, (size_t)(end - begin));
}

bool _z_keyexpr_intersects_special(const char *lbegin, const char *lend, const char *rbegin, const char *rend) {
    // left is a non-empty part of a ke sorrounded by doublestar i.e. **/l/l/l/l**
    // right is ke that does not start, nor end with a doublestar(s), but can contain doublestar in the middle
    // none of the kes contain any verbatim chunks
    // so there are only 2 cases where left can intersect with right:
    // 1. right contains a doublestar in the middle, in this case bound doublestars in left can match parts before and
    // after doublestar in right, while right's doublestar can match the middle part of left
    //
    // 2. right does not contain a doublestar, but every chunk of left **/L1/**/L2/**/../LN/** is matched in ke in right
    // in the correct order and without overlap.
    if (_z_keyexpr_get_next_double_star_chunk(rbegin, rend) != NULL) {
        return true;
    }

    // right is guaranteed not to have doublestars
    while (lbegin < lend) {
        const char *lcbegin = lbegin;
        const char *lcend = _z_keyexpr_get_next_double_star_chunk(lbegin, lend);
        lcend = lcend != NULL ? lcend - _Z_DELIMITER_LEN : lend;
        const char *rcbegin = rbegin;

        while (lcbegin < lcend) {
            if (rcbegin >= rend) {
                return false;
            }
            _z_chunk_forward_match_data_t res = _z_chunk_intersects_forward(lcbegin, lcend, rcbegin, rend);
            if (res.result == _Z_CHUNK_MATCH_RESULT_YES) {
                lcbegin = res.lend + _Z_DELIMITER_LEN;
                rcbegin = res.rend + _Z_DELIMITER_LEN;
            } else {  // res.result == _Z_CHUNK_MATCH_RESULT_NO
                // reset subke matching and try to match current left subke with the next right chunk
                lcbegin = lbegin;
                rbegin = _z_chunk_end(rbegin, rend) + _Z_DELIMITER_LEN;
                rcbegin = rbegin;
            }
        }
        lbegin = lcbegin;
        rbegin = rcbegin;
    }
    return true;
}

bool _z_keyexpr_intersects_parts(const char *lbegin, const char *lend, const char *rbegin, const char *rend);

bool _z_keyexpr_intersects_backward(const char *lbegin, const char *lend, const char *rbegin, const char *rend,
                                    bool can_have_verbatim) {
    // left is a suffix following a doublestar i.e. **/l/l/l/l
    while (lbegin < lend && rbegin < rend) {
        _z_chunk_backward_match_data_t res = _z_chunk_intersects_backward(lbegin, lend, rbegin, rend);
        if (res.result == _Z_CHUNK_MATCH_RESULT_NO) {
            return false;
        } else if (res.result == _Z_CHUNK_MATCH_RESULT_YES) {
            bool left_exhausted = lbegin == res.lbegin;
            bool right_exhausted = rbegin == res.rbegin;
            if (left_exhausted && right_exhausted) {
                return true;  // leading doublestar in left matches an empty string
            } else if (left_exhausted) {
                // left is exhausted, right is matched if it does not contain any verbatim chunks
                return !can_have_verbatim ||
                       _z_keyexpr_get_next_verbatim_chunk(rbegin, res.rbegin - _Z_DELIMITER_LEN) == NULL;
            } else if (right_exhausted) {
                return false;  // right is exhausted, left is not (and thus contain at least one non doublestar chunk),
                               // match is impossible
            }
            lend = res.lbegin - _Z_DELIMITER_LEN;
            rend = res.rbegin - _Z_DELIMITER_LEN;
        } else if (can_have_verbatim) {
            // Now this becomes more complicated
            // in the absence of verbatim chunks, we could apply the same logic as for matching stardsl chunks, but not
            // here. Given that we anyway would need to scan both kes for verbatim chunks and compare them we rather
            // fallback to splitting kes across verbatim chunks and running intersections on each subke.
            return _z_keyexpr_intersects_parts(lbegin - _Z_DOUBLE_STAR_LEN - _Z_DELIMITER_LEN, lend, rbegin, rend);
        } else if (res.result == _Z_CHUNK_MATCH_RESULT_LEFT_SUPERWILD) {
            return _z_keyexpr_intersects_special(lbegin, lend - _Z_DOUBLE_STAR_LEN - _Z_DELIMITER_LEN, rbegin, rend);
        } else if (res.result == _Z_CHUNK_MATCH_RESULT_RIGHT_SUPERWILD) {
            return true;
        }
    }
    bool left_exhausted = lbegin >= lend;
    bool right_exhausted = rbegin >= rend;
    return (left_exhausted || _z_keyexpr_is_double_star(lbegin, lend)) &&
           (right_exhausted || _z_keyexpr_is_double_star(rbegin, rend));
}

bool _z_keyexpr_intersects_forward(const char *lbegin, const char *lend, const char *rbegin, const char *rend,
                                   bool can_have_verbatim) {
    while (lbegin < lend && rbegin < rend) {
        _z_chunk_forward_match_data_t res = _z_chunk_intersects_forward(lbegin, lend, rbegin, rend);
        if (res.result == _Z_CHUNK_MATCH_RESULT_NO) {
            return false;
        } else if (res.result == _Z_CHUNK_MATCH_RESULT_LEFT_SUPERWILD) {
            if (lbegin + _Z_DOUBLE_STAR_LEN == lend) {  // left is exhausted
                return _z_keyexpr_get_next_verbatim_chunk(rbegin, rend) == NULL;
            }
            return _z_keyexpr_intersects_backward(lbegin + _Z_DOUBLE_STAR_LEN + _Z_DELIMITER_LEN, lend, rbegin, rend,
                                                  can_have_verbatim);
        } else if (res.result == _Z_CHUNK_MATCH_RESULT_RIGHT_SUPERWILD) {
            if (rbegin + _Z_DOUBLE_STAR_LEN == rend) {  // right is exhausted
                return _z_keyexpr_get_next_verbatim_chunk(lbegin, lend) == NULL;
            }
            return _z_keyexpr_intersects_backward(rbegin + _Z_DOUBLE_STAR_LEN + _Z_DELIMITER_LEN, rend, lbegin, lend,
                                                  can_have_verbatim);
        } else {
            lbegin = res.lend + _Z_DELIMITER_LEN;
            rbegin = res.rend + _Z_DELIMITER_LEN;
        }
    }

    bool left_exhausted = lbegin >= lend;
    bool right_exhausted = rbegin >= rend;
    return (left_exhausted || _z_keyexpr_is_double_star(lbegin, lend)) &&
           (right_exhausted || _z_keyexpr_is_double_star(rbegin, rend));
}

bool _z_keyexpr_intersects_parts(const char *lbegin, const char *lend, const char *rbegin, const char *rend) {
    while (lbegin < lend && rbegin < rend) {
        const char *lverbatim = _z_keyexpr_get_next_verbatim_chunk(lbegin, lend);
        const char *rverbatim = _z_keyexpr_get_next_verbatim_chunk(rbegin, rend);
        if (lverbatim == NULL && rverbatim == NULL) {
            return _z_keyexpr_intersects_forward(lbegin, lend, rbegin, rend, false);
        } else if (lverbatim == NULL ||
                   rverbatim == NULL) {  // different number of verbatim chunks, they can not intersect
            return false;
        }

        if (!_z_keyexpr_intersects_forward(lbegin, lverbatim - _Z_DELIMITER_LEN, rbegin, rverbatim - _Z_DELIMITER_LEN,
                                           false)) {
            return false;  // prefixes before verbatim chunks do not intersect, so kes can not intersect
        }

        _z_chunk_forward_match_data_t res = _z_chunk_intersects_forward(lverbatim, lend, rverbatim, rend);
        if (res.result != _Z_CHUNK_MATCH_RESULT_YES) {
            return false;  // verbatim chunks do not match, so kes can not intersect
        }
        lbegin = res.lend + _Z_DELIMITER_LEN;
        rbegin = res.rend + _Z_DELIMITER_LEN;
    }
    bool left_exhausted = lbegin >= lend;
    bool right_exhausted = rbegin >= rend;
    return (left_exhausted || _z_keyexpr_is_double_star(lbegin, lend)) &&
           (right_exhausted || _z_keyexpr_is_double_star(rbegin, rend));
}

bool _z_keyexpr_intersects(const _z_keyexpr_t *left, const _z_keyexpr_t *right) {
    size_t left_len = _z_string_len(&left->_keyexpr);
    size_t right_len = _z_string_len(&right->_keyexpr);
    const char *left_start = _z_string_data(&left->_keyexpr);
    const char *right_start = _z_string_data(&right->_keyexpr);

    // fast path for identical key expressions, do we really need it ?
    if ((left_len == right_len) && (strncmp(left_start, right_start, left_len) == 0)) {
        return true;
    }

    return _z_keyexpr_intersects_forward(left_start, left_start + left_len, right_start, right_start + right_len, true);
}

bool _z_chunk_includes_special(const char *lbegin, const char *lend, const char *rbegin, const char *rend) {
    // left is a non-empty part of a chunk following a stardsl and preceeding a stardsl i.e. $*lllll$*
    // right is chunk that does not start, nor end with a stardsl(s), but can contain stardsl in the middle
    // original left chunk has the following form $*L1$*L2$*...$*LN$*
    // so there is only 1 case where it can include right:
    //
    // Every subchunk of left (L1, L2, ..., LN) is present in right in the
    // correct order (and they do not overlap), which implies that right has the form
    // R1L1R2L2R3...RNLNR(N+1), in this case all Ri can be included by $* in left
    return _z_chunk_right_contains_all_stardsl_subchunks_of_left(lbegin, lend, rbegin, rend);
}

_z_chunk_forward_match_data_t _z_chunk_includes_forward_backward(const char *lbegin, const char *lkend,
                                                                 const char *rbegin, const char *rkend) {
    // left is a part of a chunk following a stardsl. i.e $*llllll
    _z_chunk_forward_match_data_t result = {0};
    result.lend = _z_chunk_end(lbegin, lkend);
    result.rend = _z_chunk_end(rbegin, rkend);

    const char *lend = result.lend;
    const char *rend = result.rend;

    while (lend > lbegin && rend > rbegin) {
        lend--;
        rend--;
        if (*lend == _Z_DSL1) {
            result.result = _z_chunk_includes_special(lbegin, lend + 1 - _Z_DSL_LEN, rbegin, rend + 1)
                                ? _Z_CHUNK_MATCH_RESULT_YES
                                : _Z_CHUNK_MATCH_RESULT_NO;
            return result;
        } else if (*rend == _Z_DSL1 || *lend != *rend) {
            // stardsl in right that can not be included by left, or non-matching characters, so no match
            return result;
        }
    }
    if (lbegin == lend) {  // remainder of left is entirely matched, leading stardsl can include the rest of right
        result.result = _Z_CHUNK_MATCH_RESULT_YES;
    }
    return result;
}

_z_chunk_forward_match_data_t _z_chunk_includes_forward(const char *lbegin, const char *lkend, const char *rbegin,
                                                        const char *rkend) {
    // left and right are guaranteed to be non-empty canonized chunks
    _z_chunk_forward_match_data_t result = {0};
    // assume canonized chunks
    bool left_is_wild = *lbegin == _Z_STAR;
    bool right_is_wild = *rbegin == _Z_STAR;

    bool left_is_superwild = left_is_wild && (lbegin + 2 <= lkend) && *(lbegin + 1) == _Z_STAR;
    bool right_is_superwild = right_is_wild && (rbegin + 2 <= rkend) && *(rbegin + 1) == _Z_STAR;

    if (left_is_superwild) {
        result.result = _Z_CHUNK_MATCH_RESULT_LEFT_SUPERWILD;
        return result;
    } else if (right_is_superwild) {
        return result;
    }
    bool left_is_verbatim = *lbegin == _Z_VERBATIM;
    bool right_is_verbatim = *rbegin == _Z_VERBATIM;

    if (left_is_verbatim != right_is_verbatim) {
        return result;
    } else if (left_is_wild) {
        result.result = _Z_CHUNK_MATCH_RESULT_YES;
        result.lend = lbegin + 1;
        result.rend = _z_chunk_end(rbegin, rkend);
        return result;
    } else if (right_is_wild) {
        return result;
    }

    // at this stage we should only care about stardsl, as the presence of verbatim or wild is already checked.
    while (lbegin < lkend && rbegin < rkend && *lbegin != _Z_SEPARATOR && *rbegin != _Z_SEPARATOR) {
        if (*lbegin == _Z_DSL0) {
            return _z_chunk_includes_forward_backward(lbegin + _Z_DSL_LEN, lkend, rbegin, rkend);
        } else if (*rbegin == _Z_DSL0 || *lbegin != *rbegin) {
            return result;
        }
        lbegin++;
        rbegin++;
    }
    bool left_exhausted = lbegin == lkend || *lbegin == _Z_SEPARATOR;
    bool right_exhausted = rbegin == rkend || *rbegin == _Z_SEPARATOR;
    if (left_exhausted && right_exhausted) {
        result.result = _Z_CHUNK_MATCH_RESULT_YES;
        result.lend = lbegin;
        result.rend = rbegin;
    } else if (right_exhausted && _z_chunk_is_stardsl(lbegin, lkend)) {
        result.result = _Z_CHUNK_MATCH_RESULT_YES;
        result.lend = lbegin + _Z_DSL_LEN;
        result.rend = rbegin;
    }
    return result;
}

_z_chunk_backward_match_data_t _z_chunk_includes_backward_forward(const char *lkbegin, const char *lend,
                                                                  const char *rkbegin, const char *rend) {
    // left is a part of a chunk preceeding a stardsl i.e lllll$*
    _z_chunk_backward_match_data_t result = {0};
    const char *lbegin = _z_chunk_begin(lkbegin, lend);
    const char *rbegin = _z_chunk_begin(rkbegin, rend);
    result.lbegin = lbegin;
    result.rbegin = rbegin;

    // chunks can not be star, nor doublestar
    while (lbegin < lend && rbegin < rend) {
        if (*lbegin == _Z_DSL0) {
            result.result = _z_chunk_includes_special(lbegin + 2, lend, rbegin, rend) ? _Z_CHUNK_MATCH_RESULT_YES
                                                                                      : _Z_CHUNK_MATCH_RESULT_NO;
            return result;
        } else if (*rbegin == _Z_DSL0 || *lbegin != *rbegin) {
            result.result = _Z_CHUNK_MATCH_RESULT_NO;
            return result;
        }
        lbegin++;
        rbegin++;
    }

    if (lbegin == lend) {  // remainder of left is entirely matched, suffix stardsl can match the rest of right
        result.result = _Z_CHUNK_MATCH_RESULT_YES;
    }
    return result;
}

_z_chunk_backward_match_data_t _z_chunk_includes_backward(const char *lkbegin, const char *lend, const char *rkbegin,
                                                          const char *rend) {
    _z_chunk_backward_match_data_t result = {0};
    const char *llast = lend - 1;
    const char *rlast = rend - 1;
    // assume canonized chunks
    bool left_is_wild = *llast == _Z_STAR;
    bool right_is_wild = *rlast == _Z_STAR;

    bool left_is_superwild = left_is_wild && (lkbegin + 2 <= lend) && *(llast - 1) == _Z_STAR;
    bool right_is_superwild = right_is_wild && (rkbegin + 2 <= rend) && *(rlast - 1) == _Z_STAR;

    if (left_is_superwild) {
        result.result = _Z_CHUNK_MATCH_RESULT_LEFT_SUPERWILD;
        return result;
    } else if (right_is_superwild) {
        return result;
    }
    left_is_wild = left_is_wild && (lkbegin == llast || *(llast - 1) == _Z_SEPARATOR);
    right_is_wild = right_is_wild && (rkbegin == rlast || *(rlast - 1) == _Z_SEPARATOR);
    if (left_is_wild) {
        result.lbegin = llast;
        result.rbegin = _z_chunk_begin(rkbegin, rend);
        result.result = *result.rbegin == _Z_VERBATIM ? _Z_CHUNK_MATCH_RESULT_NO : _Z_CHUNK_MATCH_RESULT_YES;
        return result;
    } else if (right_is_wild) {  // right has a star that can not be included by left
        return result;
    }

    while (llast >= lkbegin && rlast >= rkbegin && *llast != _Z_SEPARATOR && *rlast != _Z_SEPARATOR) {
        if (*llast == _Z_DSL1) {
            return _z_chunk_includes_backward_forward(lkbegin, llast + 1 - _Z_DSL_LEN, rkbegin, rlast + 1);
        } else if (*rlast == _Z_DSL1) {
            return result;
        }
        if (*llast != *rlast) {
            result.result = _Z_CHUNK_MATCH_RESULT_NO;
            return result;
        }
        llast--;
        rlast--;
    }
    bool left_exhausted = lkbegin == llast + 1 || *llast == _Z_SEPARATOR;
    bool right_exhausted = rkbegin == rlast + 1 || *rlast == _Z_SEPARATOR;
    if (left_exhausted && right_exhausted) {
        result.result = _Z_CHUNK_MATCH_RESULT_YES;
        result.lbegin = llast + 1;
        result.rbegin = rlast + 1;
    } else if (right_exhausted && _z_chunk_is_stardsl_reverse(lkbegin, llast)) {
        result.result = _Z_CHUNK_MATCH_RESULT_YES;
        result.lbegin = llast + 1 - _Z_DSL_LEN;
        result.rbegin = rlast + 1;
    }
    return result;
}

bool _z_keyexpr_include_special(const char *lbegin, const char *lend, const char *rbegin, const char *rend) {
    // left is a non-empty part of a ke sorrounded by doublestar i.e. **/l/l/l/l**
    // right is ke that does not start, nor end with a doublestar(s), but can contain doublestar in the middle
    // none of the kes contain any verbatim chunks
    // so there are only 1 cases where left can include right:
    //
    // Every chunk of left **/L1/**/L2/**/../LN/** is include in ke in right
    // in the correct order and without overlap, i.e right has the form R1l1R2l2R3...RNlNR(N+1), where li < Li, in this
    // case all Ri can be included by ** in left

    // right is guaranteed not to have doublestars
    while (lbegin < lend) {
        const char *lcbegin = lbegin;
        const char *lcend = _z_keyexpr_get_next_double_star_chunk(lbegin, lend);
        lcend = lcend != NULL ? lcend - _Z_DELIMITER_LEN : lend;
        const char *rcbegin = rbegin;

        while (lcbegin < lcend) {
            if (rcbegin >= rend) {
                return false;
            }
            _z_chunk_forward_match_data_t res = _z_chunk_includes_forward(lcbegin, lcend, rcbegin, rend);
            if (res.result == _Z_CHUNK_MATCH_RESULT_YES) {
                lcbegin = res.lend + _Z_DELIMITER_LEN;
                rcbegin = res.rend + _Z_DELIMITER_LEN;
            } else {  // res.result == _Z_CHUNK_MATCH_RESULT_NO
                // reset subke matching and try to match current left subke with the next right chunk
                lcbegin = lbegin;
                rbegin = _z_chunk_end(rbegin, rend) + _Z_DELIMITER_LEN;
                rcbegin = rbegin;
            }
        }
        lbegin = lcbegin;
        rbegin = rcbegin;
    }
    return true;
}

bool _z_keyexpr_includes_parts(const char *lbegin, const char *lend, const char *rbegin, const char *rend);

bool _z_keyexpr_includes_backward(const char *lbegin, const char *lend, const char *rbegin, const char *rend,
                                  bool can_have_verbatim) {
    // left is a suffix following a doublestar i.e. **/l/l/l/l
    while (lbegin < lend && rbegin < rend) {
        _z_chunk_backward_match_data_t res = _z_chunk_includes_backward(lbegin, lend, rbegin, rend);
        if (res.result == _Z_CHUNK_MATCH_RESULT_NO || res.result == _Z_CHUNK_MATCH_RESULT_RIGHT_SUPERWILD) {
            return false;
        } else if (res.result == _Z_CHUNK_MATCH_RESULT_YES) {
            bool left_exhausted = lbegin == res.lbegin;
            bool right_exhausted = rbegin == res.rbegin;
            if (left_exhausted && right_exhausted) {
                return true;  // leading doublestar in left matches an empty string
            } else if (left_exhausted) {
                // left is exhausted, right is matched if it does not contain any verbatim chunks
                return !can_have_verbatim ||
                       _z_keyexpr_get_next_verbatim_chunk(rbegin, res.rbegin - _Z_DELIMITER_LEN) == NULL;
            } else if (right_exhausted) {
                return false;  // right is exhausted, left is not (and thus contain at least one non doublestar chunk),
                               // match is impossible
            }
            lend = res.lbegin - _Z_DELIMITER_LEN;
            rend = res.rbegin - _Z_DELIMITER_LEN;
        } else if (can_have_verbatim) {
            // Now this becomes more complicated
            // in the absence of verbatim chunks, we could apply the same logic as for matching stardsl chunks, but not
            // here. Given that we anyway would need to scan both kes for verbatim chunks and compare them we rather
            // fallback to splitting kes across verbatim chunks and running intersections on each subke.
            return _z_keyexpr_includes_parts(lbegin - _Z_DOUBLE_STAR_LEN - _Z_DELIMITER_LEN, lend, rbegin, rend);
        } else if (res.result == _Z_CHUNK_MATCH_RESULT_LEFT_SUPERWILD) {
            return _z_keyexpr_include_special(lbegin, lend - _Z_DOUBLE_STAR_LEN - _Z_DELIMITER_LEN, rbegin, rend);
        }
    }
    bool left_exhausted = lbegin >= lend;
    bool right_exhausted = rbegin >= rend;
    return right_exhausted && (left_exhausted || _z_keyexpr_is_double_star(lbegin, lend));
}

bool _z_keyexpr_includes_forward(const char *lbegin, const char *lend, const char *rbegin, const char *rend,
                                 bool can_have_verbatim) {
    while (lbegin < lend && rbegin < rend) {
        _z_chunk_forward_match_data_t res = _z_chunk_includes_forward(lbegin, lend, rbegin, rend);
        if (res.result == _Z_CHUNK_MATCH_RESULT_YES) {
            lbegin = res.lend + _Z_DELIMITER_LEN;
            rbegin = res.rend + _Z_DELIMITER_LEN;
        } else if (res.result == _Z_CHUNK_MATCH_RESULT_LEFT_SUPERWILD) {
            if (lbegin + _Z_DOUBLE_STAR_LEN == lend) {  // left is exhausted
                return _z_keyexpr_get_next_verbatim_chunk(rbegin, rend) == NULL;
            }
            return _z_keyexpr_includes_backward(lbegin + _Z_DOUBLE_STAR_LEN + _Z_DELIMITER_LEN, lend, rbegin, rend,
                                                can_have_verbatim);
        } else {
            // superwild right chunk without superwild left implies no inclusion
            return false;
        }
    }

    bool left_exhausted = lbegin >= lend;
    bool right_exhausted = rbegin >= rend;
    return right_exhausted && (left_exhausted || _z_keyexpr_is_double_star(lbegin, lend));
}

bool _z_keyexpr_includes_parts(const char *lbegin, const char *lend, const char *rbegin, const char *rend) {
    while (lbegin < lend && rbegin < rend) {
        const char *lverbatim = _z_keyexpr_get_next_verbatim_chunk(lbegin, lend);
        const char *rverbatim = _z_keyexpr_get_next_verbatim_chunk(rbegin, rend);
        if (lverbatim == NULL && rverbatim == NULL) {
            return _z_keyexpr_includes_forward(lbegin, lend, rbegin, rend, false);
        } else if (lverbatim == NULL ||
                   rverbatim == NULL) {  // different number of verbatim chunks, they can not intersect
            return false;
        }

        if (!_z_keyexpr_includes_forward(lbegin, lverbatim - _Z_DELIMITER_LEN, rbegin, rverbatim - _Z_DELIMITER_LEN,
                                         false)) {
            return false;  // prefixes before verbatim chunks do not match, so kes can not match
        }

        _z_chunk_forward_match_data_t res = _z_chunk_includes_forward(lverbatim, lend, rverbatim, rend);
        if (res.result != _Z_CHUNK_MATCH_RESULT_YES) {
            return false;  // verbatim chunks do not match, so kes can not match
        }
        lbegin = res.lend + _Z_DELIMITER_LEN;
        rbegin = res.rend + _Z_DELIMITER_LEN;
    }
    bool left_exhausted = lbegin >= lend;
    bool right_exhausted = rbegin >= rend;
    return right_exhausted && (left_exhausted || _z_keyexpr_is_double_star(lbegin, lend));
}

bool _z_keyexpr_includes(const _z_keyexpr_t *left, const _z_keyexpr_t *right) {
    size_t left_len = _z_string_len(&left->_keyexpr);
    size_t right_len = _z_string_len(&right->_keyexpr);
    const char *left_start = _z_string_data(&left->_keyexpr);
    const char *right_start = _z_string_data(&right->_keyexpr);

    // fast path for identical key expressions, do we really need it ?
    if ((left_len == right_len) && (strncmp(left_start, right_start, left_len) == 0)) {
        return true;
    }

    return _z_keyexpr_includes_forward(left_start, left_start + left_len, right_start, right_start + right_len, true);
}
