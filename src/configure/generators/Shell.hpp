#pragma once

#include <configure/Generator.hpp>

namespace configure { namespace generators {

	class Shell
		: public Generator
	{
	public:
		bool is_available(Build& build) const override;
		void generate(Build& build) const override;
		std::string name() const override;
		std::vector<std::string>
		build_command(Build& build, std::string const& target) const override;
	};

}}
