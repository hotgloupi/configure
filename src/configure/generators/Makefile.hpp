#pragma once

#include <configure/Generator.hpp>

#include <iosfwd>
#include <vector>
#include <string>

namespace configure { namespace generators {

	class Makefile
		: public Generator
	{
	public:
		bool is_available(Build& build) const override;
		void generate(Build& build) override;
		std::string name() const override;
	protected:
		virtual void dump_command(
		    std::ostream& out,
		    std::vector<std::string> const& cmd);
	};

}}
