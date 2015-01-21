#pragma once

#include "fwd.hpp"
#include "Graph.hpp"

#include <boost/filesystem/path.hpp>

namespace configure {

	// Base type used to store them.
	class Node
	{
		friend class BuildGraph;
	public:
		typedef Vertex index_type;
		enum Kind
		{
			file_node,
			directory_node,
			virtual_node,
		};

	public:
		BuildGraph& graph;
		index_type const index;

	protected:
		Node(BuildGraph& graph, index_type index);
		virtual ~Node();

	public:
		bool is_virtual() const;
		bool is_directory() const;
		bool is_file() const;

		std::string string() const;

	public:
		virtual Kind kind() const = 0;
		virtual std::string const& name() const;
		virtual boost::filesystem::path const& path() const;
		virtual boost::filesystem::path relative_path(boost::filesystem::path const& start) const;
	};

	// A virtual node is just a name
	class VirtualNode
		: public Node
	{
	private:
		std::string _name;
	public:
		VirtualNode(BuildGraph& graph, index_type index, std::string name);
		Kind kind() const final { return virtual_node; }
		std::string const& name() const override;
	};

	// A file node refers to a source file or a generated file.
	class FileNode
		: public Node
	{
	private:
		boost::filesystem::path _path;
	public:
		FileNode(BuildGraph& graph, index_type index,
		         boost::filesystem::path path);
		boost::filesystem::path const& path() const override;
		Kind kind() const final { return file_node; }
	};

	// A directory node.
	class DirectoryNode
		: public Node
	{
	private:
		boost::filesystem::path _path;
	public:
		DirectoryNode(BuildGraph& graph, index_type index,
		              boost::filesystem::path path);
		boost::filesystem::path const& path() const override;
		Kind kind() const final { return directory_node; }
	};

}
