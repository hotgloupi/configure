#include "Node.hpp"
#include "BuildGraph.hpp"
#include "error.hpp"
#include "PropertyMap.hpp"
#include "utils/path.hpp"
#include "log.hpp"

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace configure {

	Node::Node(BuildGraph& graph, index_type index)
		: graph(graph)
		, index(index)
	{}

	Node::~Node()
	{}

	bool Node::is_virtual() const
	{ return this->kind() == virtual_node; }

	bool Node::is_directory() const
	{ return this->kind() == directory_node; }

	bool Node::is_file() const
	{ return this->kind() == file_node; }


	std::string Node::string() const
	{
		switch (this->kind())
		{
		case Node::file_node:
			return "<FileNode \"" + this->path().string() + "\">";
		case Node::directory_node:
			return "<DirectoryNode \"" + this->path().string() + "\">";
		case Node::virtual_node:
			return "<VirtualNode \"" + this->name() + "\">";
		}
		std::abort();
	}
	PropertyMap& Node::properties() const
	{ return this->graph.properties(*this); }

	bool Node::has_property(std::string key) const
	{ return this->properties().has(std::move(key)); }

	Environ::Value
	Node::set_cached_property(std::string const& key,
	                          std::function<Environ::Value()> const& cb)
	{
		if (!this->is_file())
			CONFIGURE_THROW(error::InvalidNode(
			  "Only file nodes support lazy properties, got " +
			  this->string()));
		std::time_t modification_time =
		  boost::filesystem::last_write_time(this->path());
		if (!this->has_property(key) ||
		    !this->has_property("last-write-time") ||
		    this->property<int64_t>("last-write-time") != modification_time)
		{
			log::debug("Computing lazy property", key);
			this->set_property(key, cb());
			this->properties().deferred_set(
			  "last-write-time", Environ::Value(modification_time));
		}
		else
		{
			log::debug("Keep", key, "lazy property value:",
			           this->property<int64_t>("last-write-time"), "!=",
			           modification_time);
		}
		return this->properties().get(key);
	}

	std::string const& Node::name() const
	{ throw std::runtime_error("This node has no name"); }

	boost::filesystem::path const& Node::path() const
	{ throw std::runtime_error("This node has no path"); }

	boost::filesystem::path Node::relative_path(boost::filesystem::path const& start) const
	{ return utils::relative_path(this->path(), start); }

	VirtualNode::VirtualNode(BuildGraph& graph, index_type index, std::string name)
		: Node(graph, index)
		, _name(std::move(name))
	{}

	std::string const& VirtualNode::name() const
	{ return _name; }

	FileNode::FileNode(BuildGraph& graph, index_type index,
	                   boost::filesystem::path path)
		: Node(graph, index)
		, _path(std::move(path))
	{}

	boost::filesystem::path const& FileNode::path() const
	{ return _path; }

	DirectoryNode::DirectoryNode(BuildGraph& graph, index_type index,
	                             boost::filesystem::path path)
		: Node(graph, index)
		, _path(std::move(path))
	{}

	boost::filesystem::path const& DirectoryNode::path() const
	{ return _path; }

}
