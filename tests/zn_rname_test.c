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
#include "zenoh-pico/protocol/utils.h"

int main(void)
{
    assert(_z_rname_intersect("/", "/"));
    assert(_z_rname_intersect("/a", "/a"));
    assert(_z_rname_intersect("/a/", "/a"));
    assert(_z_rname_intersect("/a", "/a/"));
    assert(_z_rname_intersect("/a/b", "/a/b"));
    assert(_z_rname_intersect("/*", "/abc"));
    assert(_z_rname_intersect("/*", "/abc/"));
    assert(_z_rname_intersect("/*/", "/abc"));
    assert(!_z_rname_intersect("/*", "/"));
    assert(!_z_rname_intersect("/*", "xxx"));
    assert(_z_rname_intersect("/ab*", "/abcd"));
    assert(_z_rname_intersect("/ab*d", "/abcd"));
    assert(_z_rname_intersect("/ab*", "/ab"));
    assert(!_z_rname_intersect("/ab/*", "/ab"));
    assert(_z_rname_intersect("/a/*/c/*/e", "/a/b/c/d/e"));
    assert(_z_rname_intersect("/a/**/d/**/l", "/a/b/c/d/e/f/g/h/i/l"));
    assert(_z_rname_intersect("/a/**/d/**/l", "/a/d/foo/l"));
    assert(_z_rname_intersect("/a/*b/c/*d/e", "/a/xb/c/xd/e"));
    assert(!_z_rname_intersect("/a/*/c/*/e", "/a/c/e"));
    assert(!_z_rname_intersect("/a/*/c/*/e", "/a/b/c/d/x/e"));
    assert(!_z_rname_intersect("/ab*cd", "/abxxcxxd"));
    assert(_z_rname_intersect("/ab*cd", "/abxxcxxcd"));
    assert(!_z_rname_intersect("/ab*cd", "/abxxcxxcdx"));
    assert(_z_rname_intersect("/**", "/abc"));
    assert(_z_rname_intersect("/**", "/a/b/c"));
    assert(_z_rname_intersect("/**", "/a/b/c/"));
    assert(_z_rname_intersect("/**/", "/a/b/c"));
    assert(_z_rname_intersect("/**/", "/"));
    assert(_z_rname_intersect("/ab/**", "/ab"));
    assert(_z_rname_intersect("/**/xyz", "/a/b/xyz/d/e/f/xyz"));
    assert(!_z_rname_intersect("/**/xyz*xyz", "/a/b/xyz/d/e/f/xyz"));
    assert(_z_rname_intersect("/a/**/c/**/e", "/a/b/b/b/c/d/d/d/e"));
    assert(_z_rname_intersect("/a/**/c/**/e", "/a/c/e"));
    assert(_z_rname_intersect("/a/**/c/*/e/*", "/a/b/b/b/c/d/d/c/d/e/f"));
    assert(!_z_rname_intersect("/a/**/c/*/e/*", "/a/b/b/b/c/d/d/c/d/d/e/f"));
    assert(!_z_rname_intersect("/ab*cd", "/abxxcxxcdx"));
    assert(_z_rname_intersect("/x/abc", "/x/abc"));
    assert(!_z_rname_intersect("/x/abc", "/abc"));
    assert(_z_rname_intersect("/x/*", "/x/abc"));
    assert(!_z_rname_intersect("/x/*", "/abc"));
    assert(!_z_rname_intersect("/*", "/x/abc"));
    assert(_z_rname_intersect("/x/*", "/x/abc*"));
    assert(_z_rname_intersect("/x/*abc", "/x/abc*"));
    assert(_z_rname_intersect("/x/a*", "/x/abc*"));
    assert(_z_rname_intersect("/x/a*de", "/x/abc*de"));
    assert(_z_rname_intersect("/x/a*d*e", "/x/a*e"));
    assert(_z_rname_intersect("/x/a*d*e", "/x/a*c*e"));
    assert(_z_rname_intersect("/x/a*d*e", "/x/ade"));
    assert(!_z_rname_intersect("/x/c*", "/x/abc*"));
    assert(!_z_rname_intersect("/x/*d", "/x/*e"));

    return 0;
}
