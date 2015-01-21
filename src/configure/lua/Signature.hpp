#pragma once

#include "fwd.hpp"

#include <functional>

namespace configure { namespace lua {

	template<typename Ret, typename... Args>
	struct Signature<Ret(*)(Args...)>
	{ typedef Ret type(Args...); };

	template<typename Ret, typename... Args>
	struct Signature<Ret(&)(Args...)>
	{ typedef Ret type(Args...); };

	template<typename Ret, typename... Args>
	struct Signature<std::function<Ret(Args...)>>
	{ typedef Ret type(Args...); };

}}
