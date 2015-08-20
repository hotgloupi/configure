#include <configure/bind.hpp>

#include <configure/Platform.hpp>
#include <configure/lua/State.hpp>
#include <configure/lua/Type.hpp>

namespace configure {

#define PLATFORM_PROPERTY(T, name)                                             \
	static int Platform_##name(lua_State* state)                               \
	{                                                                          \
		Platform& self =                                                       \
		  lua::Converter<std::reference_wrapper<Platform>>::extract(state, 1); \
		if (lua_gettop(state) == 2) {                                          \
			if (lua_isstring(state, 2))                                        \
				self.name(lua_tostring(state, 2));                             \
			else if (lua_isnumber(state, 2))                                   \
				self.name(static_cast<Platform::T>(lua_tounsigned(state, 2))); \
			else                                                               \
				throw std::runtime_error("Expected number or string");         \
			lua_pushvalue(state, 1);                                           \
		}                                                                      \
		else                                                                   \
		{                                                                      \
			lua_pushunsigned(state, static_cast<int>(self.name()));            \
		}                                                                      \
		return 1;                                                              \
	}                                                                          \
	/**/
	PLATFORM_PROPERTY(Vendor, vendor);
	PLATFORM_PROPERTY(Arch, arch);
	PLATFORM_PROPERTY(SubArch, sub_arch);
	PLATFORM_PROPERTY(OS, os);

	static int Platform_string(lua_State* state)
	{
		Platform& self =
		  lua::Converter<std::reference_wrapper<Platform>>::extract(state, 1);
		std::stringstream ss;
		ss << self;
		lua::Converter<std::string>::push(state, ss.str());
		return 1;
	}

	void bind_platform(lua::State& state)
	{
		lua::Type<Platform, std::reference_wrapper<Platform>> type(state, "Platform");
		type
			.def("vendor", &Platform_vendor)
			.def("arch", &Platform_arch)
			.def("sub_arch", &Platform_sub_arch)
			.def("os", &Platform_os)
			.def("vendor_string", &Platform::vendor_string)
			.def("arch_string", &Platform::arch_string)
			.def("sub_arch_string", &Platform::sub_arch_string)
			.def("os_string", &Platform::os_string)
			.def("is_osx", &Platform::is_osx)
			.def("is_windows", &Platform::is_windows)
			.def("is_linux", &Platform::is_linux)
			.def("__tostring", &Platform_string)

		;
#define ENUM_VALUE(T, key) \
		lua_pushunsigned(\
			state.ptr(), \
			static_cast<unsigned>(Platform::T::key)\
		); \
		lua_setfield(state.ptr(), -2, #key); \
/**/
		// Vendor
		lua_newtable(state.ptr());
		ENUM_VALUE(Vendor, unknown);
		ENUM_VALUE(Vendor, pc);
		ENUM_VALUE(Vendor, apple);
		lua_setfield(state.ptr(), -2, "Vendor");

		// Arch
		lua_newtable(state.ptr());
		ENUM_VALUE(Arch, unknown);
		lua_setfield(state.ptr(), -2, "Arch");

		// SubArch
		lua_newtable(state.ptr());
		ENUM_VALUE(SubArch, unknown);
		lua_setfield(state.ptr(), -2, "SubArch");

		// OS
		lua_newtable(state.ptr());
		ENUM_VALUE(OS, unknown);
		ENUM_VALUE(OS, windows);
		ENUM_VALUE(OS, linux);
		ENUM_VALUE(OS, osx);
		ENUM_VALUE(OS, ios);
		ENUM_VALUE(OS, dragonfly);
		ENUM_VALUE(OS, freebsd);
		ENUM_VALUE(OS, netbsd);
		ENUM_VALUE(OS, openbsd);
		ENUM_VALUE(OS, aix);
		ENUM_VALUE(OS, hpux);
		ENUM_VALUE(OS, solaris);
		lua_setfield(state.ptr(), -2, "OS");
#undef ENUM_VALUE
	}

}
