#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>

namespace configure {

	// Boost.Graph used to store dependencies
	typedef boost::adjacency_list<
		  boost::vecS
		, boost::vecS
		, boost::bidirectionalS
		, boost::property<
			  boost::vertex_color_t
			, boost::default_color_type
		>
	> Graph;

	// Graph traits
	typedef boost::graph_traits<Graph> GraphTraits;

	// Vertex type
	typedef GraphTraits::vertex_descriptor Vertex;

	// Edge type
	typedef GraphTraits::edge_descriptor Edge;

}
