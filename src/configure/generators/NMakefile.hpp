#pragma once

#include "Makefile.hpp"

namespace configure { namespace generators {

	class NMakefile
		: public Makefile
	{
	public:
		bool is_available(Build& build) const override;
		std::string name() const override;
		std::vector<std::string>
		build_command(Build& build, std::string const& target) const override;
	protected:
		std::string dump_command(std::vector<std::string> const& cmd) const override;
		bool use_relative_path() const override;
	};

}}
