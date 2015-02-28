#include "BuildGraph.hpp"
#include "Graph.hpp"
#include "PropertyMap.hpp"
#include "error.hpp"

#include <boost/assert.hpp>

#include <unordered_map>
#include <map>

namespace configure {

	struct BuildGraph::Impl
	{
	public:
		typedef boost::property_map<Graph, boost::vertex_index_t>::type IndexMap;
		typedef std::unordered_map<Node::index_type, NodePtr> NodeMap;
		typedef std::map<DependencyLink::index_type, DependencyLink> LinkMap;

	public:
		Graph    graph;
		IndexMap index_map;
		NodeMap  node_map;
		LinkMap link_map;
		FileProperties& properties;

	public:
		Impl(FileProperties& properties)
			: graph()
			, index_map(boost::get(boost::vertex_index, graph))
			, node_map()
			, link_map()
			, properties(properties)
		{}
	};

	BuildGraph::BuildGraph(FileProperties& properties)
		: _this{new Impl(properties)}
	{}

	BuildGraph::~BuildGraph()
	{}

	Graph const& BuildGraph::graph() const
	{ return _this->graph; }

	DependencyLink& BuildGraph::link(Node const& source, Node const& target)
	{
		auto ret = boost::edge(source.index, target.index, _this->graph);
		if (ret.second)
			return _this->link_map.at(ret.first);
		DependencyLink::index_type index = _add_edge(source, target);
		auto res = _this->link_map.insert(
		    Impl::LinkMap::value_type(index, DependencyLink(*this, index))
		);
		if (!res.second)
		{
			// XXX revert all the things
			throw std::runtime_error("Couldn't create the link");
		}
		return res.first->second;
	}


	NodePtr const& BuildGraph::node(Node::index_type idx) const
	{ return _this->node_map.at(idx); }

	DependencyLink const& BuildGraph::link(DependencyLink::index_type idx) const
	{ return _this->link_map.at(idx); }

	Node::index_type BuildGraph::_add_vertex()
	{ return boost::add_vertex(_this->graph); }

	DependencyLink::index_type BuildGraph::_add_edge(Node const& source, Node const& target)
	{
		auto res = boost::add_edge(
			source.index,
			target.index,
			_this->graph
		);
		if (!res.second)
			throw std::runtime_error("Couldn't create the edge");
		return res.first;
	}

	PropertyMap& BuildGraph::properties(Node const& node) const
	{
		if (!node.is_file())
			CONFIGURE_THROW(
			    error::InvalidNode("Only file node have properties")
					<< error::node(_this->node_map[node.index])
			);
		return _this->properties[node.path()];
	}

	void BuildGraph::_save(NodePtr& node)
	{
		BOOST_ASSERT(_this->node_map.find(node->index) == _this->node_map.end());
		_this->node_map[node->index] = node;
	}



}
