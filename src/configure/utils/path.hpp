#pragma once

#include <boost/filesystem/path.hpp>
#include <boost/serialization/split_free.hpp>

namespace configure { namespace utils {

	boost::filesystem::path
	relative_path(boost::filesystem::path const& full,
	              boost::filesystem::path const& start);

	bool starts_with(boost::filesystem::path const& path,
	                 boost::filesystem::path const& prefix);

}}

namespace boost { namespace serialization {

	template<typename Archive>
	void save(Archive& ar, boost::filesystem::path const& path, unsigned int const)
	{
		ar & path.string();
	}

	template<typename Archive>
	void load(Archive& ar, boost::filesystem::path& path, unsigned int const)
	{
		std::string s;
		ar & s;
		path = s;
	}

}}

BOOST_SERIALIZATION_SPLIT_FREE(boost::filesystem::path);

namespace std {

	template<> struct hash<boost::filesystem::path>
	{
		size_t operator ()(boost::filesystem::path const& p) const
		{ return hash_value(p); }
	};

}
