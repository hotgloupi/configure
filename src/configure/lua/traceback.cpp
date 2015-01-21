#include "traceback.hpp"

#include <boost/algorithm/string/join.hpp>
#include <boost/scope_exit.hpp>

namespace configure { namespace lua {

	std::vector<Frame> traceback(lua_State *L)
	{
		// Used to compare with the buitin backtrace
		//int top = lua_gettop(L);
		//BOOST_SCOPE_EXIT_ALL(=){lua_settop(L, top);};
		//lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
		//lua_getfield(L, -1, "debug");
		//lua_getfield(L, -1, "traceback");
		//lua_call(L, 0, LUA_MULTRET);
		//if (char const* str = lua_tostring(L, -1))
		//	std::cout << "--------" << std::endl
		//		<< str << std::endl
		//		<< "---------------" << std::endl;
		int lvl = 0;
		lua_Debug ar;
		std::vector<Frame> res;
		while (lua_getstack(L, lvl, &ar))
		{
			Frame f;
			lvl += 1;
			if (!lua_getinfo(L, "nSltu", &ar))
			{
				f.name = "???";
				res.push_back(f);
				continue;
			}
			if (ar.name != nullptr)
				f.name = ar.name;
			if (ar.namewhat != nullptr)
				f.kind = ar.namewhat;
			if (ar.source != nullptr && ar.source[0] == '@')
				f.source = &ar.source[1];
			if (ar.what != nullptr)
				f.builtin = (ar.what[0] == 'C');
			f.current_line = ar.currentline;
			f.first_function_line = ar.linedefined;

			res.push_back(f);
		}
		return std::move(res);
	}


}}
