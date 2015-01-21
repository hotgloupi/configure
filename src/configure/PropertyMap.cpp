#include "PropertyMap.hpp"

#include "Environ.hpp"

namespace configure {

	struct PropertyMap::Impl
	{
		Environ map;
	};

	PropertyMap::PropertyMap()
		: _this{new Impl}
	{}

	PropertyMap::~PropertyMap()
	{}

}
