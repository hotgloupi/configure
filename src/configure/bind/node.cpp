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

	static int Node_set_property_default(lua_State* state)
	{
		NodePtr& self = lua::Converter<NodePtr>::extract(state, 1);
		Environ& env = self->properties();
		return utils::env_set_default(state, env, 2);
	}

	void bind_node(lua::State& state)
	{
		/// Node represent a vertex in the build graph.
		//
		// @classmod Node
		lua::Type<Node, NodePtr>(state, "Node")
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
		;
	}

}

