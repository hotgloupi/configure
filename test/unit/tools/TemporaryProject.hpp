#pragma once

#include <configure/lua/State.hpp>
#include <configure/Build.hpp>
#include <configure/bind.hpp>

#include "TemporaryDirectory.hpp"

struct TemporaryProject
{
	TemporaryDirectory directory;
	configure::lua::State state;
	configure::Build build;

	TemporaryProject()
		: directory()
		, state()
		, build(state, directory.dir() / "build")
	{ configure::bind(state); }

	TemporaryProject(std::string const& conf)
		: TemporaryProject()
	{ this->directory.create_file("configure.lua", conf); }

	void configure()
	{ this->build.configure(directory.dir()); }

};
