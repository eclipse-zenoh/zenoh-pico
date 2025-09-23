#!/bin/sh
#
# Copyright (c) 2025 ZettaScale Technology
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

cd "$TESTDIR" || exit 1

if [ ! -f zenohd ]; then
    git clone https://github.com/eclipse-zenoh/zenoh.git zenoh-git
    cd zenoh-git || exit 1
    if [ -n "$ZENOH_BRANCH" ]; then
        git switch "$ZENOH_BRANCH"
    fi
    cargo build --lib --bin zenohd
    cp ./target/debug/zenohd "$TESTDIR"/
    cd "$TESTDIR" || exit 1
fi

chmod +x zenohd

rm -f ca.pem ca-key.pem ca.srl server.pem server-key.pem server.csr
cat > server.cnf <<EOF
[ req ]
prompt = no
distinguished_name = dn
req_extensions = san

[ dn ]
CN = localhost

[ san ]
subjectAltName = DNS:localhost
EOF

openssl req -x509 -newkey rsa:2048 -keyout ca-key.pem -out ca.pem -days 30 -nodes -subj "/CN=Test CA"
openssl req -newkey rsa:2048 -keyout server-key.pem -out server.csr -nodes -config server.cnf
openssl x509 -req -in server.csr -CA ca.pem -CAkey ca-key.pem -CAcreateserial -out server.pem -days 30 \
    -extfile server.cnf -extensions san
rm -f server.csr server.cnf

openssl req -newkey rsa:2048 -keyout client-key.pem -out client.csr -nodes -subj "/CN=Test Client"
openssl x509 -req -in client.csr -CA ca.pem -CAkey ca-key.pem -CAcreateserial -out client.pem -days 30
rm -f client.csr

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

LOC_BASE="tls/127.0.0.1:7447"

run_test() {
    locator="$1"
    expect_success="$2"
    config_file="${3:-zenohd_tls.json}"

    echo "> Running zenohd -c $config_file ..."
    RUST_LOG=warn ./zenohd -c "$config_file" --plugin-search-dir "$TESTDIR/zenoh-git/target/debug" \
        -l "$LOC_BASE" > zenohd.z_tls_verify.log 2>&1 &
    ZPID=$!
    sleep 5

    echo "> Running $TESTBIN \"$locator\" ..."
    "$TESTBIN" "$locator" --msgs=1 > client.z_tls_verify.log 2>&1
    RETCODE=$?

    echo "> Stopping zenohd ..."
    kill -9 "$ZPID" 2>/dev/null || true
    wait "$ZPID" 2>/dev/null || true
    sleep 1

    if [ "$expect_success" = "1" ]; then
        if [ "$RETCODE" -ne 0 ]; then
            echo "Expected success but client exited with $RETCODE" >&2
            cat client.z_tls_verify.log >&2
            exit 1
        fi
    else
        if [ "$RETCODE" -eq 0 ]; then
            echo "Expected failure but client succeeded" >&2
            cat client.z_tls_verify.log >&2
            exit 1
        fi
    fi
}

run_test "$LOC_BASE#root_ca_certificate=$TESTDIR/ca.pem" 0
run_test "$LOC_BASE#root_ca_certificate=$TESTDIR/ca.pem;verify_name_on_connect=0" 1

cat > zenohd_tls_mtls.json <<EOF
{
  "transport": {
    "link": {
      "tls": {
        "root_ca_certificate": "$TESTDIR/ca.pem",
        "listen_private_key": "$TESTDIR/server-key.pem",
        "listen_certificate": "$TESTDIR/server.pem",
        "enable_mtls": true
      }
    }
  }
}
EOF

run_test "$LOC_BASE#root_ca_certificate=$TESTDIR/ca.pem;enable_mtls=1" 0 zenohd_tls_mtls.json
run_test "$LOC_BASE#root_ca_certificate=$TESTDIR/ca.pem;enable_mtls=1;connect_private_key=$TESTDIR/client-key.pem;connect_certificate=$TESTDIR/client.pem;verify_name_on_connect=0" 1 zenohd_tls_mtls.json

rm -f client.z_tls_verify.log zenohd.z_tls_verify.log zenohd_tls_mtls.json
rm -f client.pem client-key.pem server.pem server-key.pem ca.pem ca-key.pem ca.srl
exit 0
