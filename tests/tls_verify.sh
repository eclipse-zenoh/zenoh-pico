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

PORT=${ZENOH_PORT:-7447}
rm -f ca_${PORT}.pem ca_${PORT}-key.pem ca_${PORT}.srl server_${PORT}.pem server_${PORT}-key.pem
cat > server_${PORT}.cnf <<EOF
[ req ]
prompt = no
distinguished_name = dn
req_extensions = san

[ dn ]
CN = localhost

[ san ]
subjectAltName = DNS:localhost
EOF

openssl req -x509 -newkey rsa:2048 -keyout ca_${PORT}-key.pem -out ca_${PORT}.pem -days 30 -nodes -subj "/CN=Test CA"
openssl req -newkey rsa:2048 -keyout server_${PORT}-key.pem -out server_${PORT}.csr -nodes -config server_${PORT}.cnf
openssl x509 -req -in server_${PORT}.csr -CA ca_${PORT}.pem -CAkey ca_${PORT}-key.pem -CAcreateserial -out server_${PORT}.pem -days 30 \
    -extfile server_${PORT}.cnf -extensions san
rm -f server_${PORT}.csr server_${PORT}.cnf

cat > client_${PORT}.cnf <<EOF
[ req ]
prompt = no
distinguished_name = dn
req_extensions = v3_req

[ dn ]
CN = Test Client

[ v3_req ]
basicConstraints = CA:FALSE
keyUsage = critical, digitalSignature, keyEncipherment
extendedKeyUsage = clientAuth
EOF

openssl req -newkey rsa:2048 -keyout client_${PORT}-key.pem -out client_${PORT}.csr -nodes -config client_${PORT}.cnf
openssl x509 -req -in client_${PORT}.csr -CA ca_${PORT}.pem -CAkey ca_${PORT}-key.pem -CAcreateserial -out client_${PORT}.pem -days 30 \
    -extfile client_${PORT}.cnf -extensions v3_req
rm -f client_${PORT}.csr client_${PORT}.cnf

cat > zenohd_tls_${PORT}.json <<EOF
{
  "transport": {
    "link": {
      "tls": {
        "root_ca_certificate": "$TESTDIR/ca_${PORT}.pem",
        "listen_private_key": "$TESTDIR/server_${PORT}-key.pem",
        "listen_certificate": "$TESTDIR/server_${PORT}.pem"
      }
    }
  }
}
EOF

LOC_BASE="tls/127.0.0.1:$PORT"

run_test() {
    locator="$1"
    expect_success="$2"
    config_file="${3:-zenohd_tls_${PORT}.json}"

    echo "> Running ./zenohd -c $config_file -l ${LOC_BASE} ..."
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

run_test "$LOC_BASE#root_ca_certificate=$TESTDIR/ca_${PORT}.pem" 0
run_test "$LOC_BASE#root_ca_certificate=$TESTDIR/ca_${PORT}.pem;verify_name_on_connect=0" 1

cat > zenohd_tls_mtls_${PORT}.json <<EOF
{
  "transport": {
    "link": {
      "tls": {
        "root_ca_certificate": "$TESTDIR/ca_${PORT}.pem",
        "listen_private_key": "$TESTDIR/server_${PORT}-key.pem",
        "listen_certificate": "$TESTDIR/server_${PORT}.pem",
        "enable_mtls": true
      }
    }
  }
}
EOF

run_test "$LOC_BASE#root_ca_certificate=$TESTDIR/ca_${PORT}.pem;enable_mtls=1" 0 zenohd_tls_mtls_${PORT}.json
run_test "$LOC_BASE#root_ca_certificate=$TESTDIR/ca_${PORT}.pem;enable_mtls=1;connect_private_key=$TESTDIR/client_${PORT}-key.pem;connect_certificate=$TESTDIR/client_${PORT}.pem;verify_name_on_connect=0" 1 zenohd_tls_mtls_${PORT}.json

rm -f client.z_tls_verify.log zenohd.z_tls_verify.log zenohd_tls_mtls_${PORT}.json
rm -f client_${PORT}.pem client_${PORT}-key.pem server_${PORT}.pem server_${PORT}-key.pem ca_${PORT}.pem ca_${PORT}-key.pem ca_${PORT}.srl
exit 0
