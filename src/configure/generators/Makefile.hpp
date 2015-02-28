#pragma once

#include <configure/Generator.hpp>

#include <vector>
#include <string>

namespace configure { namespace generators {

	class Makefile
		: public Generator
	{
	public:
		bool is_available(Build& build) const override;
		void generate(Build& build,
		              boost::filesystem::path const& root_project_directory) const override;
		std::string name() const override;
		std::vector<std::string>
		build_command(Build& build, std::string const& target) const override;
	protected:
		virtual std::string dump_command(std::vector<std::string> const& cmd) const;
		virtual bool use_relative_path() const;
	};

}}
