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
    cd "$TESTDIR" || exit
fi

chmod +x zenohd

LOCATORS="tcp/127.0.0.1:7447 tcp/[::1]:7447 udp/127.0.0.1:7447 udp/[::1]:7447"
ENABLE_TLS=0
if [ "$2" = "--enable-tls" ]; then
    ENABLE_TLS=1
    LOCATORS="$LOCATORS tls/127.0.0.1:7447#root_ca_certificate=$TESTDIR/ca.pem tls/[::1]:7447#root_ca_certificate=$TESTDIR/ca.pem"

    rm -f ca.pem ca-key.pem ca.srl server.pem server-key.pem server.csr

    cat > server.cnf <<EOF
[req]
prompt = no
distinguished_name = dn
req_extensions = san

[dn]
CN = localhost

[san]
subjectAltName = DNS:localhost,IP:127.0.0.1,IP:::1
EOF

    openssl req -x509 -newkey rsa:2048 -keyout ca-key.pem -out ca.pem -days 365 -nodes -subj "/CN=Test CA"
    openssl req -newkey rsa:2048 -keyout server-key.pem -out server.csr -nodes -config server.cnf
    openssl x509 -req -in server.csr -CA ca.pem -CAkey ca-key.pem -CAcreateserial -out server.pem -days 365 \
        -extfile server.cnf -extensions san
    rm server.csr server.cnf
fi
for LOCATOR in $(echo "$LOCATORS" | xargs); do
    sleep 1

    echo "> Running zenohd ... $LOCATOR"
    if [ "$ENABLE_TLS" = "1" ]; then
        cat > zenohd_tls.json <<EOF
{
  "transport": {
    "link": {
      "tls": {
        "root_ca_certificate": "$TESTDIR/ca.pem",
        "listen_private_key": "$TESTDIR/server-key.pem",
        "listen_certificate": "$TESTDIR/server.pem"
      }
    }
  }
}
EOF
        RUST_LOG=debug ./zenohd -c zenohd_tls.json --plugin-search-dir "$TESTDIR/zenoh-git/target/debug" -l "$LOCATOR" > zenohd."$1".log 2>&1 &
    else
        RUST_LOG=debug ./zenohd --plugin-search-dir "$TESTDIR/zenoh-git/target/debug" -l "$LOCATOR" > zenohd."$1".log 2>&1 &
    fi
    ZPID=$!

    sleep 5

    echo "> Running $TESTBIN ..."
    "$TESTBIN" "$LOCATOR" > client."$1".log 2>&1
    RETCODE=$?

    echo "> Stopping zenohd ..."
    kill -9 "$ZPID"
    wait "$ZPID"
    sleep 1

    if [ "$RETCODE" -lt 0 ]; then
        echo "> Logs of client ..."
        cat client."$1".log
        echo "> Logs of zenohd ..."
        cat zenohd."$1".log
        exit "$RETCODE"
    fi
done

echo "> Done ($RETCODE)."
exit "$RETCODE"
