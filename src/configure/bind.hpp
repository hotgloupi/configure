#pragma once

#include "lua/fwd.hpp"

namespace configure {

	void bind(lua::State& state);

	void bind_build(lua::State& state);
	void bind_environ(lua::State& state);
	void bind_filesystem(lua::State& state);
	void bind_node(lua::State& state);
	void bind_path(lua::State& state);
	void bind_platform(lua::State& state);
	void bind_process(lua::State& state);
	void bind_rule(lua::State& state);
	void bind_shell_command(lua::State& state);
	void bind_temporary_directory(lua::State& state);

}
