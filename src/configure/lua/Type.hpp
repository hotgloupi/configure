#pragma once

#include "State.hpp"
#include "store_exception.hpp"

#include <configure/error.hpp>
#include <boost/units/detail/utility.hpp>

#include <functional>

namespace configure { namespace lua {

	namespace convert {
		template<typename T>
		inline T& to_ref(T& value) { return value; }

		template<typename T>
		inline T& to_ref(std::shared_ptr<T>& value) { return *value.get(); }

		template<typename T>
		inline T& to_ref(std::reference_wrapper<T>& value) { return value.get(); }

	}

	namespace return_policy {

		struct copy {
			template<typename T>
			struct get {
				typedef typename std::remove_cv<
					typename std::remove_reference<T>::type
				>::type type;
			};
		};

		struct ref {
			template<typename T>
			struct get {
				typedef typename std::reference_wrapper<
					typename copy::get<T>::type
				> type;
			};
		};

	}

	template<
		  typename T
		, typename Storage = T
	>
	struct Type
	{
	private:
		State* _state;

	public:
		explicit Type(State& state, std::string const& name = "")
			: _state(&state)
		{
			_state->push_metatable<Storage>(name);
			_state->push("__index");
			_state->pushvalue(-2);
			_state->settable(-3);
		}

		Type(Type const&) = delete;

		Type(Type&& other)
			: _state(other._state)
		{ other._state = nullptr; }

		~Type() { if (_state != nullptr) _state->pop(); }

	public:
		template<typename ReturnPolicy = return_policy::copy, typename Ret, typename... Args>
		Type& def(char const* name,
		          Ret (T::*method)(Args...))
		{
			typedef typename ReturnPolicy::template get<Ret>::type return_type;
			_state->push(name);
			_state->push_callable(
				std::function<return_type(Storage&, Args...)>(
				    [method](Storage& self, Args&&... args) -> Ret {
						return (convert::to_ref(self).*method)(std::forward<Args>(args)...);
					}
				)
			);
			_state->settable(-3);
			return *this;
		}

		template<typename ReturnPolicy = return_policy::copy, typename Ret, typename... Args>
		Type& def(char const* name,
		          Ret (T::*method)(Args...) const)
		{
			typedef typename ReturnPolicy::template get<Ret>::type return_type;
			_state->push(name);
			_state->push_callable(
				std::function<return_type(Storage&, Args...)>(
				    [method](Storage& self, Args&&... args) -> Ret {
						return (convert::to_ref(self).*method)(std::forward<Args>(args)...);
					}
				)
			);
			_state->settable(-3);
			return *this;
		}

		typedef int (*raw_signature)(lua_State*);

		Type& def(char const* name, raw_signature fn)
		{
			_state->push(name);
			lua_pushlightuserdata(_state->ptr(), (void*)fn);
			_state->push(name);
			lua_pushcclosure(_state->ptr(), &_call_raw, 2);
			_state->settable(-3);
			return *this;
		}

	private:
		static std::string type_name()
		{ return boost::units::detail::demangle(typeid(T).name()); }

		static std::string method_name(char const* name)
		{ return type_name() + "::" + name + "()"; }

		static int _call_raw(lua_State* state)
		{
			void* fn_ptr = lua_touserdata(state, lua_upvalueindex(1));
			char const* name = lua_tostring(state, lua_upvalueindex(2));
			if (raw_signature fn = (raw_signature) fn_ptr)
			{
				return store_exception<int>(
					state,
					[&] { return fn(state); },
					[&] { return method_name(name); }
				);
			}
			std::abort();
		}
	};

}}
