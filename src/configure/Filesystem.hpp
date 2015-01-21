#pragma once

#include "fwd.hpp"

#include <boost/filesystem/path.hpp>
#include <boost/optional.hpp>

#include <vector>
#include <string>

namespace configure {

	class Filesystem
	{
	public:
		typedef boost::filesystem::path path_t;
	private:
		Build& _build;

	public:
		Filesystem(Build& build);
		~Filesystem();

		std::vector<NodePtr> glob(std::string const& pattern);
		std::vector<NodePtr> glob(path_t const& dir, std::string const& pattern);
		std::vector<NodePtr> rglob(path_t const& dir, std::string const& pattern);
		boost::optional<path_t> which(std::string const& program);
		NodePtr& copy(path_t src, path_t dst);
		NodePtr& copy(NodePtr& src, path_t dst);
	};

}
