#pragma once

#include "fwd.hpp"

#include <boost/filesystem/path.hpp>
#include <boost/optional.hpp>

#include <vector>
#include <string>

namespace configure {

	typedef boost::filesystem::path Path;

	std::vector<Path> list_directory(Path const& dir);

	std::vector<Path> glob(Path const& dir, std::string const& pattern);
	std::vector<Path> rglob(Path const& dir, std::string const& pattern);

	class Filesystem
	{
	private:
		Build& _build;

	public:
		Filesystem(Build& build);
		~Filesystem();

		std::vector<NodePtr> glob(std::string const& pattern);
		std::vector<NodePtr> glob(Path const& dir, std::string const& pattern);
		std::vector<NodePtr> rglob(Path const& dir, std::string const& pattern);
		std::vector<NodePtr> list_directory(Path const& dir);
		NodePtr& find_file(std::vector<Path> const& directories,
		                   Path const& file);
		static boost::optional<Path> which(std::string const& program);
		NodePtr& copy(Path src, Path dst);
		NodePtr& copy(NodePtr& src, Path dst);
	};

}
