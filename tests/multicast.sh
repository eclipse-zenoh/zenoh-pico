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

cd "$TESTDIR" || exit

echo "------------------ Running test $TESTBIN -------------------"

INTERFACES=$(ifconfig -l)
for INTERFACE in $(echo "$INTERFACES" | xargs); do
    INET=$(ifconfig "$INTERFACE" | awk '$1 == "inet" {print $2}')
    if [ "$INET" != "127.0.0.1" ] && [ "$INET" != "" ]; then
        break
    fi
done

LOCATORS="udp/224.0.0.225:7447#iface=$INTERFACE"
for LOCATOR in $(echo "$LOCATORS" | xargs); do
    sleep 1

    echo "> Running $TESTBIN $LOCATOR..."
    "$TESTBIN" "$LOCATOR"
    RETCODE=$?

    [ "$RETCODE" -lt 0 ] && exit "$RETCODE"
done

INTERFACES=$(ifconfig -l)
for INTERFACE in $(echo "$INTERFACES" | xargs); do
    INET=$(ifconfig "$INTERFACE" | awk '$1 == "inet6" {print $2}')
    if [ "$INET" != "::1" ] && [ "$INET" != "" ]; then
        break
    fi
done

LOCATORS="udp/[ff10::1234]:7447#iface=$INTERFACE"
for LOCATOR in $(echo "$LOCATORS" | xargs); do
    sleep 1

    echo "> Running $TESTBIN $LOCATOR..."
    "$TESTBIN" "$LOCATOR"
    RETCODE=$?

    [ "$RETCODE" -lt 0 ] && exit "$RETCODE"
done

echo "> Done ($RETCODE)."
exit "$RETCODE"
