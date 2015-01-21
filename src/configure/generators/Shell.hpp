#pragma once

#include <configure/Generator.hpp>

namespace configure { namespace generators {

	class Shell
		: public Generator
	{
	public:
		bool is_available(Build& build) const override;
		void generate(Build& build) override;
		std::string name() const override;
	};

}}
