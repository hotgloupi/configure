#include "Build.hpp"

#include "BuildGraph.hpp"
#include "Command.hpp"
#include "DependencyLink.hpp"
#include "Environ.hpp"
#include "error.hpp"
#include "Filesystem.hpp"
#include "log.hpp"
#include "lua/State.hpp"
#include "Platform.hpp"
#include "Rule.hpp"
#include "quote.hpp"
#include "utils/path.hpp"
#include "PropertyMap.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/functional/hash.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/scope_exit.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/map.hpp>

#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <fstream>

namespace fs = boost::filesystem;

namespace std {

	template<> struct hash<boost::filesystem::path>
	{
		size_t operator ()(boost::filesystem::path const& p) const
		{ return hash_value(p); }
	};

}

namespace configure {

	struct Build::Impl
	{
		fs::path                                 root_directory;
		lua::State&                              lua;
		std::vector<fs::path>                    project_stack;
		std::vector<fs::path>                    build_stack;
		std::unordered_map<std::string, NodePtr> virtual_nodes;
		std::unordered_map<fs::path, NodePtr>    file_nodes;
		std::unordered_map<fs::path, NodePtr>    directory_nodes;
		std::map<fs::path, PropertyMap>          properties;
		BuildGraph                               build_graph;
		NodePtr                                  root_node;
		Filesystem                               fs;
		Environ                                  env;
		fs::path                                 env_path;
		fs::path                                 properties_path;
		std::map<std::string, std::string>       options;
		std::map<std::string, std::string>       build_args;
		Platform                                 host_platform;
		Platform                                 target_platform;

		Impl(Build& build, lua::State& lua, fs::path directory)
			: root_directory(std::move(directory))
			, lua(lua)
			, project_stack()
			, build_stack()
			, virtual_nodes()
			, file_nodes()
			, directory_nodes()
			, properties()
			, build_graph(properties)
			, root_node(this->build_graph.add_node<VirtualNode>(""))
			, fs(build)
			, env()
			, env_path(root_directory / ".build" / "env")
			, properties_path(root_directory / ".build" / "properties")
			, options()
			, build_args()
			, host_platform(Platform::current())
			, target_platform(Platform::current())
		{}
	};

	Build::Build(lua::State& lua, fs::path directory)
		: _this(new Impl(*this, lua, std::move(directory)))
	{
		_this->build_stack.push_back(_this->root_directory);
		if (fs::is_regular_file(_this->env_path))
		{
			try { _this->env.load(_this->env_path); }
			catch (...) {
				CONFIGURE_THROW(
					error::InvalidEnviron("Couldn't load the configuration file")
						<< error::path(_this->env_path)
						<< error::nested(std::current_exception())
				);
			}
		}

		if (fs::is_regular_file(_this->properties_path))
		{
			try {
				std::ifstream in(_this->properties_path.string(), std::ios::binary);
				boost::archive::binary_iarchive ar(in);
				ar & _this->properties;
			} catch (...) {
				CONFIGURE_THROW(
					error::InvalidEnviron("Couldn't load properties")
						<< error::path(_this->properties_path)
						<< error::nested(std::current_exception())
				);
			}
		}
	}

	Build::Build(lua::State& lua, fs::path root_directory,
	      std::map<std::string, std::string> build_args)
		: Build(lua, std::move(root_directory))
	{
		for (auto& pair: build_args)
			_this->build_args[Environ::normalize(pair.first)] = pair.second;
	}

	Build::~Build()
	{
		if (fs::is_directory(_this->root_directory))
		{
			try {
				_this->env.save(_this->env_path);
			}
			catch (...) {
				log::error("Couldn't save environ in", _this->env_path, ":",
						   error_string());
			}
			try {
				std::ofstream out(_this->properties_path.string(), std::ios::binary);
				boost::archive::binary_oarchive ar(out);
				ar & _this->properties;
			} catch (...) {
				log::error("Couldn't save properties in", _this->env_path, ":",
						   error_string());
			}
		}
		else
		{
			log::debug("Build directory", _this->root_directory,
			           "not found, dropping the environ");
		}
	}

	void Build::configure(fs::path const& project_directory,
	                      fs::path const& sub_directory)
	{
		try {
			if (!project_directory.is_absolute())
				CONFIGURE_THROW(error::InvalidProject("Not an absolute path"));
			if (!fs::is_directory(project_directory))
				CONFIGURE_THROW(error::InvalidProject("Not a directory"));
			if (fs::equivalent(project_directory, this->directory()))
				CONFIGURE_THROW(error::InvalidProject("Same as build directory"));
		} catch (error::InvalidProject& err) {
			throw err << error::path(project_directory);
		}
		_this->project_stack.push_back(project_directory);
		_this->build_stack.push_back(_prepare_build_directory(sub_directory));
		log::status("Configuring project", this->project_directory(), "in", this->directory());

		BOOST_SCOPE_EXIT((&_this)){
			_this->project_stack.pop_back();
			_this->build_stack.pop_back();
		} BOOST_SCOPE_EXIT_END

		try {
			_this->lua.load(find_project_file(project_directory));
			_this->lua.getglobal("configure");
			// XXX check if not nil
			_this->lua.construct<std::reference_wrapper<Build>>(*this);
			_this->lua.call(1);
			if (_this->project_stack.size() == 1) // Last project on the stack
				_finalize_build_directory();
		} catch (error::Base& e) {
			//e << error::message(
			//	"While configuring project " + project_directory.string()
			//);
			throw;
		}
	}

	fs::path const& Build::project_directory() const
	{
		if (_this->project_stack.empty())
			throw std::logic_error("No project on the stack");
		return _this->project_stack.back();
	}
	fs::path const& Build::root_directory() const
	{ return _this->root_directory; }

	fs::path const& Build::directory() const
	{ return _this->build_stack.back(); }

	NodePtr const& Build::root_node() const
	{ return _this->root_node; }

	BuildGraph const& Build::build_graph() const
	{ return _this->build_graph; }

	Filesystem& Build::fs()
	{ return _this->fs; }

	Environ& Build::env()
	{ return _this->env; }

	std::map<std::string, std::string> const& Build::options() const
	{ return _this->options; }

	template<typename T>
	boost::optional<T> Build::option(std::string name,
	                                 std::string const& description)
	{
		name = Environ::normalize(std::move(name));
		_this->options[name] = description;
		auto it = _this->build_args.find(name);
		if (it != _this->build_args.end())
		{
			log::debug("Found option", name, "in command line arguments:", it->second);
			if (std::is_same<T, bool>::value)
			{
				std::string str = boost::to_lower_copy(it->second);
				static std::map<std::string, bool> conversions = {
					{"true", true}, {"false", false},
					{"yes", true}, {"no", false},
					{"y", true}, {"n", false},
					{"1", true}, {"0", false},
				};
				bool value;
				try { value = conversions.at(str); }
				catch (...) {
					CONFIGURE_THROW(
						error::InvalidOption("Cannot convert '" + it->second +
						                     "' to a boolean")
					);
				}
				_this->env.set(name, value);
			}
			else
				_this->env.set(name, boost::lexical_cast<T>(it->second));
			_this->build_args.erase(it);
		}
		if (_this->env.has(name))
			return boost::optional<T>(_this->env.get<T>(name));
		return boost::none;
	}

	template<typename T>
	typename Environ::const_ref<T>::type
	Build::option(std::string name,
	              std::string const& description,
	              typename Environ::const_ref<T>::type default_value)
	{
		this->option<T>(name, description);
		if (!_this->env.has(name))
			_this->env.set(name, default_value);
		return _this->env.get<T>(name);
	}

#define INSTANCIATE(T) \
	template \
	boost::optional<T> Build::option<T>(std::string name, \
	                                    std::string const& description); \
	template \
	Environ::const_ref<T>::type \
	Build::option<T>(std::string name, \
	                 std::string const& description, \
	                 Environ::const_ref<T>::type default_value); \
/**/

	INSTANCIATE(std::string);
	INSTANCIATE(int);
	INSTANCIATE(bool);
	INSTANCIATE(fs::path);

	fs::path Build::_prepare_build_directory(fs::path const& sub_directory)
	{
		fs::create_directories(this->directory() / sub_directory / ".build");
		return fs::canonical(this->directory() / sub_directory);
	}


	void Build::_finalize_build_directory()
	{
		std::unordered_set<fs::path> directories;
		for (auto const& pair: _this->file_nodes)
		{
			if (utils::starts_with(pair.first, this->root_directory()))
				directories.insert(pair.first.parent_path());
		}

		for (auto&& d: directories)
			fs::create_directories(d);
	}

	std::vector<fs::path> const& Build::possible_configure_files()
	{
		static std::vector<fs::path> ret {
			"configure.lua",
			".configure.lua",
			".config/project.lua",
		};
		return ret;
	}

	fs::path Build::find_project_file(fs::path project_directory)
	{
		for (auto&& p: possible_configure_files())
			if (fs::is_regular_file(project_directory / p))
				return project_directory / p;
		throw std::runtime_error("No configuration file found in " + project_directory.string());
	}

	NodePtr& Build::virtual_node(std::string const& name)
	{
		if (name.empty())
			CONFIGURE_THROW(error::InvalidVirtualNode("Name cannot be empty"));
		auto it = _this->virtual_nodes.find(name);
		if (it != _this->virtual_nodes.end())
			return it->second;
		auto node = _this->build_graph.add_node<VirtualNode>(name);
		return (_this->virtual_nodes[name] = std::move(node));
	}

	NodePtr& Build::file_node(fs::path path)
	{
		path.make_preferred();
		if (!path.is_absolute())
			throw std::runtime_error("Not an absolute path: " + path.string());
		auto it = _this->file_nodes.find(path);
		if (it != _this->file_nodes.end())
			return it->second;
		auto node = _this->build_graph.add_node<FileNode>(path);
		return (_this->file_nodes[path] = std::move(node));
	}

	NodePtr& Build::directory_node(fs::path path)
	{
		path.make_preferred();
		if (!path.is_absolute())
			throw std::runtime_error("Not an absolute path: " + path.string());
		auto it = _this->directory_nodes.find(path);
		if (it != _this->directory_nodes.end())
			return it->second;
		auto node = _this->build_graph.add_node<DirectoryNode>(path);
		return (_this->directory_nodes[path] = std::move(node));
	}

	NodePtr& Build::source_node(fs::path const& path)
	{
		fs::path src;
		if (!path.is_absolute())
			src = this->project_directory() / path;
		else
		{
			src = path;
			if (utils::starts_with(src, this->root_directory()))
				CONFIGURE_THROW(
					error::InvalidSourceNode("Generated file cannot be a source node")
						<< error::path(src)
				);
		}
		if (!fs::is_regular_file(src))
			CONFIGURE_THROW(error::InvalidSourceNode("File not found")
				<< error::path(src)
			);
		return this->file_node(src);
	}

	NodePtr& Build::target_node(fs::path const& path)
	{
		if (path.is_absolute())
			return this->file_node(path);
		return this->file_node(this->directory() / path);
	}

	void Build::add_rule(Rule const& rule)
	{
		CommandPtr cmd = rule.command();
		if (rule.targets().empty())
			CONFIGURE_THROW(error::InvalidRule("No target specified"));

		if (rule.sources().empty())
			for (auto& target: rule.targets())
			{
				try {
					_this->build_graph.link(*_this->root_node, *target).command(cmd);
				} catch (error::CommandAlreadySet&) {
					CONFIGURE_THROW(
						error::InvalidRule(
							"Trying to generate " + target->string() + " twice"
						) << error::nested(std::current_exception())
					);
				}
			}
		else
			for (auto& target: rule.targets())
				for (auto& source: rule.sources())
				{
					try {
						_this->build_graph.link(*source, *target)
							.command(cmd);
					} catch (error::CommandAlreadySet&) {
						CONFIGURE_THROW(
							error::InvalidRule(
								"Trying to generate " + target->string() + " twice"
							) << error::nested(std::current_exception())
						);
					}
				}
	}

	Platform& Build::host()
	{ return _this->host_platform; }

	Platform& Build::target()
	{ return _this->target_platform; }

	void Build::dump_graphviz(std::ostream& out) const
	{
		struct Writer {
			Build const& _b;
			BuildGraph const& _g;

			Writer(Build const& b, BuildGraph const& g) : _b(b), _g(g) {}

			void operator ()(std::ostream& out, Node::index_type idx) const
			{
				auto& node = _g.node(idx);
				switch (node->kind())
				{
				case Node::file_node:
				case Node::directory_node:
					out << "[label=\"" << node->relative_path(_b.directory()).string() << "\"]";
					break;
				case Node::virtual_node:
					out << "[label=\"" << node->name() << "\"]";
					break;
				default:
					out << "[label=\"" << "UNKNOWN" << "\"]";
				}
			}

			void operator ()(std::ostream& out, DependencyLink::index_type idx) const
			{
				return;
				auto const& cmd = _g.link(idx).command();
				out << "[label=\"";
				bool first = true;
				for (auto&& shell_command: cmd.shell_commands())
				{
					if (first) first = false;
					else out << "\\n";
					out << quote<CommandParser::unix_shell>(shell_command.dump());
				}
				out << "\"]";

			}
		} writer(*this, _this->build_graph);
		boost::write_graphviz(
			out,
			_this->build_graph.graph(),
			writer,
			writer
		);
	}

	void Build::dump_options(std::ostream& out) const
	{
		for (auto& p: this->options())
		{
			out << "  - " << p.first;
			if (_this->env.has(p.first))
				out << " = " << _this->env.as_string(p.first);
			else
				out << " not set";
			out << " (" << p.second << ')' << std::endl;
		}
	}

	void Build::dump_env(std::ostream& out) const
	{
		for (auto& key: _this->env.keys())
		{
			out << "  - " << key << " = "
			    << _this->env.as_string(key) << std::endl;
		}
	}

	void Build::dump_targets(std::ostream& out) const
	{
		for (auto& p: _this->file_nodes)
		{
			if (utils::starts_with(p.first, this->directory()))
				out << "  - " << p.second->relative_path(this->directory())
				    << std::endl;
		}
	}

}
