#include <configure/bind.hpp>

#include <configure/Rule.hpp>
#include <configure/lua/State.hpp>
#include <configure/lua/Type.hpp>

#include <boost/filesystem/path.hpp>

namespace fs = boost::filesystem;

namespace configure {

	static int Rule_new(lua_State* state)
	{
		lua::Converter<Rule>::push(state);
		return 1;
	}
	static int Rule_tostring(lua_State* state)
	{
		std::string res = "<Rule ";
		Rule& rule = lua::Converter<Rule>::extract(state, 1);
		for (auto&& node: rule.sources())
		{
			lua::Converter<NodePtr>::push(state, node);
			res.push_back(' ');
			res.append(luaL_tolstring(state, -1, nullptr));
			lua_pop(state, 1);
		}
		if (rule.sources().empty())
			res.append(" ()");
		res.append(" ->");

		for (auto&& node: rule.targets())
		{
			lua::Converter<NodePtr>::push(state, node);
			res.push_back(' ');
			res.append(luaL_tolstring(state, -1, nullptr));
			lua_pop(state, 1);
		}
		if (rule.targets().empty())
			res.append(" ()");

		res.push_back(':');
		for (auto&& cmd: rule.shell_commands())
		{
			lua::Converter<ShellCommand>::push(state, cmd);
			res.push_back(' ');
			res.append(luaL_tolstring(state, -1, nullptr));
			lua_pop(state, 1);
		}
		res.push_back('>');
		lua_pushstring(state, res.c_str());
		return 1;
	}

	static int Rule_add_source(lua_State* state)
	{
		Rule& self = lua::Converter<Rule>::extract(state, 1);
		if (NodePtr* n = lua::Converter<NodePtr>::extract_ptr(state, 2))
			self.add_source(*n);
		else
			CONFIGURE_THROW(
				error::LuaError(
					"Expected source node, got '" + std::string(luaL_tolstring(state, 2, nullptr)) + "'"
				)
			);
		lua_pushvalue(state, 1);
		return 1;
	}

	static int Rule_add_sources(lua_State* state)
	{
		Rule& self = lua::Converter<Rule>::extract(state, 1);
		if (!lua_istable(state, 2))
			CONFIGURE_THROW(
				error::LuaError(
					"Expected a table, got '" + std::string(luaL_tolstring(state, 2, nullptr)) + "'"
				)
			);
		for (int i = 1, len = lua_rawlen(state, 2); i <= len; ++i)
		{
			lua_rawgeti(state, 2, i);
			if (NodePtr* n = lua::Converter<NodePtr>::extract_ptr(state, -1))
				self.add_source(*n);
			else
				CONFIGURE_THROW(
					error::LuaError(
						"Expected source node, got '" + std::string(luaL_tolstring(state, -1, nullptr))
						+ "' at index " + std::to_string(i)
					)
				);
			lua_remove(state, -1);
		}
		lua_pushvalue(state, 1);
		return 1;
	}

	static int Rule_add_targets(lua_State* state)
	{
		Rule& self = lua::Converter<Rule>::extract(state, 1);
		if (!lua_istable(state, 2))
			CONFIGURE_THROW(
				error::LuaError(
					"Expected a table, got '" + std::string(luaL_tolstring(state, 2, nullptr)) + "'"
				)
			);
		for (int i = 1, len = lua_rawlen(state, 2); i <= len; ++i)
		{
			lua_rawgeti(state, 2, i);
			if (NodePtr* n = lua::Converter<NodePtr>::extract_ptr(state, -1))
				self.add_target(*n);
			else
				CONFIGURE_THROW(
					error::LuaError(
						"Expected target node, got '" + std::string(luaL_tolstring(state, -1, nullptr))
						+ "' at index " + std::to_string(i)
					)
				);
			lua_remove(state, -1);
		}
		lua_pushvalue(state, 1);
		return 1;
	}

	static int Rule_add_target(lua_State* state)
	{
		Rule& self = lua::Converter<Rule>::extract(state, 1);
		if (NodePtr* n = lua::Converter<NodePtr>::extract_ptr(state, 2))
			self.add_target(*n);
		else
			throw std::runtime_error("Cannot extract a target node");
		lua_pushvalue(state, 1);
		return 1;
	}

	static int Rule_add_shell_command(lua_State* state)
	{
		Rule& self = lua::Converter<Rule>::extract(state, 1);
		if (ShellCommand* n = lua::Converter<ShellCommand>::extract_ptr(state, 2))
			self.add_shell_command(std::move(*n));
		else
			throw std::runtime_error("Cannot extract a shell command");
		lua_pushvalue(state, 1);
		return 1;
	}

	void bind_rule(lua::State& state)
	{
		/// @classmod Rule
		lua::Type<Rule>(state, "Rule")
			/// Create a new rule
			// @function Rule:new
			// @treturn New instance
			.def("new", &Rule_new)
			.def("__tostring", &Rule_tostring)

			/// Add a source @{Node}
			// @function Rule:add_source
			// @tparam Node source
			// @treturn Rule self instance
			.def("add_source", &Rule_add_source)

			/// Add source @{Node}s
			// @function Rule:add_sources
			// @tparam table sources List of source nodes
			// @treturn Rule self instance
			.def("add_sources", &Rule_add_sources)

			/// Add a target @{Node}
			// @function Rule:add_target
			// @tparam Node target
			// @treturn Rule self instance
			.def("add_target", &Rule_add_target)

			/// Add target @{Node}s
			// @function Rule:add_targets
			// @tparam table targets List of source nodes
			// @treturn Rule self instance
			.def("add_targets", &Rule_add_targets)

			/// Add a @{ShellCommand}
			// @function Rule:add_shell_command
			// @tparam ShellCommand cmd the shell command
			// @treturn Rule self instance
			.def("add_shell_command", &Rule_add_shell_command)
		;
	}

}
