#include "Generator.hpp"

namespace configure {

	Generator::Generator(Build& build,
	                     path_t project_directory,
	                     path_t configure_exe,
	                     std::string name)
		: _build(build)
		, _project_directory(std::move(project_directory))
		, _configure_exe(std::move(configure_exe))
		, _name(std::move(name))
	{}

	Generator::~Generator()
	{}

	void Generator::prepare()
	{}

}
