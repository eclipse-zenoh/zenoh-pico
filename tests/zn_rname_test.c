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
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#include <assert.h>

#include "zenoh-pico/protocol/keyexpr.h"

int main(void) {
    assert(_z_keyexpr_intersect("a", "a"));
    assert(_z_keyexpr_intersect("a", "a"));
    assert(_z_keyexpr_intersect("a", "a"));
    assert(_z_keyexpr_intersect("a/b", "a/b"));
    assert(_z_keyexpr_intersect("*", "abc"));
    assert(_z_keyexpr_intersect("*", "abc"));
    assert(_z_keyexpr_intersect("*", "abc"));
    assert(_z_keyexpr_intersect("*", "xxx"));
    assert(_z_keyexpr_intersect("ab$*", "abcd"));
    assert(_z_keyexpr_intersect("ab$*d", "abcd"));
    assert(_z_keyexpr_intersect("ab$*", "ab"));
    assert(!_z_keyexpr_intersect("ab/*", "ab"));
    assert(_z_keyexpr_intersect("a/*/c/*/e", "a/b/c/d/e"));
    assert(_z_keyexpr_intersect("a/**/d/**/l", "a/b/c/d/e/f/g/h/i/l"));
    assert(_z_keyexpr_intersect("a/**/d/**/l", "a/d/foo/l"));
    assert(_z_keyexpr_intersect("a/$*b/c/$*d/e", "a/xb/c/xd/e"));
    assert(!_z_keyexpr_intersect("a/*/c/*/e", "a/c/e"));
    assert(!_z_keyexpr_intersect("a/*/c/*/e", "a/b/c/d/x/e"));
    assert(!_z_keyexpr_intersect("ab$*cd", "abxxcxxd"));
    assert(_z_keyexpr_intersect("ab$*cd", "abxxcxxcd"));
    assert(!_z_keyexpr_intersect("ab$*cd", "abxxcxxcdx"));
    assert(_z_keyexpr_intersect("**", "abc"));
    assert(_z_keyexpr_intersect("**", "a/b/c"));
    assert(_z_keyexpr_intersect("**", "a/b/c"));
    assert(_z_keyexpr_intersect("**", "a/b/c"));
    assert(_z_keyexpr_intersect("ab/**", "ab"));
    assert(_z_keyexpr_intersect("**/xyz", "a/b/xyz/d/e/f/xyz"));
    assert(!_z_keyexpr_intersect("**/xyz$*xyz", "a/b/xyz/d/e/f/xyz"));
    assert(_z_keyexpr_intersect("a/**/c/**/e", "a/b/b/b/c/d/d/d/e"));
    assert(_z_keyexpr_intersect("a/**/c/**/e", "a/c/e"));
    assert(_z_keyexpr_intersect("a/**/c/*/e/*", "a/b/b/b/c/d/d/c/d/e/f"));
    assert(!_z_keyexpr_intersect("a/**/c/*/e/*", "a/b/b/b/c/d/d/c/d/d/e/f"));
    assert(!_z_keyexpr_intersect("ab$*cd", "abxxcxxcdx"));
    assert(_z_keyexpr_intersect("x/abc", "x/abc"));
    assert(!_z_keyexpr_intersect("x/abc", "abc"));
    assert(_z_keyexpr_intersect("x/*", "x/abc"));
    assert(!_z_keyexpr_intersect("x/*", "abc"));
    assert(!_z_keyexpr_intersect("*", "x/abc"));
    assert(_z_keyexpr_intersect("x/*", "x/abc$*"));
    assert(_z_keyexpr_intersect("x/$*abc", "x/abc$*"));
    assert(_z_keyexpr_intersect("x/a$*", "x/abc$*"));
    assert(_z_keyexpr_intersect("x/a$*de", "x/abc$*de"));
    assert(_z_keyexpr_intersect("x/a$*d$*e", "x/a$*e"));
    assert(_z_keyexpr_intersect("x/a$*d$*e", "x/a$*c$*e"));
    assert(_z_keyexpr_intersect("x/a$*d$*e", "x/ade"));
    assert(!_z_keyexpr_intersect("x/c$*", "x/abc$*"));
    assert(!_z_keyexpr_intersect("x/$*d", "x/$*e"));
    assert(_z_keyexpr_intersect("a", "a"));
    assert(_z_keyexpr_intersect("a/b", "a/b"));
    assert(_z_keyexpr_intersect("*", "a"));
    assert(_z_keyexpr_intersect("a", "*"));
    assert(_z_keyexpr_intersect("*", "aaaaa"));
    assert(_z_keyexpr_intersect("**", "a"));
    assert(_z_keyexpr_intersect("a", "**"));
    assert(_z_keyexpr_intersect("**", "a"));
    assert(_z_keyexpr_intersect("a/a/a/a", "**"));
    assert(_z_keyexpr_intersect("a/*", "a/b"));
    assert(!_z_keyexpr_intersect("a/*/b", "a/b"));
    assert(_z_keyexpr_intersect("a/**/b", "a/b"));
    assert(_z_keyexpr_intersect("a/b$*", "a/b"));
    assert(_z_keyexpr_intersect("a/$*b$*", "a/b"));
    assert(_z_keyexpr_intersect("a/$*b", "a/b"));
    assert(_z_keyexpr_intersect("a/b$*", "a/bc"));
    assert(_z_keyexpr_intersect("a/$*b$*", "a/ebc"));
    assert(_z_keyexpr_intersect("a/$*b", "a/cb"));
    assert(!_z_keyexpr_intersect("a/b$*", "a/ebc"));
    assert(!_z_keyexpr_intersect("a/$*b", "a/cbc"));
    assert(_z_keyexpr_intersect("a/**/b$*", "a/b"));
    assert(_z_keyexpr_intersect("a/**/$*b$*", "a/b"));
    assert(_z_keyexpr_intersect("a/**/$*b", "a/b"));
    assert(_z_keyexpr_intersect("a/**/b$*", "a/bc"));
    assert(_z_keyexpr_intersect("a/**/$*b$*", "a/ebc"));
    assert(_z_keyexpr_intersect("a/**/$*b", "a/cb"));
    assert(!_z_keyexpr_intersect("a/**/b$*", "a/ebc"));
    assert(!_z_keyexpr_intersect("a/**/$*b", "a/cbc"));

    assert(_z_keyexpr_includes("a", "a"));
    assert(_z_keyexpr_includes("a/b", "a/b"));
    assert(_z_keyexpr_includes("*", "a"));
    assert(!_z_keyexpr_includes("a", "*"));
    assert(_z_keyexpr_includes("*", "aaaaa"));
    assert(_z_keyexpr_includes("**", "a"));
    assert(!_z_keyexpr_includes("a", "**"));
    assert(_z_keyexpr_includes("**", "a"));
    assert(_z_keyexpr_includes("**", "a/a/a/a"));
    assert(_z_keyexpr_includes("**", "*/**"));
    assert(_z_keyexpr_includes("*/**", "*/**"));
    assert(!_z_keyexpr_includes("*/**", "**"));
    assert(!_z_keyexpr_includes("a/a/a/a", "**"));
    assert(_z_keyexpr_includes("a/*", "a/b"));
    assert(!_z_keyexpr_includes("a/*/b", "a/b"));
    assert(_z_keyexpr_includes("a/**/b", "a/b"));
    assert(_z_keyexpr_includes("a/b$*", "a/b"));
    assert(!_z_keyexpr_includes("a/b", "a/b$*"));
    assert(_z_keyexpr_includes("a/$*b$*", "a/b"));
    assert(_z_keyexpr_includes("a/$*b", "a/b"));
    assert(_z_keyexpr_includes("a/b$*", "a/bc"));
    assert(_z_keyexpr_includes("a/$*b$*", "a/ebc"));
    assert(_z_keyexpr_includes("a/$*b", "a/cb"));
    assert(!_z_keyexpr_includes("a/b$*", "a/ebc"));
    assert(!_z_keyexpr_includes("a/$*b", "a/cbc"));
    assert(_z_keyexpr_includes("a/**/b$*", "a/b"));
    assert(_z_keyexpr_includes("a/**/$*b$*", "a/b"));
    assert(_z_keyexpr_includes("a/**/$*b", "a/b"));
    assert(_z_keyexpr_includes("a/**/b$*", "a/bc"));
    assert(_z_keyexpr_includes("a/**/$*b$*", "a/ebc"));
    assert(_z_keyexpr_includes("a/**/$*b", "a/cb"));
    assert(!_z_keyexpr_includes("a/**/b$*", "a/ebc"));
    assert(!_z_keyexpr_includes("a/**/$*b", "a/cbc"));

#define N 26
    const char *input[N] = {"greetings/hello/there",
                            "greetings/good/*/morning",
                            "greetings/*",
                            "greetings/*/**",
                            "greetings/$*",
                            "greetings/**/*/morning",
                            "greetings/**/*",
                            "greetings/**/**",
                            "greetings/**/*/**",
                            "$*",
                            "$*$*",
                            "$*$*$*",
                            "$*hi$*$*",
                            "$*$*hi$*",
                            "hi$*$*$*",
                            "$*$*$*hi",
                            "$*$*$*hi$*$*$*",
                            "hi*",
                            "/hi",
                            "hi/",
                            "",
                            "greetings/**/*/",
                            "greetings/**/*/e?",
                            "greetings/**/*/e#",
                            "greetings/**/*/e$",
                            "greetings/**/*/$e"};
    z_keyexpr_canon_status_t expected[N] = {Z_KEYEXPR_CANON_SUCCESS,
                                            Z_KEYEXPR_CANON_SUCCESS,
                                            Z_KEYEXPR_CANON_SUCCESS,
                                            Z_KEYEXPR_CANON_SUCCESS,
                                            Z_KEYEXPR_CANON_SUCCESS,
                                            Z_KEYEXPR_CANON_SUCCESS,
                                            Z_KEYEXPR_CANON_SUCCESS,
                                            Z_KEYEXPR_CANON_SUCCESS,
                                            Z_KEYEXPR_CANON_SUCCESS,
                                            Z_KEYEXPR_CANON_SUCCESS,
                                            Z_KEYEXPR_CANON_SUCCESS,
                                            Z_KEYEXPR_CANON_SUCCESS,
                                            Z_KEYEXPR_CANON_SUCCESS,
                                            Z_KEYEXPR_CANON_SUCCESS,
                                            Z_KEYEXPR_CANON_SUCCESS,
                                            Z_KEYEXPR_CANON_SUCCESS,
                                            Z_KEYEXPR_CANON_SUCCESS,
                                            Z_KEYEXPR_CANON_STARS_IN_CHUNK,
                                            Z_KEYEXPR_CANON_EMPTY_CHUNK,
                                            Z_KEYEXPR_CANON_EMPTY_CHUNK,
                                            Z_KEYEXPR_CANON_EMPTY_CHUNK,
                                            Z_KEYEXPR_CANON_EMPTY_CHUNK,
                                            Z_KEYEXPR_CANON_CONTAINS_SHARP_OR_QMARK,
                                            Z_KEYEXPR_CANON_CONTAINS_SHARP_OR_QMARK,
                                            Z_KEYEXPR_CANON_CONTAINS_UNBOUND_DOLLAR,
                                            Z_KEYEXPR_CANON_CONTAINS_UNBOUND_DOLLAR};
    const char *canonized[N] = {"greetings/hello/there",
                                "greetings/good/*/morning",
                                "greetings/*",
                                "greetings/*/**",
                                "greetings/*",
                                "greetings/*/**/morning",
                                "greetings/*/**",
                                "greetings/**",
                                "greetings/*/**",
                                "*",
                                "*",
                                "*",
                                "$*hi$*",
                                "$*hi$*",
                                "hi$*",
                                "$*hi",
                                "$*hi$*",
                                "hi*",
                                "/hi",
                                "hi/",
                                "",
                                "greetings/**/*/",
                                "greetings/**/*/e?",
                                "greetings/**/*/e#",
                                "greetings/**/*/e$",
                                "greetings/**/*/$e"};

    for (int i = 0; i < N; i++) {
        const char *ke = input[i];
        char canon[128];
        strcpy(canon, ke);
        size_t canon_len = strlen(canon);
        z_keyexpr_canon_status_t status = _z_keyexpr_canonize(canon, &canon_len);
        printf("%s\n", ke);
        printf("\t%d : %d\n", status, expected[i]);
        assert(status == expected[i]);
        if (status == Z_KEYEXPR_CANON_SUCCESS) {
            printf("\t%.*s : %s\n", (int)canon_len, canon, canonized[i]);
            assert(strncmp(canonized[i], canon, canon_len) == 0);
        }
    }

    return 0;
}
