#!/usr/bin/env bash

set -xeo pipefail

readonly live_run=${LIVE_RUN:-false}
# Release number
readonly version=${VERSION:-input VERSION is required}
# Git actor name
readonly git_user_name=${GIT_USER_NAME:?input GIT_USER_NAME is required}
# Git actor email
readonly git_user_email=${GIT_USER_EMAIL:?input GIT_USER_EMAIL is required}

export GIT_AUTHOR_NAME=$git_user_name
export GIT_AUTHOR_EMAIL=$git_user_email
export GIT_COMMITTER_NAME=$git_user_name
export GIT_COMMITTER_EMAIL=$git_user_email

# Bump CMake project version
printf '%s' "$version" > version.txt

# Refresh the in-tree copies of zenoh-pico.h and library.json so the
# release commit (and the tag that points at it) is internally
# consistent. A short CMake configure is enough — configure_file()
# writes the files into the source tree as part of the normal
# configure step.
cmake -S . -B build-bump -DBUILD_TESTING=OFF -DBUILD_EXAMPLES=OFF
rm -rf build-bump

git commit version.txt include/zenoh-pico.h library.json \
  -m "chore: Bump version to $version"
if [[ ${live_run} == true ]]; then
  git tag --force "$version" -m "v$version"
  git show-ref --tags
  git --no-pager log -10
  git push --force origin
  git push --force origin "$version"
else
  git --no-pager log -10
  git push --force origin
fi
