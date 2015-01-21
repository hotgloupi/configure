#include "bind.hpp"

namespace configure {

	void bind(lua::State& state)
	{
		bind_build(state);
		bind_environ(state);
		bind_filesystem(state);
		bind_node(state);
		bind_path(state);
		bind_platform(state);
		bind_rule(state);
		bind_shell_command(state);
	}

}
