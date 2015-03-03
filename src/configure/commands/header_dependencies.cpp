#include "header_dependencies.hpp"

#include <boost/filesystem.hpp>

#include <fstream>
#include <set>

namespace configure { namespace commands {

	namespace fs = boost::filesystem;

	static
	void inspect(fs::path const& source,
	             std::set<fs::path>& seen,
	             std::vector<fs::path> const& include_directories)
	{
		if (!fs::is_regular_file(source))
			return;

		std::vector<boost::filesystem::path> found;
		std::ifstream f(source.string());
		std::string line;
		while (std::getline(f, line))
		{
			char const* ptr = line.c_str();
			while (*ptr == ' ' || *ptr == '\t') // XXX should use isblank
				ptr += 1;
			if (*ptr != '#')
				continue;
			ptr += 1;
			if (std::strncmp(ptr, "include", sizeof("include") - 1) != 0)
				continue;
			ptr += sizeof("include");

			while (*ptr == ' ' || *ptr == '\t') // XXX should use isblank
				ptr += 1;

			if (*ptr != '<' && *ptr != '"')
				continue;
			ptr += 1;
			size_t i = 0;
			while (ptr[i] != '\0' && ptr[i] != '>' && ptr[i] != '"')
				i += 1;
			found.push_back(std::string(ptr, i));
		}
		fs::path source_dir = source.parent_path();
		for (auto& el: found)
		{
			fs::path local = source_dir / el;
			if (fs::is_regular_file(local))
			{
				if (seen.insert(local).second == true)
					inspect(local, seen, include_directories);
				continue;
			}
			for (auto const& include_dir: include_directories)
			{
				fs::path local = include_dir / el;
				if (fs::is_regular_file(local))
				{
					if (seen.insert(local).second == true)
						inspect(local, seen, include_directories);
					continue;
				}
			}
		}
	}

	void header_dependencies(
	    std::ostream& out,
	    boost::filesystem::path const& source,
	    std::vector<boost::filesystem::path> const& targets,
	    std::vector<boost::filesystem::path> const& include_directories)
	{
		std::set<fs::path> seen;
		inspect(source, seen, include_directories);

		for (auto& target: targets)
			out << target.string() << ' ';
		out << ": \\\n";

		for (auto& el: seen)
			out << "  " << el.string() << " \\\n";
	}

}}

