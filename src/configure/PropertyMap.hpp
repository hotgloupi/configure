#pragma once

#include "fwd.hpp"

#include <memory>

namespace configure {

	class PropertyMap
	{
	private:
		struct Impl;
		std::unique_ptr<Impl> _this;

	public:
		PropertyMap();
		~PropertyMap();

	public:
		Environ const& values() const;
		Environ& values();
	};

}
