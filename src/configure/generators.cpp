#include "generators.hpp"
#include "generators/Makefile.hpp"
#include "generators/NMakefile.hpp"
#include "generators/Shell.hpp"
#include "error.hpp"

#include <boost/algorithm/string/predicate.hpp>

#include <map>

namespace fs = boost::filesystem;

namespace configure { namespace generators {

	template<typename G>
	static
	std::unique_ptr<Generator> create(Build& b,
	                                  fs::path project_dir,
	                                  fs::path configure_exe)
	{
		return std::unique_ptr<Generator>(
			new G(b, std::move(project_dir), std::move(configure_exe))
		);
	}

	typedef std::unique_ptr<Generator> (*creator_t)(Build&, fs::path, fs::path);
	typedef bool (*available_t)(Build&);
	struct GeneratorDescription
	{
		creator_t create;
		available_t is_available;
	};

	typedef std::map<std::string, GeneratorDescription> GeneratorMap;

	static GeneratorMap const& all()
	{
		static GeneratorMap res;
		if (res.empty())
		{
#define ADD_GENERATOR(T)\
			res[T::name()] = {&create<T>, &T::is_available};
			ADD_GENERATOR(Makefile);
			ADD_GENERATOR(NMakefile);
			ADD_GENERATOR(Shell);
#undef ADD_GENERATOR
		}
		return res;
	}

	std::unique_ptr<Generator> from_name(std::string const& name,
	                                     Build& build,
	                                     boost::filesystem::path project_directory,
	                                     boost::filesystem::path configure_exe)
	{
		for (auto& pair: all())
		{
			if (boost::iequals(pair.first, name))
			{
				if (!pair.second.is_available(build))
					CONFIGURE_THROW(
						error::InvalidGenerator(
							"Generator '" + name + "' is not available"
						)
					);
				return pair.second.create(
					build,
					std::move(project_directory),
					std::move(configure_exe)
				);
			}
		}
		CONFIGURE_THROW(
			error::InvalidGenerator("Unknown generator '" + name + "'")
		);
	}

	std::string first_available(Build& build)
	{
		for (auto& pair: all())
				if (pair.second.is_available(build))
					return pair.first;
		CONFIGURE_THROW(
		    error::InvalidGenerator("No generator available")
		);
	}

}}
