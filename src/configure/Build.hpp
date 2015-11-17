#pragma once

#include "fwd.hpp"
#include "lua/fwd.hpp"
#include "Environ.hpp"

#include <map>
#include <iosfwd>

#include <boost/filesystem/path.hpp>
#include <boost/optional/optional_fwd.hpp>

namespace configure {

	// Represent a build directory.
	//
	// A build is ultimatly a graph of rules that generate files.
	class Build
	{
	public:
		typedef boost::filesystem::path path_t;

	private:
		struct Impl;
		std::unique_ptr<Impl> _this;

	public:
		Build(path_t configure_program, lua::State& lua, path_t root_directory);
		Build(path_t configure_program, lua::State& lua, path_t root_directory,
		      std::map<std::string, std::string> build_args);
		~Build();

	public:
		void configure(path_t const& project_directory,
		               path_t const& sub_directory = ".",
		               bool has_args = false);

	public:
		// Current project directory
		path_t const& project_directory() const;

		// List of all project configured so far.
		std::vector<path_t> const& configured_projects() const;

		// Root build directory
		path_t const& root_directory() const;

		// Current build directory
		path_t const& directory() const;

		// The build root node (unnamed virtual node).
		NodePtr const& root_node() const;

		// Build graph
		BuildGraph const& build_graph() const;

		// Filesystem
		Filesystem& fs();

		// Environ
		Environ& env();

		// Path to the configure executable.
		path_t const& configure_program() const;

		// Lua state in use for the configuration.
		lua::State& lua_state() const;

		void clear_properties();

	public:
		// Declare a new option.
		template<typename T>
		boost::optional<typename Environ::const_ref<T>::type>
		option(std::string name,
		       std::string description);

		// Declare a new option with a default value.
		template<typename T>
		typename Environ::const_ref<T>::type
		option(std::string name,
		       std::string description,
		       typename Environ::const_ref<T>::type default_value);

		// Declare a lazy option.
		template<typename T>
		typename Environ::const_ref<T>::type
		lazy_option(std::string name,
		            std::string description,
		            std::function<T()> const& fn);

		// Retreive the options map
		std::map<std::string, std::string> const& options() const;

	public:
		// Return a virtual node with the given `name`.
		NodePtr& virtual_node(std::string const& name);

		// Return a file node with the given absolute `path`.
		NodePtr& file_node(path_t path);

		// Return a file node relative to the project directory with the given
		// relative `path`.
		NodePtr& source_node(path_t const& path);

		// Return a file node relative to the build directory with the given
		// relative `path`.
		NodePtr& target_node(path_t const& path);

		// Return a directory node with the given absolute `path`.
		NodePtr& directory_node(path_t path);

		// Add a new rule.
		void add_rule(Rule const& rule);

		void visit_targets(std::function<void(NodePtr&)> const& fn);

	public:
		// The host platform
		Platform& host();

		// The target platform
		Platform& target();

	private:
		path_t _prepare_build_directory(path_t const& sub_directory);
		void _finalize_build_directory();

	public:
		static std::vector<path_t> const& possible_configure_files();
		static path_t find_project_file(path_t project_directory);

	public:
		void dump_graphviz(std::ostream& out) const;
		void dump_options(std::ostream& out) const;
		void dump_env(std::ostream& out) const;
		void dump_targets(std::ostream& out) const;
	};

}
