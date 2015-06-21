#pragma once

#include <iosfwd>
#include <string>

namespace configure {

	class Platform
	{
	public:
		enum class Vendor {
			unknown,
			pc,
			apple,
		};

		enum class Arch {
			unknown,

		};

		enum class SubArch {
			unknown,
		};

		enum class OS {
			unknown,
			windows,
			linux,
			osx,
			ios,
			dragonfly,
			freebsd,
			netbsd,
			openbsd,
			aix,
			hpux,
			solaris,
		};

	private:
		Vendor  _vendor;
		Arch    _arch;
		SubArch _sub_arch;
		OS      _os;

	public:
		Platform();

	public:
#define CONFIGURE_PLATFORM_PROPERTY(EnumType, name) \
		EnumType name() const \
		{ return _ ## name; } \
		std::string name ## _string() const \
		{ return to_string<EnumType>(_ ## name); } \
		Platform& name(EnumType value) \
		{ _ ## name = value; return *this; } \
		Platform& name(std::string const& value) \
		{ _ ## name = from_string<EnumType>(value); return *this; } \
/**/
		CONFIGURE_PLATFORM_PROPERTY(Vendor, vendor);
		CONFIGURE_PLATFORM_PROPERTY(Arch, arch);
		CONFIGURE_PLATFORM_PROPERTY(SubArch, sub_arch);
		CONFIGURE_PLATFORM_PROPERTY(OS, os);
#undef CONFIGURE_PLATFORM_PROPERTY

	public:
		static Platform current();

		template<typename EnumType>
		static std::string const& to_string(EnumType value);
		template<typename EnumType>
		static EnumType from_string(std::string const& value);
	};

	std::ostream& operator <<(std::ostream& out, Platform const& p);
}
