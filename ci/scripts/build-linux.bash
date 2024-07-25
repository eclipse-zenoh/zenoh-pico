#!/usr/bin/env bash

set -xeo pipefail

# Repository
readonly repo=${REPO:?input REPO is required}
# Release number
readonly version=${VERSION:?input VERSION is required}
# Build target
readonly target=${TARGET:?input TARGET is required}

BUILD_TYPE=RELEASE make "$target"

readonly out=$GITHUB_WORKSPACE
readonly repo_name=${repo#*/}
readonly archive_lib=$out/$repo_name-$version-$target.zip
readonly archive_deb=$out/$repo_name-$version-$target-deb-pkgs.zip
readonly archive_rpm=$out/$repo_name-$version-$target-rpm-pkgs.zip
readonly archive_examples=$out/$repo_name-$version-$target-examples.zip

cd crossbuilds/"$target"
zip -r "$archive_lib" lib
cd -
zip -r "$archive_lib" include

cd crossbuilds/"$target"/packages
zip "$archive_deb" ./*.deb
zip "$archive_rpm" ./*.rpm
cd -

cd crossbuilds/"$target"/examples
zip "$archive_examples" ./*
cd -

{ echo "archive-lib=$(basename "$archive_lib")";
  echo "archive-deb=$(basename "$archive_deb")";
  echo "archive-rpm=$(basename "$archive_rpm")";
  echo "archive-examples=$(basename "$archive_examples")";
} >> "$GITHUB_OUTPUT"
