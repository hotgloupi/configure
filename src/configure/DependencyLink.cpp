#include "DependencyLink.hpp"
#include "error.hpp"
#include "Command.hpp"

namespace configure {

	DependencyLink::DependencyLink(BuildGraph& graph, index_type index)
		: graph(graph)
		, index(index)
	{}

	DependencyLink::~DependencyLink() {}

	Command const& DependencyLink::command() const
	{
		if (has_command())
			return *_command;
		throw std::runtime_error("This link does not have command");
	}

	void DependencyLink::command(CommandPtr cmd)
	{
		if (has_command())
			CONFIGURE_THROW(error::CommandAlreadySet());
		_command = cmd;
	}

}

