#!/bin/sh
#
# Copyright (c) 2017, 2020 ADLINK Technology Inc.
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
# which is available at https://www.apache.org/licenses/LICENSE-2.0.
#
# SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
#
# Contributors:
#   ADLINK zenoh team, <zenoh@adlink-labs.tech>
#

TESTBIN=$1
TESTDIR=$(dirname $0)

cd $TESTDIR

echo "------------------ Running test $TESTBIN -------------------"

sleep 5

if [ ! -f zenohd ]; then
    git clone https://github.com/eclipse-zenoh/zenoh.git zenoh-git
    cd zenoh-git
    git checkout c19144e2dc40c45e80f3c13e5ad232bb73e44929
    cargo build
    cp ./target/debug/zenohd $TESTDIR/
    cd $TESTDIR
fi

chmod +x zenohd

#LOCATORS="tcp/127.0.0.1:7447 udp/127.0.0.1:7447 tcp/[::1]:7447 udp/[::1]:7447"
LOCATORS="tcp/127.0.0.1:7447 tcp/[::1]:7447"

for LOCATOR in $(echo $LOCATORS | xargs); do
    sleep 1

    echo "> Running zenohd ..."
    RUST_LOG=debug ./zenohd -l $LOCATOR &> zenohd.$TESTBIN.log &
    ZPID=$!

    sleep 5

    echo "> Running $TESTBIN ..."
    ./$TESTBIN $LOCATOR
    RETCODE=$?

    echo "> Stopping zenohd ..."
    kill -9 $ZPID

    echo "> Logs of zenohd ..."
    cat zenohd.$TESTBIN.log

    [ $RETCODE -lt 0 ] && exit $RETCODE
done

echo "> Done ($RETCODE)."
exit $RETCODE
