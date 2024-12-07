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

#include <stdio.h>
#include <stdlib.h>

#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/protocol/keyexpr.h"

#undef NDEBUG
#include <assert.h>

#define TEST_TRUE_INTERSECT(a, b) \
    ke_a = _z_rname(a);           \
    ke_b = _z_rname(b);           \
    assert(_z_keyexpr_suffix_intersects(&ke_a, &ke_b));

#define TEST_FALSE_INTERSECT(a, b) \
    ke_a = _z_rname(a);            \
    ke_b = _z_rname(b);            \
    assert(!_z_keyexpr_suffix_intersects(&ke_a, &ke_b));

#define TEST_TRUE_INCLUDE(a, b) \
    ke_a = _z_rname(a);         \
    ke_b = _z_rname(b);         \
    assert(_z_keyexpr_suffix_includes(&ke_a, &ke_b));

#define TEST_FALSE_INCLUDE(a, b) \
    ke_a = _z_rname(a);          \
    ke_b = _z_rname(b);          \
    assert(!_z_keyexpr_suffix_includes(&ke_a, &ke_b));

#define TEST_TRUE_EQUAL(a, b) \
    ke_a = _z_rname(a);       \
    ke_b = _z_rname(b);       \
    assert(_z_keyexpr_suffix_equals(&ke_a, &ke_b));

#define TEST_FALSE_EQUAL(a, b) \
    ke_a = _z_rname(a);        \
    ke_b = _z_rname(b);        \
    assert(!_z_keyexpr_suffix_equals(&ke_a, &ke_b));

void test_intersects(void) {
    _z_keyexpr_t ke_a, ke_b;
    TEST_TRUE_INTERSECT("a", "a")
    TEST_TRUE_INTERSECT("a/b", "a/b")
    TEST_TRUE_INTERSECT("*", "abc")
    TEST_TRUE_INTERSECT("*", "abc")
    TEST_TRUE_INTERSECT("*", "abc")
    TEST_TRUE_INTERSECT("*", "xxx")
    TEST_TRUE_INTERSECT("ab$*", "abcd")
    TEST_TRUE_INTERSECT("ab$*d", "abcd")
    TEST_TRUE_INTERSECT("ab$*", "ab")
    TEST_FALSE_INTERSECT("ab/*", "ab")
    TEST_TRUE_INTERSECT("a/*/c/*/e", "a/b/c/d/e")
    TEST_TRUE_INTERSECT("a/**/d/**/l", "a/b/c/d/e/f/g/h/i/l")
    TEST_TRUE_INTERSECT("a/**/d/**/l", "a/d/foo/l")
    TEST_TRUE_INTERSECT("a/$*b/c/$*d/e", "a/xb/c/xd/e")
    TEST_FALSE_INTERSECT("a/*/c/*/e", "a/c/e")
    TEST_FALSE_INTERSECT("a/*/c/*/e", "a/b/c/d/x/e")
    TEST_FALSE_INTERSECT("ab$*cd", "abxxcxxd")
    TEST_TRUE_INTERSECT("ab$*cd", "abxxcxxcd")
    TEST_FALSE_INTERSECT("ab$*cd", "abxxcxxcdx")
    TEST_TRUE_INTERSECT("**", "abc")
    TEST_TRUE_INTERSECT("**", "a/b/c")
    TEST_TRUE_INTERSECT("ab/**", "ab")
    TEST_TRUE_INTERSECT("**/xyz", "a/b/xyz/d/e/f/xyz")
    TEST_FALSE_INTERSECT("**/xyz$*xyz", "a/b/xyz/d/e/f/xyz")
    TEST_TRUE_INTERSECT("a/**/c/**/e", "a/b/b/b/c/d/d/d/e")
    TEST_TRUE_INTERSECT("a/**/c/**/e", "a/c/e")
    TEST_TRUE_INTERSECT("a/**/c/*/e/*", "a/b/b/b/c/d/d/c/d/e/f")
    TEST_FALSE_INTERSECT("a/**/c/*/e/*", "a/b/b/b/c/d/d/c/d/d/e/f")
    TEST_FALSE_INTERSECT("ab$*cd", "abxxcxxcdx")
    TEST_TRUE_INTERSECT("x/abc", "x/abc")
    TEST_FALSE_INTERSECT("x/abc", "abc")
    TEST_TRUE_INTERSECT("x/*", "x/abc")
    TEST_FALSE_INTERSECT("x/*", "abc")
    TEST_FALSE_INTERSECT("*", "x/abc")
    TEST_TRUE_INTERSECT("x/*", "x/abc$*")
    TEST_TRUE_INTERSECT("x/$*abc", "x/abc$*")
    TEST_TRUE_INTERSECT("x/a$*", "x/abc$*")
    TEST_TRUE_INTERSECT("x/a$*de", "x/abc$*de")
    TEST_TRUE_INTERSECT("x/a$*d$*e", "x/a$*e")
    TEST_TRUE_INTERSECT("x/a$*d$*e", "x/a$*c$*e")
    TEST_TRUE_INTERSECT("x/a$*d$*e", "x/ade")
    TEST_FALSE_INTERSECT("x/c$*", "x/abc$*")
    TEST_FALSE_INTERSECT("x/$*d", "x/$*e")
    TEST_TRUE_INTERSECT("a", "a")
    TEST_TRUE_INTERSECT("a/b", "a/b")
    TEST_TRUE_INTERSECT("*", "a")
    TEST_TRUE_INTERSECT("a", "*")
    TEST_TRUE_INTERSECT("*", "aaaaa")
    TEST_TRUE_INTERSECT("**", "a")
    TEST_TRUE_INTERSECT("a", "**")
    TEST_TRUE_INTERSECT("**", "a")
    TEST_TRUE_INTERSECT("a/a/a/a", "**")
    TEST_TRUE_INTERSECT("a/*", "a/b")
    TEST_FALSE_INTERSECT("a/*/b", "a/b")
    TEST_TRUE_INTERSECT("a/**/b", "a/b")
    TEST_TRUE_INTERSECT("a/b$*", "a/b")
    TEST_TRUE_INTERSECT("a/$*b$*", "a/b")
    TEST_TRUE_INTERSECT("a/$*b", "a/b")
    TEST_TRUE_INTERSECT("a/b$*", "a/bc")
    TEST_TRUE_INTERSECT("a/$*b$*", "a/ebc")
    TEST_TRUE_INTERSECT("a/$*b", "a/cb")
    TEST_FALSE_INTERSECT("a/b$*", "a/ebc")
    TEST_FALSE_INTERSECT("a/$*b", "a/cbc")
    TEST_TRUE_INTERSECT("a/**/b$*", "a/b")
    TEST_TRUE_INTERSECT("a/**/$*b$*", "a/b")
    TEST_TRUE_INTERSECT("a/**/$*b", "a/b")
    TEST_TRUE_INTERSECT("a/**/b$*", "a/bc")
    TEST_TRUE_INTERSECT("a/**/$*b$*", "a/ebc")
    TEST_TRUE_INTERSECT("a/**/$*b", "a/cb")
    TEST_FALSE_INTERSECT("a/**/b$*", "a/ebc")
    TEST_FALSE_INTERSECT("a/**/$*b", "a/cbc")

    TEST_TRUE_INTERSECT("@a", "@a")
    TEST_FALSE_INTERSECT("@a", "@ab")
    TEST_FALSE_INTERSECT("@a", "@a/b")
    TEST_FALSE_INTERSECT("@a", "@a/*")
    TEST_FALSE_INTERSECT("@a", "@a/*/**")
    TEST_FALSE_INTERSECT("@a", "@a$*/**")
    TEST_TRUE_INTERSECT("@a", "@a/**")
    TEST_FALSE_INTERSECT("**/xyz$*xyz", "@a/b/xyzdefxyz")
    TEST_TRUE_INTERSECT("@a/**/c/**/e", "@a/b/b/b/c/d/d/d/e")
    TEST_FALSE_INTERSECT("@a/**/c/**/e", "@a/@b/b/b/c/d/d/d/e")
    TEST_TRUE_INTERSECT("@a/**/@c/**/e", "@a/b/b/b/@c/d/d/d/e")
    TEST_TRUE_INTERSECT("@a/**/e", "@a/b/b/d/d/d/e")
    TEST_TRUE_INTERSECT("@a/**/e", "@a/b/b/b/d/d/d/e")
    TEST_TRUE_INTERSECT("@a/**/e", "@a/b/b/c/d/d/d/e")
    TEST_FALSE_INTERSECT("@a/**/e", "@a/b/b/@c/b/d/d/d/e")
    TEST_FALSE_INTERSECT("@a/*", "@a/@b")
    TEST_FALSE_INTERSECT("@a/**", "@a/@b")
    TEST_TRUE_INTERSECT("@a/**/@b", "@a/@b")
    TEST_FALSE_INTERSECT("@a/**/@b", "@a/**/@c/**/@b")
    TEST_TRUE_INTERSECT("@a/@b/**", "@a/@b")
    TEST_TRUE_INTERSECT("@a/**/@c/@b", "@a/**/@c/**/@b")
    TEST_TRUE_INTERSECT("@a/**/@c/**/@b", "@a/**/@c/@b")
}

void test_includes(void) {
    _z_keyexpr_t ke_a, ke_b;
    TEST_TRUE_INCLUDE("a", "a")
    TEST_TRUE_INCLUDE("a/b", "a/b")
    TEST_TRUE_INCLUDE("*", "a")
    TEST_FALSE_INCLUDE("a", "*")
    TEST_TRUE_INCLUDE("*", "aaaaa")
    TEST_TRUE_INCLUDE("**", "a")
    TEST_FALSE_INCLUDE("a", "**")
    TEST_TRUE_INCLUDE("**", "a")
    TEST_TRUE_INCLUDE("**", "a/a/a/a")
    TEST_TRUE_INCLUDE("**", "*/**")
    TEST_TRUE_INCLUDE("*/**", "*/**")
    TEST_FALSE_INCLUDE("*/**", "**")
    TEST_FALSE_INCLUDE("a/a/a/a", "**")
    TEST_TRUE_INCLUDE("a/*", "a/b")
    TEST_FALSE_INCLUDE("a/*/b", "a/b")
    TEST_TRUE_INCLUDE("a/**/b", "a/b")
    TEST_TRUE_INCLUDE("a/b$*", "a/b")
    TEST_FALSE_INCLUDE("a/b", "a/b$*")
    TEST_TRUE_INCLUDE("a/$*b$*", "a/b")
    TEST_TRUE_INCLUDE("a/$*b", "a/b")
    TEST_TRUE_INCLUDE("a/b$*", "a/bc")
    TEST_TRUE_INCLUDE("a/$*b$*", "a/ebc")
    TEST_TRUE_INCLUDE("a/$*b", "a/cb")
    TEST_FALSE_INCLUDE("a/b$*", "a/ebc")
    TEST_FALSE_INCLUDE("a/$*b", "a/cbc")
    TEST_TRUE_INCLUDE("a/**/b$*", "a/b")
    TEST_TRUE_INCLUDE("a/**/$*b$*", "a/b")
    TEST_TRUE_INCLUDE("a/**/$*b", "a/b")
    TEST_TRUE_INCLUDE("a/**/b$*", "a/bc")
    TEST_TRUE_INCLUDE("a/**/$*b$*", "a/ebc")
    TEST_TRUE_INCLUDE("a/**/$*b", "a/cb")
    TEST_FALSE_INCLUDE("a/**/b$*", "a/ebc")
    TEST_FALSE_INCLUDE("a/**/$*b", "a/cbc")

    TEST_TRUE_INCLUDE("@a", "@a")
    TEST_FALSE_INCLUDE("@a", "@ab")
    TEST_FALSE_INCLUDE("@a", "@a/b")
    TEST_FALSE_INCLUDE("@a", "@a/*")
    TEST_FALSE_INCLUDE("@a", "@a/*/**")
    TEST_FALSE_INCLUDE("@a$*/**", "@a")
    TEST_FALSE_INCLUDE("@a", "@a/**")
    TEST_TRUE_INCLUDE("@a/**", "@a")
    TEST_FALSE_INCLUDE("**/xyz$*xyz", "@a/b/xyzdefxyz")
    TEST_TRUE_INCLUDE("@a/**/c/**/e", "@a/b/b/b/c/d/d/d/e")
    TEST_FALSE_INCLUDE("@a/*", "@a/@b")
    TEST_FALSE_INCLUDE("@a/**", "@a/@b")
    TEST_TRUE_INCLUDE("@a/**/@b", "@a/@b")
    TEST_TRUE_INCLUDE("@a/@b/**", "@a/@b")
}

void test_canonize(void) {
    // clang-format off

#define N 31
    const char *input[N] = {"greetings/hello/there",
                            "greetings/good/*/morning",
                            "greetings/*",
                            "greetings/*/**",
                            "greetings/$*",
                            "greetings/**/*/morning",
                            "greetings/**/*/g/morning",
                            "greetings/**/*/m",
                            "greetings/**/*",
                            "greetings/**/**",
                            "greetings/**/**/morning",
                            "greetings/**/**/g/morning",
                            "greetings/**/**/m",
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
    const zp_keyexpr_canon_status_t expected[N] = {Z_KEYEXPR_CANON_SUCCESS,
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
                                "greetings/*/**/g/morning",
                                "greetings/*/**/m",
                                "greetings/*/**",
                                "greetings/**",
                                "greetings/**/morning",
                                "greetings/**/g/morning",
                                "greetings/**/m",
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

    // clang-format on

    for (int i = 0; i < N; i++) {
        const char *ke = input[i];
        char *canon = (char *)malloc(128);
        memset(canon, 0, 128);
        strncpy(canon, ke, 128);
        size_t canon_len = strlen(canon);
        zp_keyexpr_canon_status_t status = z_keyexpr_canonize(canon, &canon_len);
        printf("%s ", ke);
        printf("  Status: %d : %d\n", status, expected[i]);
        assert(status == expected[i]);
        if (status == Z_KEYEXPR_CANON_SUCCESS) {
            printf("  Match: %.*s : %s\n", (int)canon_len, canon, canonized[i]);
            assert(strncmp(canonized[i], canon, canon_len) == 0);
        }
        free(canon);
    }

    for (int i = 0; i < N; i++) {
        const char *ke = input[i];
        char *canon = (char *)malloc(128);
        memset(canon, 0, 128);
        strncpy(canon, ke, 128);
        size_t canon_len = strlen(canon);
        zp_keyexpr_canon_status_t status = z_keyexpr_canonize(canon, &canon_len);
        printf("%s ", ke);
        printf("  Status: %d : %d", status, expected[i]);
        assert(status == expected[i]);
        if (status == Z_KEYEXPR_CANON_SUCCESS) {
            printf("  Match: %.*s : %s", (int)canon_len, canon, canonized[i]);
            assert(strncmp(canonized[i], canon, canon_len) == 0);
        }
        printf("\n");
        free(canon);
    }
}

void test_equals(void) {
    _z_keyexpr_t ke_a, ke_b;
    TEST_FALSE_EQUAL("a/**/$*b", "a/cb");
    TEST_FALSE_EQUAL("a/bc", "a/cb");
    TEST_TRUE_EQUAL("greetings/hello/there", "greetings/hello/there");
}

bool keyexpr_equals_string(const z_loaned_keyexpr_t *ke, const char *s) {
    z_view_string_t vs;
    z_keyexpr_as_view_string(ke, &vs);
    _z_string_t str = _z_string_alias_str(s);
    return _z_string_equals(z_view_string_loan(&vs), &str);
}

void test_keyexpr_constructor(void) {
    z_owned_keyexpr_t ke;
    z_keyexpr_from_str(&ke, "a/b/c");
    assert(keyexpr_equals_string(z_keyexpr_loan(&ke), "a/b/c"));
    z_keyexpr_drop(z_keyexpr_move(&ke));

    z_keyexpr_from_substr(&ke, "a/b/c/d/e", 5);
    assert(keyexpr_equals_string(z_keyexpr_loan(&ke), "a/b/c"));
    z_keyexpr_drop(z_keyexpr_move(&ke));

    assert(0 == z_keyexpr_from_str_autocanonize(&ke, "a/**/**"));
    assert(keyexpr_equals_string(z_keyexpr_loan(&ke), "a/**"));
    z_keyexpr_drop(z_keyexpr_move(&ke));

    size_t len = 9;
    assert(0 == z_keyexpr_from_substr_autocanonize(&ke, "a/**/**/m/b/c", &len));
    assert(keyexpr_equals_string(z_keyexpr_loan(&ke), "a/**/m"));
    assert(len == 6);
    z_keyexpr_drop(z_keyexpr_move(&ke));

    z_view_keyexpr_t vke;
    z_view_keyexpr_from_str(&vke, "a/b/c");
    assert(keyexpr_equals_string(z_view_keyexpr_loan(&vke), "a/b/c"));

    char s[] = "a/**/**";
    z_view_keyexpr_from_str_autocanonize(&vke, s);
    assert(keyexpr_equals_string(z_view_keyexpr_loan(&vke), "a/**"));
}

void test_concat(void) {
    z_owned_keyexpr_t ke1, ke2;
    z_keyexpr_from_str(&ke1, "a/b/c/*");
    assert(0 == z_keyexpr_concat(&ke2, z_keyexpr_loan(&ke1), "/d/e/*", 4));
    assert(keyexpr_equals_string(z_keyexpr_loan(&ke2), "a/b/c/*/d/e"));
    z_keyexpr_drop(z_keyexpr_move(&ke2));

    assert(0 != z_keyexpr_concat(&ke2, z_keyexpr_loan(&ke1), "*/e/*", 3));
    assert(!z_internal_keyexpr_check(&ke2));

    z_keyexpr_drop(z_keyexpr_move(&ke1));
}

void test_join(void) {
    z_owned_keyexpr_t ke1, ke2, ke3;
    z_keyexpr_from_str(&ke1, "a/b/c/*");
    z_keyexpr_from_str(&ke2, "d/e/*");
    assert(0 == z_keyexpr_join(&ke3, z_keyexpr_loan(&ke1), z_keyexpr_loan(&ke2)));
    assert(keyexpr_equals_string(z_keyexpr_loan(&ke3), "a/b/c/*/d/e/*"));
    z_keyexpr_drop(z_keyexpr_move(&ke1));
    z_keyexpr_drop(z_keyexpr_move(&ke2));
    z_keyexpr_drop(z_keyexpr_move(&ke3));

    z_keyexpr_from_str(&ke1, "a/*/**");
    z_keyexpr_from_str(&ke2, "**/d/e/c");

    assert(0 == z_keyexpr_join(&ke3, z_keyexpr_loan(&ke1), z_keyexpr_loan(&ke2)));
    assert(keyexpr_equals_string(z_keyexpr_loan(&ke3), "a/*/**/d/e/c"));

    z_keyexpr_drop(z_keyexpr_move(&ke1));
    z_keyexpr_drop(z_keyexpr_move(&ke2));
    z_keyexpr_drop(z_keyexpr_move(&ke3));
}

void test_relation_to(void) {
    z_view_keyexpr_t foobar, foostar, barstar;
    z_view_keyexpr_from_str(&foobar, "foo/bar");
    z_view_keyexpr_from_str(&foostar, "foo/*");
    z_view_keyexpr_from_str(&barstar, "bar/*");

    assert(z_keyexpr_relation_to(z_view_keyexpr_loan(&foostar), z_view_keyexpr_loan(&foobar)) ==
           Z_KEYEXPR_INTERSECTION_LEVEL_INCLUDES);
    assert(z_keyexpr_relation_to(z_view_keyexpr_loan(&foobar), z_view_keyexpr_loan(&foostar)) ==
           Z_KEYEXPR_INTERSECTION_LEVEL_INTERSECTS);
    assert(z_keyexpr_relation_to(z_view_keyexpr_loan(&foostar), z_view_keyexpr_loan(&foostar)) ==
           Z_KEYEXPR_INTERSECTION_LEVEL_EQUALS);
    assert(z_keyexpr_relation_to(z_view_keyexpr_loan(&barstar), z_view_keyexpr_loan(&foobar)) ==
           Z_KEYEXPR_INTERSECTION_LEVEL_DISJOINT);
}

int main(void) {
    test_intersects();
    test_includes();
    test_canonize();
    test_equals();
    test_keyexpr_constructor();
    test_concat();
    test_join();
    test_relation_to();

    return 0;
}
