#!/usr/bin/env bash

set -xeo pipefail

# Repository
readonly repo=${REPO:?input REPO is required}
# Release number
readonly version=${VERSION:?input VERSION is required}

BUILD_TYPE=RELEASE make

readonly out=$GITHUB_WORKSPACE
readonly repo_name=${repo#*/}
readonly archive_lib=$out/$repo_name-$version-macos-x64.zip
readonly archive_examples=$out/$repo_name-$version-macos-x64-examples.zip

cd build
zip -r "$archive_lib" lib
cd ..
zip -r "$archive_lib" include

cd build/examples
zip "$archive_examples" ./*
cd ..

{ echo "archive-lib=$(basename "$archive_lib")";
  echo "archive-examples=$(basename "$archive_examples")";
} >> "$GITHUB_OUTPUT"
