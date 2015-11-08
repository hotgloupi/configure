#pragma once

#include "fwd.hpp"

#include <boost/variant.hpp>
#include <boost/filesystem/path.hpp>

#include <iosfwd>
#include <memory>
#include <type_traits>
#include <vector>
#include <unordered_map>
#include <boost/functional/hash.hpp>

namespace configure {

	// Encapsulate a map where keys are strings and values are strings, path,
	// bool, or integers.
	//
	// The case of keys is ignored and characters '-' and '_' are considered
	// the same. Only ascii characters are accepted.
	class Environ
	{
	public:

	public:
		template<typename T> struct const_ref { typedef T const& type; };
		template<typename T> struct extract;
		enum class Kind {
			string,
			path,
			boolean,
			integer,
			list,
			none,
		};
		typedef boost::make_recursive_variant<
			std::string,
			boost::filesystem::path,
			bool,
			int64_t,
			std::vector<boost::recursive_variant_>
		>::type Value;

	private:
		template<typename Char>
		static inline Char normalize_char(Char c)
		{
			static std::locale const locale;
			if (c == '-') return '_';
			return std::toupper(c, locale);
		}

		struct Hash
		{
			template<typename String>
			size_t operator ()(String const& value) const
			{
				size_t seed = 0;
				for (auto c: value)
					boost::hash_combine(seed, Environ::normalize_char(c));
				return seed;
			}
		};
		struct Equals
		{
			// My guess is that equals is called only when hashes matches
			// so the strings are likely to be equal.
			template<typename String>
			bool operator() (String const& lhs, String const& rhs) const
			{
				auto p1 = lhs.c_str();
				auto p2 = rhs.c_str();
				while (*p1 != '\0')
				{
					if (Environ::normalize_char(*p1) != Environ::normalize_char(*p2))
						return false;
					p1 += 1;
					p2 += 1;
				}
				return *p2 == '\0';
			}
		};
		typedef std::unordered_map<std::string, Value, Hash, Equals> Values;
		Values _values;

	public:
		Environ();
		Environ(Environ const& other);
		virtual ~Environ();

		bool has(std::string const& key) const;
		Kind kind(std::string const& key) const;
		std::string as_string(std::string const& key) const;

		// Returns an existing value of type T or throws if the key is not
		// present or if the value cannot be casted to the asked return value.
		//
		template<typename Ret = Value>
		typename const_ref<Ret>::type get(std::string const& key) const
		{ return extract<Ret>::apply(_values.at(key)); }

		// Assign key to value.
		template<typename Ret = Value, typename T>
		typename const_ref<Ret>::type set(std::string const& key, T&& value)
		{
			auto it = _values.find(key);
			if (it == _values.end())
				return _set_new<Ret>(key, std::forward<T>(value));
			Value new_value = _convert<Ret>(std::forward<T>(value));
			if (!(it->second == new_value))
			{
				it->second = std::move(new_value);
				this->value_changed(it->first); // XXX not exception safe
			}
			return extract<Ret>::apply(it->second);
		}

		// Assign key to value not already set, returns associated value.
		template<typename Ret = Value, typename T>
		typename const_ref<Ret>::type set_default(std::string const& key,
		                                        T&& value)
		{
			auto it = _values.find(key);
			if (it == _values.end())
				return _set_new<Ret>(key, std::forward<T>(value));
			return extract<Ret>::apply(it->second);
		}

		void load(boost::filesystem::path const& path);
		void save(boost::filesystem::path const& path) const;

		void clear() { _values.clear(); }

	public:
		template<typename Archive>
		void serialize(Archive& ar, unsigned int);
		template<class Archive>
		void save(Archive& ar, unsigned int) const;
		template<class Archive>
		void load(Archive& ar, unsigned int);

		std::vector<std::string> keys() const;

	protected:
		// Called when the value of an existing key has changed (default implem
		// does nothing).
		virtual void value_changed(std::string const& key);

		// Called when a key has been added (default implem does nothing).
		virtual void new_key(std::string const& key);

	public:
		// Upper case and replace dashes by underscores.
		static std::string normalize(std::string key);

	private:
		// Force insert.
		template<typename Ret, typename T>
		typename const_ref<Ret>::type _set_new(std::string const& key,
		                                       T&& value)
		{
			Value& ret = (_values[key] = _convert<Ret>(std::forward<T>(value)));
			this->new_key(key); // XXX not exception safe
			return extract<Ret>::apply(ret);
		}

		// Convert to a value
		template<typename Ret, typename T>
		Value _convert(T&& value) { return Value(Ret(std::forward<T>(value))); }

		template<typename Ret, typename T>
		Value _convert(std::vector<T> const& value)
		{
			std::vector<Value> res;
			res.reserve(value.size());
			for (auto&& el: value)
				res.emplace_back(el);
			return res;
		}

		template<typename Ret, typename T>
		Value _convert(std::vector<T>&& value)
		{
			std::vector<Value> res;
			res.reserve(value.size());
			for (auto&& el: value)
				res.emplace_back(std::move(el));
			return res;
		}

		template<typename Ret>
		Value _convert(std::vector<Value> const& value)
		{ return Value(value); }

		template<typename Ret>
		Value _convert(std::vector<Value>&& value)
		{ return Value(std::move(value)); }

		template<typename Ret>
		Value _convert(NodePtr const& node)
		{ return Value(Ret(_node_path(*node))); }

		template<typename Ret>
		Value _convert(Node const& node)
		{ return Value(Ret(_node_path(node))); }

		static
		boost::filesystem::path const& _node_path(Node const& node);
	};

	std::ostream& operator <<(std::ostream& out, Environ::Kind kind);

	// const_ref specialization -----------------------------------------------
	template<> struct Environ::const_ref<int>
	{ typedef int type; };

	template<> struct Environ::const_ref<bool>
	{ typedef bool type; };

	template<typename T> struct Environ::const_ref<std::vector<T>>
	{ typedef std::vector<T> type; };

	template<> struct Environ::const_ref<std::vector<Environ::Value>>
	{ typedef std::vector<Environ::Value> const& type; };


	// extract specialization -------------------------------------------------
	template<typename T> struct Environ::extract
	{
		typedef typename Environ::const_ref<T>::type return_type;
		static
		return_type apply(Value const& value)
		{ return boost::get<T>(value); }
	};

	template<> struct Environ::extract<Environ::Value>
	{
		static Environ::Value const& apply(Environ::Value const& value)
		{ return value; }
	};

	template<> struct Environ::extract<std::vector<Environ::Value>>
	{
		typedef std::vector<Environ::Value> const& return_type;
		static return_type apply(Environ::Value const& value)
		{ return boost::get<std::vector<Environ::Value>>(value); }
	};

	template<typename T> struct Environ::extract<std::vector<T>>
	{
		typedef std::vector<T> return_type;
		static
		return_type apply(Value const& value)
		{
			auto const& vector = boost::get<std::vector<Value>>(value);
			return_type res;
			res.reserve(vector.size());
			for (auto& el: vector)
				res.push_back(boost::get<T>(el));
			return res;
		}
	};

}
