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

void test_intersects(void) {
    assert(_z_keyexpr_intersects("a", strlen("a"), "a", strlen("a")));
    assert(_z_keyexpr_intersects("a/b", strlen("a/b"), "a/b", strlen("a/b")));
    assert(_z_keyexpr_intersects("*", strlen("*"), "abc", strlen("abc")));
    assert(_z_keyexpr_intersects("*", strlen("*"), "abc", strlen("abc")));
    assert(_z_keyexpr_intersects("*", strlen("*"), "abc", strlen("abc")));
    assert(_z_keyexpr_intersects("*", strlen("*"), "xxx", strlen("xxx")));
    assert(_z_keyexpr_intersects("ab$*", strlen("ab$*"), "abcd", strlen("abcd")));
    assert(_z_keyexpr_intersects("ab$*d", strlen("ab$*d"), "abcd", strlen("abcd")));
    assert(!_z_keyexpr_intersects("ab$*d", strlen("ab$*d"), "abcde", strlen("abcde")));
    assert(_z_keyexpr_intersects("ab$*", strlen("ab$*"), "ab", strlen("ab")));
    assert(!_z_keyexpr_intersects("ab/*", strlen("ab/*"), "ab", strlen("ab")));
    assert(_z_keyexpr_intersects("a/*/c/*/e", strlen("a/*/c/*/e"), "a/b/c/d/e", strlen("a/b/c/d/e")));
    assert(_z_keyexpr_intersects("a/**/d/**/l", strlen("a/**/d/**/l"), "a/b/c/d/e/f/g/h/i/l",
                                 strlen("a/b/c/d/e/f/g/h/i/l")));
    assert(_z_keyexpr_intersects("a/**/d/**/l", strlen("a/**/d/**/l"), "a/d/foo/l", strlen("a/d/foo/l")));
    assert(_z_keyexpr_intersects("a/$*b/c/$*d/e", strlen("a/$*b/c/$*d/e"), "a/xb/c/xd/e", strlen("a/xb/c/xd/e")));
    assert(!_z_keyexpr_intersects("a/*/c/*/e", strlen("a/*/c/*/e"), "a/c/e", strlen("a/c/e")));
    assert(!_z_keyexpr_intersects("a/*/c/*/e", strlen("a/*/c/*/e"), "a/b/c/d/x/e", strlen("a/b/c/d/x/e")));
    assert(!_z_keyexpr_intersects("ab$*cd", strlen("ab$*cd"), "abxxcxxd", strlen("abxxcxxd")));
    assert(_z_keyexpr_intersects("ab$*cd", strlen("ab$*cd"), "abxxcxxcd", strlen("abxxcxxcd")));
    assert(!_z_keyexpr_intersects("ab$*cd", strlen("ab$*cd"), "abxxcxxcdx", strlen("abxxcxxcdx")));
    assert(_z_keyexpr_intersects("**", strlen("**"), "abc", strlen("abc")));
    assert(_z_keyexpr_intersects("**", strlen("**"), "a/b/c", strlen("a/b/c")));
    assert(_z_keyexpr_intersects("ab/**", strlen("ab/**"), "ab", strlen("ab")));
    assert(_z_keyexpr_intersects("**/xyz", strlen("**/xyz"), "a/b/xyz/d/e/f/xyz", strlen("a/b/xyz/d/e/f/xyz")));
    assert(
        !_z_keyexpr_intersects("**/xyz$*xyz", strlen("**/xyz$*xyz"), "a/b/xyz/d/e/f/xyz", strlen("a/b/xyz/d/e/f/xyz")));
    assert(
        _z_keyexpr_intersects("a/**/c/**/e", strlen("a/**/c/**/e"), "a/b/b/b/c/d/d/d/e", strlen("a/b/b/b/c/d/d/d/e")));
    assert(_z_keyexpr_intersects("a/**/c/**/e", strlen("a/**/c/**/e"), "a/c/e", strlen("a/c/e")));
    assert(_z_keyexpr_intersects("a/**/c/*/e/*", strlen("a/**/c/*/e/*"), "a/b/b/b/c/d/d/c/d/e/f",
                                 strlen("a/b/b/b/c/d/d/c/d/e/f")));
    assert(!_z_keyexpr_intersects("a/**/c/*/e/*", strlen("a/**/c/*/e/*"), "a/b/b/b/c/d/d/c/d/d/e/f",
                                  strlen("a/b/b/b/c/d/d/c/d/d/e/f")));
    assert(!_z_keyexpr_intersects("ab$*cd", strlen("ab$*cd"), "abxxcxxcdx", strlen("abxxcxxcdx")));
    assert(_z_keyexpr_intersects("x/abc", strlen("x/abc"), "x/abc", strlen("x/abc")));
    assert(!_z_keyexpr_intersects("x/abc", strlen("x/abc"), "abc", strlen("abc")));
    assert(_z_keyexpr_intersects("x/*", strlen("x/*"), "x/abc", strlen("x/abc")));
    assert(!_z_keyexpr_intersects("x/*", strlen("x/*"), "abc", strlen("abc")));
    assert(!_z_keyexpr_intersects("*", strlen("*"), "x/abc", strlen("x/abc")));
    assert(_z_keyexpr_intersects("x/*", strlen("x/*"), "x/abc$*", strlen("x/abc$*")));
    assert(_z_keyexpr_intersects("x/$*abc", strlen("x/$*abc"), "x/abc$*", strlen("x/abc$*")));
    assert(_z_keyexpr_intersects("x/a$*", strlen("x/a$*"), "x/abc$*", strlen("x/abc$*")));
    assert(_z_keyexpr_intersects("x/a$*de", strlen("x/a$*de"), "x/abc$*de", strlen("x/abc$*de")));
    assert(_z_keyexpr_intersects("x/a$*d$*e", strlen("x/a$*d$*e"), "x/a$*e", strlen("x/a$*e")));
    assert(_z_keyexpr_intersects("x/a$*d$*e", strlen("x/a$*d$*e"), "x/a$*c$*e", strlen("x/a$*c$*e")));
    assert(_z_keyexpr_intersects("x/a$*d$*e", strlen("x/a$*d$*e"), "x/ade", strlen("x/ade")));
    assert(!_z_keyexpr_intersects("x/c$*", strlen("x/c$*"), "x/abc$*", strlen("x/abc$*")));
    assert(!_z_keyexpr_intersects("x/$*d", strlen("x/$*d"), "x/$*e", strlen("x/$*e")));
    assert(_z_keyexpr_intersects("a", strlen("a"), "a", strlen("a")));
    assert(_z_keyexpr_intersects("a/b", strlen("a/b"), "a/b", strlen("a/b")));
    assert(_z_keyexpr_intersects("*", strlen("*"), "a", strlen("a")));
    assert(_z_keyexpr_intersects("a", strlen("a"), "*", strlen("*")));
    assert(_z_keyexpr_intersects("*", strlen("*"), "aaaaa", strlen("aaaaa")));
    assert(_z_keyexpr_intersects("**", strlen("**"), "a", strlen("a")));
    assert(_z_keyexpr_intersects("a", strlen("a"), "**", strlen("**")));
    assert(_z_keyexpr_intersects("**", strlen("**"), "a", strlen("a")));
    assert(_z_keyexpr_intersects("a/a/a/a", strlen("a/a/a/a"), "**", strlen("**")));
    assert(_z_keyexpr_intersects("a/*", strlen("a/*"), "a/b", strlen("a/b")));
    assert(!_z_keyexpr_intersects("a/*/b", strlen("a/*/b"), "a/b", strlen("a/b")));
    assert(_z_keyexpr_intersects("a/**/b", strlen("a/**/b"), "a/b", strlen("a/b")));
    assert(_z_keyexpr_intersects("a/b$*", strlen("a/b$*"), "a/b", strlen("a/b")));
    assert(_z_keyexpr_intersects("a/$*b$*", strlen("a/$*b$*"), "a/b", strlen("a/b")));
    assert(_z_keyexpr_intersects("a/$*b", strlen("a/$*b"), "a/b", strlen("a/b")));
    assert(_z_keyexpr_intersects("a/b$*", strlen("a/b$*"), "a/bc", strlen("a/bc")));
    assert(_z_keyexpr_intersects("a/$*b$*", strlen("a/$*b$*"), "a/ebc", strlen("a/ebc")));
    assert(_z_keyexpr_intersects("a/$*b", strlen("a/$*b"), "a/cb", strlen("a/cb")));
    assert(!_z_keyexpr_intersects("a/b$*", strlen("a/b$*"), "a/ebc", strlen("a/ebc")));
    assert(!_z_keyexpr_intersects("a/$*b", strlen("a/$*b"), "a/cbc", strlen("a/cbc")));
    assert(_z_keyexpr_intersects("a/**/b$*", strlen("a/**/b$*"), "a/b", strlen("a/b")));
    assert(_z_keyexpr_intersects("a/**/$*b$*", strlen("a/**/$*b$*"), "a/b", strlen("a/b")));
    assert(_z_keyexpr_intersects("a/**/$*b", strlen("a/**/$*b"), "a/b", strlen("a/b")));
    assert(_z_keyexpr_intersects("a/**/b$*", strlen("a/**/b$*"), "a/bc", strlen("a/bc")));
    assert(_z_keyexpr_intersects("a/**/$*b$*", strlen("a/**/$*b$*"), "a/ebc", strlen("a/ebc")));
    assert(_z_keyexpr_intersects("a/**/$*b", strlen("a/**/$*b"), "a/cb", strlen("a/cb")));
    assert(_z_keyexpr_intersects("a/**/b/c/**/d", strlen("a/**/b/c/**/d"), "a/b/b/b/c/d", strlen("a/b/b/b/c/d")));
    assert(
        !_z_keyexpr_intersects("a/**/b/c/**/d", strlen("a/**/b/c/**/d"), "a/b/b/b/c/@c/d", strlen("a/b/b/b/c/@c/d")));
    assert(!_z_keyexpr_intersects("a/**/b/c/**/d", strlen("a/**/b/c/**/d"), "a/b/@b/b/c/d", strlen("a/b/@b/b/c/d")));
    assert(_z_keyexpr_intersects("a/**/b/@b/**/b/c/**/d", strlen("a/**/b/@b/**/b/c/**/d"), "a/b/@b/b/c/d",
                                 strlen("a/b/@b/b/c/d")));
    assert(!_z_keyexpr_intersects("a/**/b$*", strlen("a/**/b$*"), "a/ebc", strlen("a/ebc")));
    assert(!_z_keyexpr_intersects("a/**/$*b", strlen("a/**/$*b"), "a/cbc", strlen("a/cbc")));

    assert(zp_keyexpr_intersect_null_terminated("a", "a"));
    assert(zp_keyexpr_intersect_null_terminated("a/b", "a/b"));
    assert(zp_keyexpr_intersect_null_terminated("*", "abc"));
    assert(zp_keyexpr_intersect_null_terminated("*", "abc"));
    assert(zp_keyexpr_intersect_null_terminated("*", "abc"));
    assert(zp_keyexpr_intersect_null_terminated("*", "xxx"));
    assert(zp_keyexpr_intersect_null_terminated("ab$*", "abcd"));
    assert(zp_keyexpr_intersect_null_terminated("ab$*d", "abcd"));
    assert(zp_keyexpr_intersect_null_terminated("ab$*", "ab"));
    assert(!zp_keyexpr_intersect_null_terminated("ab/*", "ab"));
    assert(zp_keyexpr_intersect_null_terminated("a/*/c/*/e", "a/b/c/d/e"));
    assert(zp_keyexpr_intersect_null_terminated("a/**/d/**/l", "a/b/c/d/e/f/g/h/i/l"));
    assert(zp_keyexpr_intersect_null_terminated("a/**/d/**/l", "a/d/foo/l"));
    assert(zp_keyexpr_intersect_null_terminated("a/$*b/c/$*d/e", "a/xb/c/xd/e"));
    assert(!zp_keyexpr_intersect_null_terminated("a/*/c/*/e", "a/c/e"));
    assert(!zp_keyexpr_intersect_null_terminated("a/*/c/*/e", "a/b/c/d/x/e"));
    assert(!zp_keyexpr_intersect_null_terminated("ab$*cd", "abxxcxxd"));
    assert(zp_keyexpr_intersect_null_terminated("ab$*cd", "abxxcxxcd"));
    assert(!zp_keyexpr_intersect_null_terminated("ab$*cd", "abxxcxxcdx"));
    assert(zp_keyexpr_intersect_null_terminated("**", "abc"));
    assert(zp_keyexpr_intersect_null_terminated("**", "a/b/c"));
    assert(zp_keyexpr_intersect_null_terminated("ab/**", "ab"));
    assert(zp_keyexpr_intersect_null_terminated("**/xyz", "a/b/xyz/d/e/f/xyz"));
    assert(!zp_keyexpr_intersect_null_terminated("**/xyz$*xyz", "a/b/xyz/d/e/f/xyz"));
    assert(zp_keyexpr_intersect_null_terminated("a/**/c/**/e", "a/b/b/b/c/d/d/d/e"));
    assert(zp_keyexpr_intersect_null_terminated("a/**/c/**/e", "a/c/e"));
    assert(zp_keyexpr_intersect_null_terminated("a/**/c/*/e/*", "a/b/b/b/c/d/d/c/d/e/f"));
    assert(!zp_keyexpr_intersect_null_terminated("a/**/c/*/e/*", "a/b/b/b/c/d/d/c/d/d/e/f"));
    assert(!zp_keyexpr_intersect_null_terminated("ab$*cd", "abxxcxxcdx"));
    assert(zp_keyexpr_intersect_null_terminated("x/abc", "x/abc"));
    assert(!zp_keyexpr_intersect_null_terminated("x/abc", "abc"));
    assert(zp_keyexpr_intersect_null_terminated("x/*", "x/abc"));
    assert(!zp_keyexpr_intersect_null_terminated("x/*", "abc"));
    assert(!zp_keyexpr_intersect_null_terminated("*", "x/abc"));
    assert(zp_keyexpr_intersect_null_terminated("x/*", "x/abc$*"));
    assert(zp_keyexpr_intersect_null_terminated("x/$*abc", "x/abc$*"));
    assert(zp_keyexpr_intersect_null_terminated("x/a$*", "x/abc$*"));
    assert(zp_keyexpr_intersect_null_terminated("x/a$*de", "x/abc$*de"));
    assert(zp_keyexpr_intersect_null_terminated("x/a$*d$*e", "x/a$*e"));
    assert(zp_keyexpr_intersect_null_terminated("x/a$*d$*e", "x/a$*c$*e"));
    assert(zp_keyexpr_intersect_null_terminated("x/a$*d$*e", "x/ade"));
    assert(!zp_keyexpr_intersect_null_terminated("x/c$*", "x/abc$*"));
    assert(!zp_keyexpr_intersect_null_terminated("x/$*d", "x/$*e"));
    assert(zp_keyexpr_intersect_null_terminated("a", "a"));
    assert(zp_keyexpr_intersect_null_terminated("a/b", "a/b"));
    assert(zp_keyexpr_intersect_null_terminated("*", "a"));
    assert(zp_keyexpr_intersect_null_terminated("a", "*"));
    assert(zp_keyexpr_intersect_null_terminated("*", "aaaaa"));
    assert(zp_keyexpr_intersect_null_terminated("**", "a"));
    assert(zp_keyexpr_intersect_null_terminated("a", "**"));
    assert(zp_keyexpr_intersect_null_terminated("**", "a"));
    assert(zp_keyexpr_intersect_null_terminated("a/a/a/a", "**"));
    assert(zp_keyexpr_intersect_null_terminated("a/*", "a/b"));
    assert(!zp_keyexpr_intersect_null_terminated("a/*/b", "a/b"));
    assert(zp_keyexpr_intersect_null_terminated("a/**/b", "a/b"));
    assert(zp_keyexpr_intersect_null_terminated("a/b$*", "a/b"));
    assert(zp_keyexpr_intersect_null_terminated("a/$*b$*", "a/b"));
    assert(zp_keyexpr_intersect_null_terminated("a/$*b", "a/b"));
    assert(zp_keyexpr_intersect_null_terminated("a/b$*", "a/bc"));
    assert(zp_keyexpr_intersect_null_terminated("a/$*b$*", "a/ebc"));
    assert(zp_keyexpr_intersect_null_terminated("a/$*b", "a/cb"));
    assert(!zp_keyexpr_intersect_null_terminated("a/b$*", "a/ebc"));
    assert(!zp_keyexpr_intersect_null_terminated("a/$*b", "a/cbc"));
    assert(zp_keyexpr_intersect_null_terminated("a/**/b$*", "a/b"));
    assert(zp_keyexpr_intersect_null_terminated("a/**/$*b$*", "a/b"));
    assert(zp_keyexpr_intersect_null_terminated("a/**/$*b", "a/b"));
    assert(zp_keyexpr_intersect_null_terminated("a/**/b$*", "a/bc"));
    assert(zp_keyexpr_intersect_null_terminated("a/**/$*b$*", "a/ebc"));
    assert(zp_keyexpr_intersect_null_terminated("a/**/$*b", "a/cb"));
    assert(!zp_keyexpr_intersect_null_terminated("a/**/b$*", "a/ebc"));
    assert(!zp_keyexpr_intersect_null_terminated("a/**/$*b", "a/cbc"));

    assert((zp_keyexpr_intersect_null_terminated("@a", "@a")));
    assert(!zp_keyexpr_intersect_null_terminated("@a", "@ab"));
    assert(!zp_keyexpr_intersect_null_terminated("@a", "@a/b"));
    assert(!zp_keyexpr_intersect_null_terminated("@a", "@a/*"));
    assert(!zp_keyexpr_intersect_null_terminated("@a", "@a/*/**"));
    assert(!zp_keyexpr_intersect_null_terminated("@a", "@a$*/**"));
    assert((zp_keyexpr_intersect_null_terminated("@a", "@a/**")));
    assert(!zp_keyexpr_intersect_null_terminated("**/xyz$*xyz", "@a/b/xyzdefxyz"));
    assert((zp_keyexpr_intersect_null_terminated("@a/**/c/**/e", "@a/b/b/b/c/d/d/d/e")));
    assert(!zp_keyexpr_intersect_null_terminated("@a/**/c/**/e", "@a/@b/b/b/c/d/d/d/e"));
    assert((zp_keyexpr_intersect_null_terminated("@a/**/@c/**/e", "@a/b/b/b/@c/d/d/d/e")));
    assert((zp_keyexpr_intersect_null_terminated("@a/**/e", "@a/b/b/d/d/d/e")));
    assert((zp_keyexpr_intersect_null_terminated("@a/**/e", "@a/b/b/b/d/d/d/e")));
    assert((zp_keyexpr_intersect_null_terminated("@a/**/e", "@a/b/b/c/d/d/d/e")));
    assert(!zp_keyexpr_intersect_null_terminated("@a/**/e", "@a/b/b/@c/b/d/d/d/e"));
    assert(!zp_keyexpr_intersect_null_terminated("@a/*", "@a/@b"));
    assert(!zp_keyexpr_intersect_null_terminated("@a/**", "@a/@b"));
    assert((zp_keyexpr_intersect_null_terminated("@a/**/@b", "@a/@b")));
    assert(!zp_keyexpr_intersect_null_terminated("@a/**/@b", "@a/**/@c/**/@b"));
    assert((zp_keyexpr_intersect_null_terminated("@a/@b/**", "@a/@b")));
    assert((zp_keyexpr_intersect_null_terminated("@a/**/@c/@b", "@a/**/@c/**/@b")));
    assert((zp_keyexpr_intersect_null_terminated("@a/**/@c/**/@b", "@a/**/@c/@b")));
}

void test_includes(void) {
    assert(_z_keyexpr_includes("a", strlen("a"), "a", strlen("a")));
    assert(_z_keyexpr_includes("a/b", strlen("a/b"), "a/b", strlen("a/b")));
    assert(_z_keyexpr_includes("*", strlen("*"), "a", strlen("a")));
    assert(!_z_keyexpr_includes("a", strlen("a"), "*", strlen("*")));
    assert(_z_keyexpr_includes("*", strlen("*"), "aaaaa", strlen("aaaaa")));
    assert(_z_keyexpr_includes("**", strlen("**"), "a", strlen("a")));
    assert(!_z_keyexpr_includes("a", strlen("a"), "**", strlen("**")));
    assert(_z_keyexpr_includes("**", strlen("**"), "a", strlen("a")));
    assert(_z_keyexpr_includes("**", strlen("**"), "a/a/a/a", strlen("a/a/a/a")));
    assert(_z_keyexpr_includes("**", strlen("**"), "*/**", strlen("*/**")));
    assert(_z_keyexpr_includes("*/**", strlen("*/**"), "*/**", strlen("*/**")));
    assert(!_z_keyexpr_includes("*/**", strlen("*/**"), "**", strlen("**")));
    assert(!_z_keyexpr_includes("a/a/a/a", strlen("a/a/a/a"), "**", strlen("**")));
    assert(_z_keyexpr_includes("a/*", strlen("a/*"), "a/b", strlen("a/b")));
    assert(!_z_keyexpr_includes("a/*/b", strlen("a/*/b"), "a/b", strlen("a/b")));
    assert(_z_keyexpr_includes("a/**/b", strlen("a/**/b"), "a/b", strlen("a/b")));
    assert(_z_keyexpr_includes("a/b$*", strlen("a/b$*"), "a/b", strlen("a/b")));
    assert(!_z_keyexpr_includes("a/b", strlen("a/b"), "a/b$*", strlen("a/b$*")));
    assert(_z_keyexpr_includes("a/$*b$*", strlen("a/$*b$*"), "a/b", strlen("a/b")));
    assert(_z_keyexpr_includes("a/$*b", strlen("a/$*b"), "a/b", strlen("a/b")));
    assert(_z_keyexpr_includes("a/b$*", strlen("a/b$*"), "a/bc", strlen("a/bc")));
    assert(_z_keyexpr_includes("a/$*b$*", strlen("a/$*b$*"), "a/ebc", strlen("a/ebc")));
    assert(_z_keyexpr_includes("a/$*b", strlen("a/$*b"), "a/cb", strlen("a/cb")));
    assert(!_z_keyexpr_includes("a/b$*", strlen("a/b$*"), "a/ebc", strlen("a/ebc")));
    assert(!_z_keyexpr_includes("a/$*b", strlen("a/$*b"), "a/cbc", strlen("a/cbc")));
    assert(_z_keyexpr_includes("a/**/b$*", strlen("a/**/b$*"), "a/b", strlen("a/b")));
    assert(_z_keyexpr_includes("a/**/$*b$*", strlen("a/**/$*b$*"), "a/b", strlen("a/b")));
    assert(_z_keyexpr_includes("a/**/$*b", strlen("a/**/$*b"), "a/b", strlen("a/b")));
    assert(_z_keyexpr_includes("a/**/b$*", strlen("a/**/b$*"), "a/bc", strlen("a/bc")));
    assert(_z_keyexpr_includes("a/**/$*b$*", strlen("a/**/$*b$*"), "a/ebc", strlen("a/ebc")));
    assert(_z_keyexpr_includes("a/**/$*b", strlen("a/**/$*b"), "a/cb", strlen("a/cb")));
    assert(!_z_keyexpr_includes("a/**/b$*", strlen("a/**/b$*"), "a/ebc", strlen("a/ebc")));
    assert(!_z_keyexpr_includes("a/**/$*b", strlen("a/**/$*b"), "a/cbc", strlen("a/cbc")));

    assert(zp_keyexpr_includes_null_terminated("a", "a"));
    assert(zp_keyexpr_includes_null_terminated("a/b", "a/b"));
    assert(zp_keyexpr_includes_null_terminated("*", "a"));
    assert(!zp_keyexpr_includes_null_terminated("a", "*"));
    assert(zp_keyexpr_includes_null_terminated("*", "aaaaa"));
    assert(zp_keyexpr_includes_null_terminated("**", "a"));
    assert(!zp_keyexpr_includes_null_terminated("a", "**"));
    assert(zp_keyexpr_includes_null_terminated("**", "a"));
    assert(zp_keyexpr_includes_null_terminated("**", "a/a/a/a"));
    assert(zp_keyexpr_includes_null_terminated("**", "*/**"));
    assert(zp_keyexpr_includes_null_terminated("*/**", "*/**"));
    assert(!zp_keyexpr_includes_null_terminated("*/**", "**"));
    assert(!zp_keyexpr_includes_null_terminated("a/a/a/a", "**"));
    assert(zp_keyexpr_includes_null_terminated("a/*", "a/b"));
    assert(!zp_keyexpr_includes_null_terminated("a/*/b", "a/b"));
    assert(zp_keyexpr_includes_null_terminated("a/**/b", "a/b"));
    assert(zp_keyexpr_includes_null_terminated("a/b$*", "a/b"));
    assert(!zp_keyexpr_includes_null_terminated("a/b", "a/b$*"));
    assert(zp_keyexpr_includes_null_terminated("a/$*b$*", "a/b"));
    assert(zp_keyexpr_includes_null_terminated("a/$*b", "a/b"));
    assert(zp_keyexpr_includes_null_terminated("a/b$*", "a/bc"));
    assert(zp_keyexpr_includes_null_terminated("a/$*b$*", "a/ebc"));
    assert(zp_keyexpr_includes_null_terminated("a/$*b", "a/cb"));
    assert(!zp_keyexpr_includes_null_terminated("a/b$*", "a/ebc"));
    assert(!zp_keyexpr_includes_null_terminated("a/$*b", "a/cbc"));
    assert(zp_keyexpr_includes_null_terminated("a/**/b$*", "a/b"));
    assert(zp_keyexpr_includes_null_terminated("a/**/$*b$*", "a/b"));
    assert(zp_keyexpr_includes_null_terminated("a/**/$*b", "a/b"));
    assert(zp_keyexpr_includes_null_terminated("a/**/b$*", "a/bc"));
    assert(zp_keyexpr_includes_null_terminated("a/**/$*b$*", "a/ebc"));
    assert(zp_keyexpr_includes_null_terminated("a/**/$*b", "a/cb"));
    assert(!zp_keyexpr_includes_null_terminated("a/**/b$*", "a/ebc"));
    assert(!zp_keyexpr_includes_null_terminated("a/**/$*b", "a/cbc"));

    assert((zp_keyexpr_includes_null_terminated("@a", "@a")));
    assert(!zp_keyexpr_includes_null_terminated("@a", "@ab"));
    assert(!zp_keyexpr_includes_null_terminated("@a", "@a/b"));
    assert(!zp_keyexpr_includes_null_terminated("@a", "@a/*"));
    assert(!zp_keyexpr_includes_null_terminated("@a", "@a/*/**"));
    assert(!zp_keyexpr_includes_null_terminated("@a$*/**", "@a"));
    assert(!zp_keyexpr_includes_null_terminated("@a", "@a/**"));
    assert((zp_keyexpr_includes_null_terminated("@a/**", "@a")));
    assert(!zp_keyexpr_includes_null_terminated("**/xyz$*xyz", "@a/b/xyzdefxyz"));
    assert((zp_keyexpr_includes_null_terminated("@a/**/c/**/e", "@a/b/b/b/c/d/d/d/e")));
    assert(!zp_keyexpr_includes_null_terminated("@a/*", "@a/@b"));
    assert(!zp_keyexpr_includes_null_terminated("@a/**", "@a/@b"));
    assert((zp_keyexpr_includes_null_terminated("@a/**/@b", "@a/@b")));
    assert((zp_keyexpr_includes_null_terminated("@a/@b/**", "@a/@b")));
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
    }

    for (int i = 0; i < N; i++) {
        const char *ke = input[i];
        char *canon = (char *)malloc(128);
        memset(canon, 0, 128);
        strncpy(canon, ke, 128);
        size_t canon_len = strlen(canon);
        zp_keyexpr_canon_status_t status = zp_keyexpr_canonize_null_terminated(canon);
        printf("%s ", ke);
        printf("  Status: %d : %d", status, expected[i]);
        assert(status == expected[i]);
        if (status == Z_KEYEXPR_CANON_SUCCESS) {
            printf("  Match: %.*s : %s", (int)canon_len, canon, canonized[i]);
            assert(strcmp(canonized[i], canon) == 0);
        }
        printf("\n");
    }
}

void test_equals(void) {
    assert(!zp_keyexpr_equals_null_terminated("a/**/$*b", "a/cb"));
    assert(!zp_keyexpr_equals_null_terminated("a/bc", "a/cb"));
    assert(zp_keyexpr_equals_null_terminated("greetings/hello/there", "greetings/hello/there"));
}

_Bool keyexpr_equals_string(const z_loaned_keyexpr_t *ke, const char *s) {
    z_view_string_t vs;
    z_keyexpr_as_view_string(ke, &vs);
    return strncmp(z_string_data(z_view_string_loan(&vs)), s, z_string_len(z_view_string_loan(&vs))) == 0;
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
    assert(!z_keyexpr_check(&ke2));

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
