#include <configure/bind.hpp>

#include <configure/Node.hpp>
#include <configure/lua/State.hpp>
#include <configure/lua/Type.hpp>

#include <boost/filesystem/path.hpp>

namespace fs = boost::filesystem;

namespace configure {

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
		;
	}

}

