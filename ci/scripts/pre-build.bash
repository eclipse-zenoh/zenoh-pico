#!/usr/bin/env bash

set -xeo pipefail

# Extract cross-compilation targets from the Makefile
crossbuild_targets=$(sed -n 's/^CROSSBUILD_TARGETS=\(.*\)/\1/p' GNUmakefile | head -n1)
target_matrix="{\"target\": [\"${crossbuild_targets// /\",\"}\"]}"
echo "build-linux-matrix=$target_matrix" >> "$GITHUB_OUTPUT"
