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

cd $(dirname $0)

echo "------------------ Running test $TESTBIN -------------------"

sleep 5

if [ ! -f zenohd ]; then
    VERSION=$(curl --silent "https://api.github.com/repos/eclipse-zenoh/zenoh/releases" | grep '"name"' | head -n1 | cut -d'"' -f4 | cut -d"v" -f2)
    if [ -z "$VERSION" ]; then
        echo "Unable to retrieve zenoh version ..."
        exit -1
    fi

    echo "> Target zenoh version $VERSION ..."
    LINK=""
    case "$OSTYPE" in
    "darwin"*)
        LINK="https://download.eclipse.org/zenoh/zenoh/master/eclipse-zenoh-$VERSION-macosx10.7-x86-64.tgz"
    ;;
    "linux-gnu"*) 
        LINK="https://download.eclipse.org/zenoh/zenoh/master/eclipse-zenoh-$VERSION-x86_64-unknown-linux-gnu.tgz"
    ;;
    *) 
       exit -1
    ;;
    esac

    echo "> Downloading $LINK ..."
    curl --location --progress-bar --output zenohd.tar.gz $LINK
    tar -xzf zenohd.tar.gz
fi

chmod +x zenohd

echo "> Running zenohd ..."
RUST_LOG=debug ./zenohd &> zenohd.$TESTBIN.log &
ZPID=$!

sleep 5

LOCATOR="tcp/127.0.0.1:7447"
echo "> Running $TESTBIN ..."
./$TESTBIN $LOCATOR
RETCODE=$?

echo "> Stopping zenohd ..."
kill -9 $ZPID

echo "> Logs of zenohd ..."
cat zenohd.$TESTBIN.log

echo "> Done ($RETCODE)."
exit $RETCODE
