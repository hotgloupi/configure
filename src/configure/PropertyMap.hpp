#pragma once

#include "Environ.hpp"

#include <memory>
#include <vector>

namespace configure
{
	class PropertyMap : public Environ
	{
	private:
		std::vector<std::string> _dirty_keys;
		std::vector<std::pair<std::string, Environ::Value>> _deferred;

	public:
		PropertyMap();
		~PropertyMap();

		bool dirty() const;
		bool dirty(std::string key) const;
		void mark_clean();

		void deferred_set(std::string key, Environ::Value value)
		{
			_deferred.push_back(
			  std::make_pair(std::move(key), std::move(value)));
		}

		template <typename Archive>
		void serialize(Archive& ar, unsigned int const);

	protected:
		void value_changed(std::string const& key) override;
		void new_key(std::string const& key) override;

	private:
		// Do not normalize the key.
		bool _dirty(std::string const& key) const;
	};
}
