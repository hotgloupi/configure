#include "Node.hpp"
#include "BuildGraph.hpp"
#include "error.hpp"
#include "PropertyMap.hpp"

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
	{ return this->properties().values().has(std::move(key)); }

	template<typename T>
	typename Environ::const_ref<T>::type Node::property(std::string key) const
	{ return this->properties().values().get<T>(std::move(key)); }

	template<typename T>
	typename Environ::const_ref<T>::type
	Node::property(std::string key,
	               typename Environ::const_ref<T>::type default_value) const
	{
		return this->properties().values().get<T>(
		    std::move(key),
		    std::move(default_value)
		);
	}

	template<typename T>
	typename Environ::const_ref<T>::type
	Node::set_property(std::string key, T value)
	{
		return this->properties().values().set<T>(
		    std::move(key),
		    std::move(value)
		);
	}

	template<typename T>
	typename Environ::const_ref<T>::type
	Node::set_property_default(std::string key,
	                     typename Environ::const_ref<T>::type default_value)
	{
		return this->properties().values().set_default<T>(
		    std::move(key),
		    std::move(default_value)
		);
	}
#define INSTANCIATE(T) \
	template \
	Environ::const_ref<T>::type \
	Node::property<T>(std::string key) const; \
	template \
	Environ::const_ref<T>::type \
	Node::property<T>(std::string key, \
	                  Environ::const_ref<T>::type default_value) const; \
	template \
	Environ::const_ref<T>::type \
	Node::set_property<T>(std::string key, T value); \
	template \
	Environ::const_ref<T>::type \
	Node::set_property_default<T>(std::string key, \
	                              Environ::const_ref<T>::type default_value); \
/**/

	INSTANCIATE(std::string);
	INSTANCIATE(fs::path);
	INSTANCIATE(bool);
	INSTANCIATE(int);
#undef INSTANCIATE

	std::string const& Node::name() const
	{ throw std::runtime_error("This node has no name"); }

	boost::filesystem::path const& Node::path() const
	{ throw std::runtime_error("This node has no path"); }

	boost::filesystem::path Node::relative_path(boost::filesystem::path const& start) const
	{
		// Stolen from http://stackoverflow.com/questions/10167382/boostfilesystem-get-relative-path
		auto const& full = this->path();
		if (!start.is_absolute())
			CONFIGURE_THROW(
				error::InvalidPath("start path must be absolute when computing relative path")
					<< error::path(start)
			);
		fs::path ret;
		auto it_start = start.begin(), end_start = start.end();
		auto it_full = full.begin(), end_full = full.end();
		// Find common base
		while (it_start != end_start && it_full != end_full && *it_start == *it_full)
		{
			++it_start;
			++it_full;
		}

		// Navigate backwards in directory to reach previously found base
		for (; it_start != end_start; ++it_start )
		{
			if( (*it_start) != "." )
				ret /= "..";
		}

		// Now navigate down the directory branch
		for (; it_full != end_full; ++it_full)
			ret /= *it_full;
		return ret;
	}

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
