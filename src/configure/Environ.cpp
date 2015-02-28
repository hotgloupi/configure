#include "Environ.hpp"

#include "error.hpp"
#include "log.hpp"
#include "utils/path.hpp"

#include <boost/filesystem/path.hpp>
#include <boost/variant.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <map>
#include <fstream>

namespace fs = boost::filesystem;

namespace configure {

	struct Environ::Impl
	{
		typedef boost::variant<
			std::string,
			boost::filesystem::path,
			bool,
			int
		> Value;
		std::map<std::string, Value> values;
	};

	Environ::Environ()
		: _this(new Impl)
	{}

	Environ::~Environ()
	{}

	bool Environ::has(std::string key) const
	{ return _this->values.find(normalize(std::move(key))) != _this->values.end(); }

	Environ::Kind Environ::kind(std::string key) const
	{
		auto it = _this->values.find(normalize(std::move(key)));
		if (it == _this->values.end())
			return Kind::none;
		switch (it->second.which())
		{
			case 0: return Kind::string;
			case 1: return Kind::path;
			case 2: return Kind::boolean;
			case 3: return Kind::integer;
			default: std::abort();
		}
	}

	std::string Environ::as_string(std::string key) const
	{
		std::stringstream ss;
		ss << _this->values.at(normalize(std::move(key)));
		return ss.str();
	}

	template<typename T>
	typename Environ::const_ref<T>::type
	Environ::get(std::string key) const
	{
		return boost::get<typename const_ref<T>::type>(
			_this->values.at(normalize(std::move(key)))
		);
	}

	template<typename T>
	typename Environ::const_ref<T>::type
	Environ::get(std::string key,
	             typename const_ref<T>::type default_value) const
	{
		auto it = _this->values.find(normalize(std::move(key)));
		if (it == _this->values.end())
			return default_value;
		return boost::get<typename const_ref<T>::type>(it->second);
	}

	template<typename T>
	typename Environ::const_ref<T>::type
	Environ::set(std::string key_, T value)
	{
		std::string key = normalize(std::move(key_));
		log::debug("ENV: set", key, '=', value);
		Impl::Value& v = (_this->values[std::move(key)] = std::move(value));
		return boost::get<typename const_ref<T>::type>(v);
	}

	template<typename T>
	typename Environ::const_ref<T>::type
	Environ::set_default(std::string key_,
	                     typename const_ref<T>::type default_value)
	{
		std::string key = normalize(std::move(key_));
		auto it = _this->values.find(key);
		if (it == _this->values.end())
			return this->set<T>(std::move(key), default_value);
		return boost::get<typename const_ref<T>::type>(it->second);
	}

#define INSTANCIATE(T) \
	template \
	Environ::const_ref<T>::type \
	Environ::get<T>(std::string key) const; \
	template \
	Environ::const_ref<T>::type \
	Environ::get<T>(std::string key, \
	                const_ref<T>::type default_value) const; \
	template \
	Environ::const_ref<T>::type \
	Environ::set<T>(std::string key, T value); \
	template \
	Environ::const_ref<T>::type \
	Environ::set_default<T>(std::string key, \
	                        const_ref<T>::type default_value); \
/**/

	INSTANCIATE(std::string);
	INSTANCIATE(fs::path);
	INSTANCIATE(bool);
	INSTANCIATE(int);
#undef INSTANCIATE

	void Environ::load(boost::filesystem::path const& path)
	{
		std::ifstream in(path.string(), std::ios::binary);
		boost::archive::binary_iarchive ar(in);
		ar >> _this->values;
	}

	void Environ::save(boost::filesystem::path const& path) const
	{
		std::ofstream out(path.string(), std::ios::binary);
		boost::archive::binary_oarchive ar(out);
		ar << _this->values;
	}

	std::vector<std::string> Environ::keys() const
	{
		std::vector<std::string> res;
		res.reserve(_this->values.size());
		for (auto& p: _this->values)
			res.push_back(p.first);
		return res;
	}

	std::string Environ::normalize(std::string key)
	{
		if (key.empty())
			CONFIGURE_THROW(error::InvalidKey("Empty key"));
		if (key[0] >= '0' && key[0] <= '9')
			CONFIGURE_THROW(error::InvalidKey("Key '" + key + "' cannot start with a digit"));
		if (key[0] == '-')
			CONFIGURE_THROW(error::InvalidKey("Key '" + key + "' cannot start with a dash"));
		if (key[0] == '_')
			CONFIGURE_THROW(error::InvalidKey("Key '" + key + "' cannot start with a dash"));
		for (size_t i = 0, len = key.size(); i < len; ++i)
		{
			char& c = key[i];
			if (c == '-') c = '_';
			else if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
			else if (!(c >= 'A' && c <= 'Z') && !(c >= '0' && c <= '9') && c != '_')
				CONFIGURE_THROW(error::InvalidKey(
					"Key '" + key + "' contains an invalid character '" +
					std::string(&c, 1) + "' at index " + std::to_string(i)
				));
		}
		if (key[key.size() - 1] == '_') // Cannot ends with '-' since they are converted in the for loop
			CONFIGURE_THROW(error::InvalidKey("Key '" + key + "' cannot end with a dash"));
		return key;
	}

	std::ostream& operator <<(std::ostream& out, Environ::Kind kind)
	{
		switch (kind)
		{
			case Environ::Kind::integer: return out << "Kind::integer";
			case Environ::Kind::boolean: return out << "Kind::boolean";
			case Environ::Kind::path: return out << "Kind::path";
			case Environ::Kind::string: return out << "Kind::string";
			case Environ::Kind::none: return out << "Kind::none";
		}
		std::abort();
	}

}
