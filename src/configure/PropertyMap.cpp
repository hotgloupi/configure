#include "PropertyMap.hpp"

#include "Environ.hpp"

namespace configure {

	struct PropertyMap::Impl
	{
		Environ values;
	};

	PropertyMap::PropertyMap()
		: _this{new Impl}
	{}

	PropertyMap::~PropertyMap()
	{}

	Environ& PropertyMap::values()
	{ return _this->values; }

	Environ const& PropertyMap::values() const
	{ return _this->values; }

}
