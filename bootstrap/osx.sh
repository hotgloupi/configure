#!/bin/sh

# Usage: ./bootstrap/osx.sh

set -x
set -e

SCRIPT="$0"
SCRIPT_DIR="$(cd "$(dirname ${SCRIPT})" && pwd)"

CXX="${CXX:-clang++}" CXXFLAGS="${CXXFLAGS:-} -stdlib=libc++" "${SCRIPT_DIR}/unix.sh"
