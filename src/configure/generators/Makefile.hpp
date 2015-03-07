#pragma once

#include <configure/Generator.hpp>

#include <vector>
#include <string>

namespace configure { namespace generators {

	class Makefile
		: public Generator
	{
	protected:
		std::vector<NodePtr> _includes;
		std::vector<NodePtr> _sources;
		std::vector<NodePtr> _targets;
		std::vector<NodePtr> _final_targets;
		std::vector<NodePtr> _virtual_nodes;

	public:
		Makefile(Build& build,
		         path_t project_directory,
		         path_t configure_exe,
		         char const* name = nullptr);

		std::vector<std::string>
		build_command(std::string const& target) const override;

	public:
		static char const* name() { return "Makefile"; }
		static bool is_available(Build& build);

	protected:
		virtual std::string dump_command(std::vector<std::string> const& cmd) const;
		virtual bool use_relative_path() const;
		virtual void include_dependencies(std::ostream& out, bool relative) const;
		void prepare();
		void generate() const override;
	};

}}
