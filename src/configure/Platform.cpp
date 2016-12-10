#include "Platform.hpp"
#include "error.hpp"
#include "log.hpp"

#include <map>
#include <cassert>

#ifdef _WIN32
# include <Windows.h>
#else
# include <sys/utsname.h>
#endif

#if defined(__DragonFly__)
# define CURRENT_OS configure::Platform::OS::dragonfly
#elif defined(__FreeBSD__)
# define CURRENT_OS configure::Platform::OS::freebsd
#elif defined(__NetBSD__)
# define CURRENT_OS configure::Platform::OS::netbsd
#elif defined(__OpenBSD__)
# define CURRENT_OS configure::Platform::OS::openbsd
#elif defined(_AIX)
# define CURRENT_OS configure::Platform::OS::aix
#elif defined(__hpux)
# define CURRENT_OS configure::Platform::OS::hpux
#elif defined(__linux__)
# define CURRENT_OS configure::Platform::OS::linux
#elif defined(__APPLE__) && defined(__MACH__)
# include <TargetConditionals.h>
# if TARGET_IPHONE_SIMULATOR == 1
# define CURRENT_OS configure::Platform::OS::ios // XXX should handle simulator ?
# elif TARGET_OS_IPHONE == 1
# define CURRENT_OS configure::Platform::OS::ios
# elif TARGET_OS_MAC == 1
# define CURRENT_OS configure::Platform::OS::osx
# endif
#elif defined(_WIN32)
# define CURRENT_OS configure::Platform::OS::windows
#elif defined(__sun) && defined(__SVR4)
# define CURRENT_OS configure::Platform::OS::solaris
#else
# define CURRENT_OS configure::Platform::OS::unknown
#endif

namespace configure {

	Platform::Platform()
		: _vendor(Vendor::unknown)
		, _arch(Arch::unknown)
		, _sub_arch(SubArch::unknown)
		, _os(OS::unknown)
	{}

	namespace {

		// Enum type name and strings of values.
		template<typename EnumType> struct strings;
		template<> struct strings<Platform::Vendor>
		{
			static std::string const& name()
			{
				static std::string const value = "Vendor";
				return value;
			}

			static std::vector<std::string> const& values()
			{
				static std::vector<std::string> res = {
					"unknown",
					"pc",
					"apple",
				};
				return res;
			}
		};

		template<> struct strings<Platform::Arch>
		{
			static std::string const& name()
			{
				static std::string const value = "Arch";
				return value;
			}

			static std::vector<std::string> const& values()
			{
				static std::vector<std::string> res = {
					"unknown",
					"x86",
					"x86_64",
				};
				return res;
			}
		};

		template<> struct strings<Platform::SubArch>
		{
			static std::string const& name()
			{
				static std::string const value = "SubArch";
				return value;
			}

			static std::vector<std::string> const& values()
			{
				static std::vector<std::string> res = {
					"unknown",
				};
				return res;
			}
		};

		template<> struct strings<Platform::OS>
		{
			static std::string const& name()
			{
				static std::string const value = "OS";
				return value;
			}

			static std::vector<std::string> const& values()
			{
				static std::vector<std::string> res = {
					"unknown",
					"windows",
					"linux",
					"osx",
					"ios",
					"dragonfly",
					"freebsd",
					"netbsd",
					"openbsd",
					"aix",
					"hpux",
					"solaris",
				};
				return res;
			}
		};

	}

	template<typename EnumType>
	std::string const& Platform::to_string(EnumType value)
	{
		auto& values = strings<EnumType>::values();
		auto index = static_cast<unsigned int>(value);
		assert(index < values.size() && "Invalid enum");
		return values[index];
	}

	template<typename EnumType>
	EnumType Platform::from_string(std::string const& value)
	{
		auto& values = strings<EnumType>::values();
		for (size_t i = 0, len = values.size(); i < len; ++i)
			if (values[i] == value)
				return static_cast<EnumType>(i);
		CONFIGURE_THROW(
			error::PlatformError(
				"Unknown " + strings<EnumType>::name() + " '" + value + "'"
			)
		);
	}

#define INSTANCIATE(EnumType) \
	template \
	std::string const& Platform::to_string<EnumType>(EnumType value); \
	template \
	EnumType Platform::from_string<EnumType>(std::string const& value); \
/**/
	INSTANCIATE(Platform::Vendor);
	INSTANCIATE(Platform::Arch);
	INSTANCIATE(Platform::SubArch);
	INSTANCIATE(Platform::OS);
#undef INSTANCIATE

	unsigned Platform::address_model() const
	{
		switch (_arch)
		{
		case Arch::x86:
			return 32;
		case Arch::x86_64:
			return 64;
		default:
			return 0;
		}
	}

	static std::string current_os_version()
	{
#ifdef _WIN32
		OSVERSIONINFO out;
		ZeroMemory(&out, sizeof(out));
		out.dwOSVersionInfoSize = sizeof(out);
		if (::GetVersionEx(&out) == 0)
			CONFIGURE_THROW_SYSTEM_ERROR("Couldn't retrieve the OS version");
		return std::to_string(out.dwMajorVersion) + "." + std::to_string(out.dwMinorVersion);
#endif
		return "";
	}

	static Platform::Arch current_arch()
	{
#ifdef _WIN32
		SYSTEM_INFO info;
		GetNativeSystemInfo(&info);
		switch (info.wProcessorArchitecture)
		{
		case PROCESSOR_ARCHITECTURE_INTEL: return Platform::Arch::x86;
		case PROCESSOR_ARCHITECTURE_AMD64: return Platform::Arch::x86_64;
		// PROCESSOR_ARCHITECTURE_INTEL            0
		// PROCESSOR_ARCHITECTURE_MIPS             1
		// PROCESSOR_ARCHITECTURE_ALPHA            2
		// PROCESSOR_ARCHITECTURE_PPC              3
		// PROCESSOR_ARCHITECTURE_SHX              4
		// PROCESSOR_ARCHITECTURE_ARM              5
		// PROCESSOR_ARCHITECTURE_IA64             6
		// PROCESSOR_ARCHITECTURE_ALPHA64          7
		// PROCESSOR_ARCHITECTURE_MSIL             8
		// PROCESSOR_ARCHITECTURE_AMD64            9
		// PROCESSOR_ARCHITECTURE_IA32_ON_WIN64    10
		}
		return Platform::Arch::unknown;
#else
		struct utsname info;
		if (uname(&info) != 0)
			return Platform::Arch::unknown;
		if (strcmp(info.machine, "i686") == 0 ||
		    strcmp(info.machine, "i586") == 0 ||
		    strcmp(info.machine, "i486") == 0 ||
		    strcmp(info.machine, "i386") == 0)
			return Platform::Arch::x86;
		try { return Platform::from_string<Platform::Arch>(info.machine); }
		catch (...) {
			log::warning("Couldn't convert", info.machine, "to a known architecture");
			return Platform::Arch::unknown;
		}
#endif
	}

	Platform Platform::current()
	{
		Platform p;
		p.os(CURRENT_OS);
		p.arch(current_arch());
		p.os_version(current_os_version());
		return p;
	}

	std::ostream& operator <<(std::ostream& out, Platform const& p)
	{
		out << "Platform(vendor=" << p.vendor_string()
		    << ", arch=" << p.arch_string()
		    << ", sub_arch=" << p.sub_arch_string()
		    << ", os=" << p.os_string();
        if (!p.os_version().empty())
            out << "-" << p.os_version();
        return out << ")";
	}
}
