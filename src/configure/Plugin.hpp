#pragma once

#include <configure/fwd.hpp>
#include <configure/lua/fwd.hpp>

#include <boost/filesystem/path.hpp>

#include <memory>

namespace configure {

	class Plugin
	{
	private:
		struct Impl;
		std::unique_ptr<Impl> _this;

	public:
		Plugin(lua::State& s, boost::filesystem::path p, std::string name);
		Plugin(Plugin&& other);
		~Plugin();

		std::string const& name() const;
		std::string description() const;
		void initialize(Build& build);
		void finalize(Build& build);
	};

}
