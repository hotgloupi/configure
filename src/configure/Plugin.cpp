#include "Plugin.hpp"

#include "lua/State.hpp"

namespace configure {

	struct Plugin::Impl
	{
		lua::State& state;
		boost::filesystem::path const path;
		std::string name;
		int table_ref;

		Impl(lua::State& s, boost::filesystem::path p, std::string name)
			: state(s)
			, path(std::move(p))
			, name(std::move(name))
		{
			this->state.load(this->path, 1);
			this->table_ref = luaL_ref(this->state.ptr(), LUA_REGISTRYINDEX);
		}

		void push_table()
		{
			lua_rawgeti(this->state.ptr(), LUA_REGISTRYINDEX, this->table_ref);
		}

		void push_value(char const* key)
		{
			this->push_table();
			lua_pushstring(this->state.ptr(), key);
			lua_gettable(this->state.ptr(), -2);
		}

		~Impl()
		{
			luaL_unref(this->state.ptr(),
			           LUA_REGISTRYINDEX,
			           this->table_ref);
		}

		void check()
		{
			this->push_table();
			if (!lua_istable(this->state.ptr(), -1))
				CONFIGURE_THROW(
					error::RuntimeError("Invalid plugin '" + this->name + "'")
					<< error::path(this->path)
				);
			this->state.pop();
		}

		void call(Build& build, char const* method)
		{
			this->push_value(method);
			if (!lua_isnil(this->state.ptr(), -1))
			{
				this->push_table();
				this->state.construct<std::reference_wrapper<Build>>(build);
				this->state.call(2);
				this->state.pop();
			}
			this->state.pop();
		}
	};

	Plugin::Plugin(lua::State& s, boost::filesystem::path p, std::string name)
		: _this(new Impl(s, std::move(p), std::move(name)))
	{
		_this->check();
	}

	Plugin::Plugin(Plugin&& other)
		: _this(std::move(other._this))
	{}

	Plugin::~Plugin()
	{}

	std::string const& Plugin::name() const
	{ return _this->name; }

	std::string Plugin::description() const
	{
		auto L = _this->state.ptr();
		_this->push_value("description");
		if (lua_isnil(L, -1))
			return "No description provided";
		auto res = lua::Converter<std::string>::extract(_this->state.ptr(), -1);
		_this->state.pop();
		return res;
	}

	void Plugin::initialize(Build& build)
	{ _this->call(build, "initialize"); }

	void Plugin::finalize(Build& build)
	{ _this->call(build, "finalize"); }
}
