#include "ShellCommand.hpp"
#include "Node.hpp"

#include <boost/variant/get.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace configure {

	ShellArg::~ShellArg()
	{}

	ShellCommand::~ShellCommand()
	{}

	std::vector<std::string>
	ShellCommand::string(Build const& build,
	                     std::vector<NodePtr> const& sources,
	                     std::vector<NodePtr> const& targets) const
	{
		struct Visitor
			: boost::static_visitor<>
		{
			Build const& _build;
			std::vector<NodePtr> const& _sources;
			std::vector<NodePtr> const& _targets;
			std::vector<std::string> _res;

			Visitor(Build const& build,
			        std::vector<NodePtr> const& sources,
			        std::vector<NodePtr> const& targets)
				: _build(build)
				, _sources(sources)
				, _targets(targets)
			{}
			void operator ()(std::string const& value)
			{ _res.push_back(value); }
			void operator ()(boost::filesystem::path const& value)
			{ _res.push_back(value.string()); }
			void operator ()(ShellArgPtr const& value)
			{
				_res.push_back(
					value->string(_build, _sources, _targets)
				);
			}
			void operator ()(NodePtr const& value)
			{ _res.push_back(value->path().string()); }
		} visitor(build, sources, targets);
		for (auto const& arg: _args)
			boost::apply_visitor(visitor, arg);
		return std::move(visitor._res);
	}

	std::vector<std::string>
	ShellCommand::dump() const
	{
		struct Visitor
			: boost::static_visitor<>
		{
			std::vector<std::string> _res;

			void operator ()(std::string const& value)
			{ _res.push_back(value); }
			void operator ()(boost::filesystem::path const& value)
			{ _res.push_back(value.string()); }
			void operator ()(ShellArgPtr const& value)
			{
				_res.push_back(
					"<ShellArg>" // XXX do better
				);
			}
			void operator ()(NodePtr const& value)
			{ _res.push_back(value->path().string()); }
		} visitor;
		for (auto const& arg: _args)
			boost::apply_visitor(visitor, arg);
		return std::move(visitor._res);
	}
}
