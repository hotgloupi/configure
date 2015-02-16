#!/bin/sh

# Usage: ./bootstrap/osx.sh

set -x
set -e

SCRIPT="$0"
SCRIPT_DIR="$(cd "$(dirname ${SCRIPT})" && pwd)"
LUA_SRC_DIR="${SCRIPT_DIR}/../lua/src"
SRC_DIR="${SCRIPT_DIR}/../src"
CXX="${CXX:-clang++}"
TMP_DIR="${TMP_DIR:-/tmp}"
BIN="${TMP_DIR}/configure"

LUA_SRCS="
${LUA_SRC_DIR}/lapi.c
${LUA_SRC_DIR}/lcode.c
${LUA_SRC_DIR}/lctype.c
${LUA_SRC_DIR}/ldebug.c
${LUA_SRC_DIR}/ldo.c
${LUA_SRC_DIR}/ldump.c
${LUA_SRC_DIR}/lfunc.c
${LUA_SRC_DIR}/lgc.c
${LUA_SRC_DIR}/llex.c
${LUA_SRC_DIR}/lmem.c
${LUA_SRC_DIR}/lobject.c
${LUA_SRC_DIR}/lopcodes.c
${LUA_SRC_DIR}/lparser.c
${LUA_SRC_DIR}/lstate.c
${LUA_SRC_DIR}/lstring.c
${LUA_SRC_DIR}/ltable.c
${LUA_SRC_DIR}/ltm.c
${LUA_SRC_DIR}/lundump.c
${LUA_SRC_DIR}/lvm.c
${LUA_SRC_DIR}/lzio.c
${LUA_SRC_DIR}/lauxlib.c
${LUA_SRC_DIR}/lbaselib.c
${LUA_SRC_DIR}/lbitlib.c
${LUA_SRC_DIR}/lcorolib.c
${LUA_SRC_DIR}/ldblib.c
${LUA_SRC_DIR}/liolib.c
${LUA_SRC_DIR}/lmathlib.c
${LUA_SRC_DIR}/loslib.c
${LUA_SRC_DIR}/lstrlib.c
${LUA_SRC_DIR}/ltablib.c
${LUA_SRC_DIR}/loadlib.c
${LUA_SRC_DIR}/linit.c
"

SRCS=$(find "${SRC_DIR}" -name '*.cpp')


"${CXX}" -x c++ -std=c++11 -stdlib=libc++ \
	-o "${BIN}" \
	-I "${LUA_SRC_DIR}" -I "${SRC_DIR}" \
	${LUA_SRCS} \
	${SRCS} \
	-lboost_filesystem -lboost_serialization -lboost_system

echo "Created bootstrap at ${BIN}"

