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
		Build(lua::State& lua, path_t root_directory);
		Build(lua::State& lua, path_t root_directory,
		      std::map<std::string, std::string> build_args);
		~Build();

	public:
		void configure(path_t const& project_directory,
		               path_t const& sub_directory = ".");

	public:
		// Current project directory
		path_t const& project_directory() const;

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

	public:
		template<typename T>
		boost::optional<T> option(std::string name,
		                          std::string const& description);
		template<typename T>
		typename Environ::const_ref<T>::type
		option(std::string name,
		       std::string const& description,
		       typename Environ::const_ref<T>::type default_value);


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

		void add_rule(Rule const& rule);

	public:
		Platform& host();
		Platform& target();

	private:
		path_t _prepare_build_directory(path_t const& sub_directory);
		void _finalize_build_directory();

	public:
		static std::vector<path_t> const& possible_configure_files();
		static path_t find_project_file(path_t project_directory);

	public:
		void dump_graphviz(std::ostream& out);
	};

}
