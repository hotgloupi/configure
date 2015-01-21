#pragma once

#include "Makefile.hpp"

namespace configure { namespace generators {

	class NMakefile
		: public Makefile
	{
	public:
		bool is_available(Build& build) const override;
		std::string name() const override;
	protected:
		void dump_command(
		    std::ostream& out,
		    std::vector<std::string> const& cmd) override;
	};

}}
