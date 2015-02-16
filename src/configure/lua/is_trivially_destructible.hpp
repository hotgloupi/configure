#pragma once

// Workaround the fact that this trait was previously named has_trivial_destructor.
// see http://stackoverflow.com/questions/12702103/writing-code-that-works-when-has-trivial-destructor-is-defined-instead-of-is

#include <type_traits>

#if defined(__GNUC__)
namespace std {
	template<typename> struct has_trivial_destructor;
	template<typename> struct is_trivially_destructible;
}

namespace configure { namespace lua {
	template<typename T>
	struct has_is_trivially_destructible_helper
	{
		template<
			typename T2
			, bool = std::is_trivially_destructible<T2>::type::value
		>
		static std::true_type test(int);

		template<
			typename T2
			, bool = std::has_trivial_destructor<T2>::type::value
		>
		static std::false_type test(...);

		typedef decltype(test<T>(0)) type;
	};

	template<typename T>
	struct has_is_trivially_destructible
		: has_is_trivially_destructible_helper<T>::type
	{};

	template<typename T>
	using is_trivially_destructible = typename std::conditional<
			  has_is_trivially_destructible<T>::value
			, std::is_trivially_destructible<T>
			, std::has_trivial_destructor<T>
		>::type;

}}
#else

namespace configure { namespace lua {

	template<typename T>
	using is_trivially_destructible =
		typename std::is_trivially_destructible<T>::type;

}}

#endif
