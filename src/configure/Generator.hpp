#pragma once

#include "fwd.hpp"

#include <string>

namespace configure {

	class Generator
	{
	public:
		Generator();
		virtual ~Generator();

	public:
		virtual bool is_available(Build& build) const = 0;
		virtual void generate(Build& build) = 0;
		virtual std::string name() const = 0;
	};

}

