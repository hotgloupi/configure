#pragma once

#include "fwd.hpp"

#include "Node.hpp"
#include "DependencyLink.hpp"

#include <memory>

namespace configure {

	// Store nodes and links.
	class BuildGraph
	{
	public:
		typedef std::map<boost::filesystem::path, PropertyMap> FileProperties;

	private:
		struct Impl;
		std::unique_ptr<Impl> _this;

	public:
		BuildGraph(FileProperties& properties);
		~BuildGraph();

	public:

		// Create a new node.
		template<typename NodeType, typename... Args>
		NodePtr add_node(Args&&... args)
		{
			Node::index_type index = _add_vertex();
			NodePtr node(
				new NodeType(*this, index, std::forward<Args>(args)...)
			);
			_save(node);
			return std::move(node);
		}

		// A (possibly new) link from source to target nodes.
		DependencyLink& link(Node const& source, Node const& target);
		bool has_link(Node const& source, Node const& target) const;

		// The underlying boost graph.
		Graph const& graph() const;

		// Node by index.
		NodePtr const& node(Node::index_type idx) const;

		// Link by index.
		DependencyLink const& link(DependencyLink::index_type idx) const;

		// Node properties
		PropertyMap& properties(Node const& node) const;

	private:
		Node::index_type _add_vertex();
		DependencyLink::index_type _add_edge(Node const& source, Node const& target);
		void _save(NodePtr& node);
	};

}
