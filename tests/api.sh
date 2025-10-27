#!/bin/sh
#
# Copyright (c) 2022 ZettaScale Technology
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
# which is available at https://www.apache.org/licenses/LICENSE-2.0.
#
# SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
#
# Contributors:
#   ZettaScale Zenoh Team, <zenoh@zettascale.tech>
#

TESTBIN="$1"
TESTDIR=$(dirname "$0")

if [ "$OSTYPE" = "msys" ]; then
  TESTBIN="$TESTDIR/Debug/$TESTBIN.exe"
else
  TESTBIN="./$TESTBIN"
fi

cd "$TESTDIR"|| exit

echo "------------------ Running test $TESTBIN -------------------"

sleep 5

if [ ! -f zenohd ]; then
    git clone https://github.com/eclipse-zenoh/zenoh.git zenoh-git
    cd zenoh-git || exit
    if [ -n "$ZENOH_BRANCH" ]; then
        git switch "$ZENOH_BRANCH"
    fi
    rustup show
    cargo build --lib --bin zenohd
    cp ./target/debug/zenohd "$TESTDIR"/
    cd "$TESTDIR"|| exit
fi

chmod +x zenohd

LOCATORS="tcp/127.0.0.1:7447"
for LOCATOR in $(echo "$LOCATORS" | xargs); do
    sleep 1

    echo "> Running zenohd ... $LOCATOR"
    RUST_LOG=debug ./zenohd --plugin-search-dir "$TESTDIR/zenoh-git/target/debug" -l "$LOCATOR" > zenohd."$1".log 2>&1 &
    ZPID=$!

    sleep 5

    echo "> Running $TESTBIN ..."
    "$TESTBIN" "$LOCATOR"
    RETCODE=$?

    echo "> Stopping zenohd ..."
    kill -9 "$ZPID"

    sleep 1

    echo "> Logs of zenohd ..."
    cat zenohd."$1".log

    [ "$RETCODE" -lt 0 ] && exit "$RETCODE"
done

echo "> Done ($RETCODE)."
exit "$RETCODE"
