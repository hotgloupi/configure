#pragma once

#include <iosfwd>
#include <memory>
#include <type_traits>

#include <boost/filesystem/path.hpp>

namespace configure {

	// Encapsulate a map where keys are strings and values are strings, path,
	// bool, or integers.
	//
	// The case of keys is ignored and characters '-' and '_' are considered
	// the same. Only ascii characters are accepted.
	class Environ
	{
	private:
		struct Impl;
		std::unique_ptr<Impl> _this;

	public:
		template<typename T> struct const_ref { typedef T const& type; };
		enum class Kind {
			string,
			path,
			boolean,
			integer,
			none,
		};

	public:
		Environ();
		~Environ();

		bool has(std::string key) const;
		Kind kind(std::string key) const;
		std::string as_string(std::string key) const;

		// Returns an existing value of type T or throws.
		template<typename T>
		typename const_ref<T>::type get(std::string key) const;

		// Returns an existing value of type T or the default value.
		template<typename T>
		typename const_ref<T>::type get(std::string key,
		                                typename const_ref<T>::type default_value) const;

		// Assign key to value.
		template<typename T>
		typename const_ref<T>::type set(std::string key, T value);

		// Assign key to value not already set, returns associated value.
		template<typename T>
		typename const_ref<T>::type set_default(std::string key,
		                                        typename const_ref<T>::type default_value);

		void load(boost::filesystem::path const& path);
		void save(boost::filesystem::path const& path) const;

		std::vector<std::string> keys() const;
	public:
		// Upper case and replace dashes by underscores.
		static std::string normalize(std::string key);
	};

	template<> struct Environ::const_ref<int> { typedef int type; };
	template<> struct Environ::const_ref<bool> { typedef bool type; };

	std::ostream& operator <<(std::ostream& out, Environ::Kind kind);

}
