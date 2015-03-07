#include "Environ.hpp"

#include "error.hpp"
#include "log.hpp"
#include "utils/path.hpp"
#include "Node.hpp"

#include <boost/serialization/vector.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <map>
#include <fstream>

namespace fs = boost::filesystem;

namespace std {

	// This overload has to be in std namespace
	static ostream& operator <<(ostream& out,
	                            vector<configure::Environ::Value> const& values)
	{
		out << "{";
		for (auto const& el: values)
			out << el << ", ";
		out << "}";
		return out;
	}

}

namespace configure {

	Environ::Environ()
	{}

	Environ::Environ(Environ const& other)
		: _values(other._values)
	{}

	Environ::~Environ()
	{}

	bool Environ::has(std::string const& key) const
	{ return _values.find(key) != _values.end(); }

	Environ::Kind Environ::kind(std::string const& key) const
	{
		auto it = _values.find(key);
		if (it == _values.end())
			return Kind::none;
		switch (it->second.which())
		{
			case 0: return Kind::string;
			case 1: return Kind::path;
			case 2: return Kind::boolean;
			case 3: return Kind::integer;
			case 4: return Kind::list;
			default: std::abort();
		}
	}

	std::string Environ::as_string(std::string const& key) const
	{
		std::stringstream ss;
		ss << _values.at(key);
		return ss.str();
	}

	void Environ::value_changed(std::string const&) { /* Nothing to do */ }

	void Environ::new_key(std::string const&) { /* Nothing to do */ }

	void Environ::load(boost::filesystem::path const& path)
	{
		std::ifstream in(path.string(), std::ios::binary);
		boost::archive::binary_iarchive ar(in);
		ar & *this;
	}

	void Environ::save(boost::filesystem::path const& path) const
	{
		std::ofstream out(path.string(), std::ios::binary);
		boost::archive::binary_oarchive ar(out);
		ar & *this;
	}

	template<class Archive>
	void Environ::save(Archive& ar, unsigned int) const
	{
		Values::size_type s = _values.size();
		ar << s;
		for (auto& el: _values)
			ar << el;
	}

	template<class Archive>
	void Environ::load(Archive& ar, unsigned int)
	{
		Values::size_type size;
		ar >> size;
		_values.reserve(size);
		for (Values::size_type i = 0; i < size; ++i)
		{
			Values::value_type el;
			ar >> el;
			_values.insert(std::move(el));
		}
	}

	template<typename Archive>
	void Environ::serialize(Archive& ar, unsigned int version)
	{
		boost::serialization::split_member(ar, *this, version);
	}

#define INSTANCIATE(T) \
	template \
	void Environ::serialize<T>(T&, unsigned int const); \

	INSTANCIATE(boost::archive::binary_iarchive);
	INSTANCIATE(boost::archive::binary_oarchive);
#undef INSTANCIATE

	std::vector<std::string> Environ::keys() const
	{
		std::vector<std::string> res;
		res.reserve(_values.size());
		for (auto& p: _values)
			res.push_back(Environ::normalize(p.first));
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
			case Environ::Kind::list: return out << "Kind::list";
			case Environ::Kind::none: return out << "Kind::none";
		}
		std::abort();
	}


	boost::filesystem::path const& Environ::_node_path(Node const& node)
	{
		if (node.is_virtual())
			CONFIGURE_THROW(
				error::InvalidNode("Cannot store a virtual node in the environ")
			);
		return node.path();
	}

}
