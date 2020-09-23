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
if [[ ! -f zenohd ]]; then
  if [[ "$OSTYPE" == "darwin"* ]]; then 
    echo "> Downloading https://github.com/atolab/atobin/raw/master/zenoh/unstable/macos/10.14.6/zenohd ..."
    curl -L -o zenohd https://github.com/atolab/atobin/raw/master/zenoh/unstable/macos/10.14.6/zenohd
  elif [[ "$OSTYPE" == "linux-gnu" ]] && [[ -f /etc/redhat-release ]]; then 
    echo "> Downloading https://github.com/atolab/atobin/raw/master/zenoh/latest/centos/7.2.1511/zenohd ..."
    curl -L -o zenohd https://github.com/atolab/atobin/raw/master/zenoh/latest/centos/7.2.1511/zenohd
  else 
    echo "> Downloading https://github.com/atolab/atobin/raw/master/zenoh/unstable/ubuntu/16.04/zenohd ..."
    curl -L -o zenohd https://github.com/atolab/atobin/raw/master/zenoh/unstable/ubuntu/16.04/zenohd
  fi
fi
chmod +x zenohd

echo "> Running zenohd ..."
./zenohd --verbosity=info > zenohd.$1.log &
ZPID=$!
sleep 0.2

echo "> Running $1 ..."
./$1
RETCODE=$?

echo "> Stopping zenohd ..."
kill -9 $ZPID

echo "> Done ($RETCODE)."
exit $RETCODE