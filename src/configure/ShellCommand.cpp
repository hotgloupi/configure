#include "ShellCommand.hpp"
#include "Node.hpp"
#include <configure/utils/path.hpp>
#include <configure/Build.hpp>

#include <boost/variant/get.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace configure {

	ShellFormatter::ShellFormatter(Build const& b)
		: _build(b)
	{}

	ShellFormatter::~ShellFormatter()
	{}

	std::string ShellFormatter::
	operator()(ShellCommand const&, std::string value) const
	{ return std::move(value); }

	std::string ShellFormatter::
	operator()(ShellCommand const& cmd,
	           boost::filesystem::path const& value) const
	{
		if (cmd.has_working_directory() &&
			utils::starts_with(value, _build.directory()))
			return utils::relative_path(
				value, cmd.working_directory()).string();
		return value.string();
	}

	std::string ShellFormatter::
	operator()(ShellCommand const& command, Node const& value) const
	{ return (*this)(command, value.path()); }

	ShellArg::~ShellArg()
	{}

	std::string ShellArg::dump() const
	{ return "<ShellArg>"; }

	ShellCommand::~ShellCommand()
	{}

	namespace {

		struct ShellCommandVisitor
			: boost::static_visitor<>
		{
			Build const& _build;
			DependencyLink const& _link;
			ShellFormatter const& _formatter;
			ShellCommand const& _command;
			std::vector<std::string> _res;

			ShellCommandVisitor(Build const& build,
			                    DependencyLink const& link,
			                    ShellFormatter const& formatter,
			                    ShellCommand const& command)
				: _build(build)
				, _link(link)
				, _formatter(formatter)
				, _command(command)
			{}
			void operator ()(std::string const& value)
			{ _res.push_back(_formatter(_command, value)); }
			void operator ()(boost::filesystem::path const& value)
			{ _res.push_back(_formatter(_command, value)); }
			void operator ()(ShellArgPtr const& value)
			{
				for (auto& el: value->string(_build, _link, _formatter))
					_res.push_back(el);
			}
			void operator ()(NodePtr const& value)
			{ _res.push_back(_formatter(_command, *value)); }
		};

	}

	std::vector<std::string>
	ShellCommand::string(Build const& build,
	                     DependencyLink const& link,
	                     ShellFormatter const& formatter) const
	{
		ShellCommandVisitor visitor(build, link, formatter, *this);
		for (auto const& arg: _args)
			boost::apply_visitor(visitor, arg);
		return std::move(visitor._res);
	}

	std::vector<std::string>
	ShellCommand::string(Build const& build,
	                     DependencyLink const& link) const
	{ return this->string(build, link, ShellFormatter(build)); }

	namespace {

		struct ShellCommandDumpVisitor
			: boost::static_visitor<>
		{
			std::vector<std::string> _res;

			void operator ()(std::string const& value)
			{ _res.push_back(value); }
			void operator ()(boost::filesystem::path const& value)
			{ _res.push_back(value.string()); }
			void operator ()(ShellArgPtr const& value)
			{
				_res.push_back(value->dump());
			}
			void operator ()(NodePtr const& value)
			{ _res.push_back(value->string()); }
		};

	}

	std::vector<std::string>
	ShellCommand::dump() const
	{
		ShellCommandDumpVisitor visitor;
		for (auto const& arg: _args)
			boost::apply_visitor(visitor, arg);
		return std::move(visitor._res);
	}

}
