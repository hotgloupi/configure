#pragma once

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

namespace configure { namespace lua {

	class State;
	template<typename T> struct Converter;
	template<typename T> struct Signature;
	template<typename T> struct Caller;

}}
