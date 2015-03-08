#include "PropertyMap.hpp"

#include "Environ.hpp"

#include <algorithm>

namespace configure {

	PropertyMap::PropertyMap()
	{}

	PropertyMap::~PropertyMap()
	{}

	void PropertyMap::value_changed(std::string const& key)
	{
		std::string normalized = Environ::normalize(key);
		if (!this->_dirty(normalized))
			_dirty_keys.push_back(std::move(normalized));
	}

	void PropertyMap::new_key(std::string const& key)
	{
		_dirty_keys.push_back(Environ::normalize(key));
	}

	bool PropertyMap::dirty() const
	{ return _dirty_keys.size() > 0; }

	bool PropertyMap::dirty(std::string key) const
	{ return _dirty(Environ::normalize(std::move(key))); }

	bool PropertyMap::_dirty(std::string const& key) const
	{
		return std::find(
			_dirty_keys.begin(),
			_dirty_keys.end(),
			key
		) != _dirty_keys.end();
	}

	void PropertyMap::mark_clean()
	{ _dirty_keys.clear(); }

}
