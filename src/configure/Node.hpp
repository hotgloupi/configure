#pragma once

#include "fwd.hpp"
#include "Environ.hpp"
#include "PropertyMap.hpp"

#include <boost/filesystem/path.hpp>

#include <functional>

namespace configure {

	// Base type used to store them.
	class Node
	{
		friend class BuildGraph;
	public:
		typedef size_t index_type;
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

		PropertyMap& properties() const;

		bool has_property(std::string key) const;

		template<typename Ret = Environ::Value>
		typename Environ::const_ref<Ret>::type property(std::string const& key) const
		{ return this->properties().get<Ret>(key); }

		// Assign key to value.
		template<typename Ret = Environ::Value, typename T>
		typename Environ::const_ref<Ret>::type
		set_property(std::string const& key, T&& value)
		{ return this->properties().set<Ret>(key, std::forward<T>(value)); }

		// Assign key to value not already set, returns associated value.
		template<typename Ret = Environ::Value, typename T>
		typename Environ::const_ref<T>::type
		set_property_default(std::string const& key, T&& value)
		{ return this->properties().set_default<Ret>(key, std::forward<T>(value)); }

		void set_lazy_property(std::string const& key,
		                       std::function<Environ::Value()> const& cb);

	public:
		virtual Kind kind() const = 0;
		virtual std::string const& name() const;
		virtual boost::filesystem::path const& path() const;
		virtual
		boost::filesystem::path
		relative_path(boost::filesystem::path const& start) const;
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
