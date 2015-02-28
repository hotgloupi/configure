#pragma once

#include <configure/Generator.hpp>

namespace configure { namespace generators {

	class Shell
		: public Generator
	{
	public:
		bool is_available(Build& build) const override;
		void generate(Build& build,
		              boost::filesystem::path const& root_project_directory) const override;
		std::string name() const override;
		std::vector<std::string>
		build_command(Build& build, std::string const& target) const override;
	};

}}
