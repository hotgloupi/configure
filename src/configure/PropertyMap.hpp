#pragma once

#include "Environ.hpp"

#include <boost/serialization/base_object.hpp>

#include <memory>
#include <vector>

namespace configure {

	class PropertyMap
		: public Environ
	{
	private:
		std::vector<std::string> _dirty_keys;

	public:
		PropertyMap();
		~PropertyMap();

		bool dirty() const;
		bool dirty(std::string key) const;
		void mark_clean();

		template<typename Archive>
		void serialize(Archive& ar, unsigned int const)
		{ ar & boost::serialization::base_object<Environ>(*this); }

	protected:
		void value_changed(std::string const& key) override;
		void new_key(std::string const& key) override;

	private:
		// Do not normalize the key.
		bool _dirty(std::string const& key) const;
	};

}
