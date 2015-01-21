#pragma once

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
	};

}
