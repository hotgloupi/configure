#include "environ_utils.hpp"

#include <configure/bind.hpp>
#include <configure/Node.hpp>
#include <configure/PropertyMap.hpp>
#include <configure/lua/State.hpp>
#include <configure/lua/Type.hpp>

#include <boost/filesystem/path.hpp>

namespace fs = boost::filesystem;

namespace configure {

	static int Node_property(lua_State* state)
	{
		NodePtr& self = lua::Converter<NodePtr>::extract(state, 1);
		Environ& env = self->properties();
		return utils::env_get(state, env, 2);
	}

	static int Node_set_property(lua_State* state)
	{
		NodePtr& self = lua::Converter<NodePtr>::extract(state, 1);
		Environ& env = self->properties();
		return utils::env_set(state, env, 2);
	}

		//_error_handler_ref = luaL_ref(_state, LUA_REGISTRYINDEX); // add ref to avoid gc
		//lua_rawgeti(_state, LUA_REGISTRYINDEX, _error_handler_ref); // push it again on top
	static int Node_set_cached_property(lua_State* state)
	{
		NodePtr& self = lua::Converter<NodePtr>::extract(state, 1);
		auto key = lua::Converter<std::string>::extract(state, 2);
		if (!lua_isfunction(state, -1))
			throw std::runtime_error("Expected a function as a second argument");
		auto res = self->set_cached_property(
			std::move(key),
			[=]() -> Environ::Value {
				lua::State::check_status(state, lua_pcall(state, 0, 1, 0));
				return lua::Converter<Environ::Value>::extract(state, -1);
			});
		lua::Converter<Environ::Value>::push(state, std::move(res));
		return 1;
	}

	static int Node_set_property_default(lua_State* state)
	{
		NodePtr& self = lua::Converter<NodePtr>::extract(state, 1);
		Environ& env = self->properties();
		return utils::env_set_default(state, env, 2);
	}

	static int Node_eq(lua_State* state)
	{
		auto& lhs = lua::Converter<NodePtr>::extract(state, 1);
		auto& rhs = lua::Converter<NodePtr>::extract(state, 2);
		lua_pushboolean(state, (lhs == rhs));
		return 1;
	}

	static int Node_kind(lua_State* state)
	{
		auto& self = lua::Converter<NodePtr>::extract(state, 1);
		lua_pushinteger(state, static_cast<lua_Integer>(self->kind()));
		return 1;
	}
	void bind_node(lua::State& state)
	{
		/// Node represent a vertex in the build graph.
		//
		// @classmod Node
		lua::Type<Node, NodePtr> type(state, "Node");
		type
			/// True if the node is a virtual node.
			// @function Node:is_virtual
			// @treturn bool
			.def("is_virtual", &Node::is_virtual)

			/// Path of the node
			// @function Node:path
			// @treturn Path absolute path
			.def("path", &Node::path)

			/// Name of a virtual node.
			// @function Node:name
			// @treturn string
			.def("name", &Node::name)

			.def("__tostring", &Node::string)

			.def("__eq", &Node_eq)

			/// Relative path to the node
			// @tparam Path start Starting point of the relative path
			// @treturn Path the relative path
			// @function Node:relative_path
			.def("relative_path", &Node::relative_path)

			/// Retrieve a Node property
			// @string name The property name
			// @treturn string|Path|boolean|nil
			// @function Node:property
			.def("property", &Node_property)

			/// Set a Node property
			// @string name The property name
			// @tparam string|Path|boolean|nil the value to set
			// @treturn string|Path|boolean|nil
			// @function Node:set_property
			.def("set_property", &Node_set_property)

			/// Declare a cached property
			// @string name The property name
			// @tparam string|Path|boolean|nil the value to set
			// @treturn string|Path|boolean|nil
			// @function Node:set_property
			.def("set_cached_property", &Node_set_cached_property)

			/// Set a Node property default value
			// @string name The property name
			// @tparam string|Path|boolean|nil the default value to set
			// @treturn string|Path|boolean|nil
			// @function Node:set_property_default
			.def("set_property_default", &Node_set_property_default)

			/// Check if the node is a directory node
			// @treturn bool
			// @function Node:is_directory
			.def("is_directory", &Node::is_directory)

			/// Check if the node is a file node
			// @treturn bool
			// @function Node:is_file
			.def("is_file", &Node::is_file)

			/// Check if the node is a virtual node
			// @treturn bool
			// @function Node:is_virtual
			.def("is_virtual", &Node::is_virtual)

			/// Kind of a node
			// @treturn Kind
			// @function Node::kind
			.def("kind", &Node_kind)
		;
#define ENUM_VALUE(key)                                                \
	lua_pushinteger(state.ptr(), static_cast<lua_Integer>(Node::key)); \
	lua_setfield(state.ptr(), -2, #key);                               \
		// Kind
		lua_newtable(state.ptr());
		ENUM_VALUE(file_node);
		ENUM_VALUE(directory_node);
		ENUM_VALUE(virtual_node);
		lua_setfield(state.ptr(), -2, "Kind");
#undef ENUM_VALUE
	}

}

