#include <configure/TemporaryDirectory.hpp>

#include <configure/lua/State.hpp>
#include <configure/lua/Type.hpp>
#include <configure/bind.hpp>

namespace configure {

	static int TemporaryDirectory_new(lua_State* state)
	{
		lua::Converter<TemporaryDirectory>::push(state);
		return 1;
	}

	void bind_temporary_directory(lua::State& state)
	{
		lua::Type<TemporaryDirectory>(state, "TemporaryDirectory")
			.def("new", &TemporaryDirectory_new)
			.def("path", &TemporaryDirectory::path)
		;
	}
}
