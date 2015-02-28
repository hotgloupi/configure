#!/bin/sh

# Usage: bash bootstrap/vs.sh

set -x
set -e

SCRIPT="$0"
SCRIPT_DIR="$(cd "$(dirname ${SCRIPT})" && pwd)"
LUA_SRC_DIR="${SCRIPT_DIR}/../lua/src"
SRC_DIR="${SCRIPT_DIR}/../src"
CXX="${CXX:-cl}"
BIN="configure.exe"
BOOST_ROOT=${BOOST_ROOT:-c:/Libraries/boost}
BOOST_LIBRARY_DIR=${BOOST_LIBRARY_DIR:-${BOOST_ROOT}/stage/lib}

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

SRCS="
${SRC_DIR}/main.cpp
${SRC_DIR}/configure/*.cpp
${SRC_DIR}/configure/bind/*.cpp
${SRC_DIR}/configure/generators/*.cpp
${SRC_DIR}/configure/lua/*.cpp
${SRC_DIR}/configure/utils/*.cpp
"

ls "$BOOST_LIBRARY_DIR"
ls "${BOOST_ROOT}"

mkdir temp

var=0
for src in ${SRCS} ${LUA_SRCS}
do
	var=$((var+1))
	"${CXX}" -nologo \
		-c -Fo"temp/$var.obj" \
		-EHsc -MD \
		-I "${LUA_SRC_DIR}" -I "${SRC_DIR}" -I "${BOOST_ROOT}" \
		-DBOOST_LIB_DIAGNOSTIC \
		-Tp "$src"
done

link -nologo \
	-OUT:"${BIN}" \
	-LIBPATH:"${BOOST_LIBRARY_DIR}" \
	-subsystem:console \
	-machine:I386 \
	temp/*.obj \
	Shlwapi.lib

rm -rf temp

echo "Created bootstrap at ${BIN}"
