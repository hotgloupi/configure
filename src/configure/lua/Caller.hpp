#pragma once

#include "fwd.hpp"
#include "Converter.hpp"

namespace configure { namespace lua {

	// Call a function of the given signature by extracting each argument from
	// the stack and push the result back to it.
	template<typename Ret, typename... Args>
	struct Caller<Ret(Args...)>
	{
		// Entry point.
		template<typename Fn>
		static int call(lua_State* state, Fn&& fn)
		{
			if (sizeof...(Args) != lua_gettop(state))
				// XXX Should use <= ?
				throw std::runtime_error("Invalid number of argument");
			return _call<sizeof...(Args)>(state, std::forward<Fn>(fn));
		}

		// Normal case, extract one argument and call itself with it.
		template<unsigned size, typename Fn, typename... ExtractedArgs>
		static typename std::enable_if<size != 0, int>::type
		_call(lua_State* state, Fn&& fn, ExtractedArgs&&... args)
		{
			return _call<size - 1>(
				state,
				std::forward<Fn>(fn),
				_extract<size - 1>(state),
				std::forward<ExtractedArgs>(args)...
			);
		}

		typedef std::tuple<Args...> args_tuple;

		// Helper to extract a clean argument type from arg_tuple.
		template<unsigned index> struct arg_at {
			typedef
				typename Converter<
					typename std::tuple_element<index, args_tuple>::type
				>::extract_type
				type;
		};

		// Extract the argument at index.
		template<unsigned index>
		static typename arg_at<index>::type
		_extract(lua_State* state)
		{
			// index: 0 -> len(args) - 1
			// stack_index: -len(args) -> -1
			const int stack_index = ((int) index) - ((int) sizeof...(Args));
			typedef typename arg_at<index>::type arg_type;
			return Converter<arg_type>::extract(state, stack_index);
		}

		// Final case with no return type (void).
		template<unsigned size, typename Fn, typename... ExtractedArgs>
		static typename std::enable_if<size == 0 && std::is_same<Ret, void>::value, int>::type
		_call(lua_State*, Fn&& fn, ExtractedArgs&&... args)
		{
			fn(std::forward<ExtractedArgs>(args)...);
			return 0;
		}

		// Final case with a return type (Ret != void)
		template<unsigned size, typename Fn, typename... ExtractedArgs>
		static typename std::enable_if<size == 0 && !std::is_same<Ret, void>::value, int>::type
		_call(lua_State* state, Fn&& fn, ExtractedArgs&&... args)
		{
			Ret ret = fn(std::forward<ExtractedArgs>(args)...);
			Converter<Ret>::push(state, std::move(ret));
			return 1;
		}
	};

}}

