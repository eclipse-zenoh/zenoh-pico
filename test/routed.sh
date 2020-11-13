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

echo "------------------ Running test $1 -------------------"
LINK=""
if [[ ! -f zenohd ]]; then
    if [[ "$OSTYPE" == "darwin"* ]]; then 
        LINK="https://download.eclipse.org/zenoh/zenoh/master/eclipse-zenoh-0.5.0-beta.5-macosx10.7-x86-64.tgz"
    elif [[ "$OSTYPE" == "linux-gnu" ]]; then 
        LINK="https://download.eclipse.org/zenoh/zenoh/master/eclipse-zenoh-0.5.0-beta.5-x86_64-unknown-linux-gnu.tgz"
    else 
       exit -1
    fi
fi

echo "> Downloading $LINK ..."
curl -L -o zenohd.tar.gz $LINK
tar -xzf zenohd.tar.gz
chmod +x zenohd

echo "> Running zenohd ..."
RUST_LOG=debug ./zenohd &> zenohd.$1.log &
ZPID=$!
sleep 1

echo "> Running $1 ..."
./$1
RETCODE=$?

echo "> Stopping zenohd ..."
kill -9 $ZPID

echo "> Done ($RETCODE)."
exit $RETCODE
