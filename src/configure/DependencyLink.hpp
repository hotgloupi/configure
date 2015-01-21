#pragma once

#include "fwd.hpp"
#include "Graph.hpp"

namespace configure {

	// Store one or more commands used to generate a file
	class DependencyLink
	{
		friend class BuildGraph;

	public:
		typedef Edge index_type;

	public:
		BuildGraph& graph;
		index_type const index;
	private:
		CommandPtr _command;

	protected:
		DependencyLink(BuildGraph& graph, index_type index);
	public:
		virtual ~DependencyLink();

		bool has_command() const { return _command != nullptr; }
		Command const& command() const;
		void command(CommandPtr cmd);
	};

}

