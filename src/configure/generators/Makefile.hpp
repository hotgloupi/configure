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
		void generate(Build& build) const override;
		std::string name() const override;
		std::vector<std::string>
		build_command(Build& build, std::string const& target) const override;
	protected:
		virtual void dump_command(
		    std::ostream& out,
		    std::vector<std::string> const& cmd) const;
		virtual std::string node_path(Build& build, Node& node) const;
	};

}}
