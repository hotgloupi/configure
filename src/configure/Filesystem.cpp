#include "Filesystem.hpp"

#include "Build.hpp"
#include "Rule.hpp"
#include "ShellCommand.hpp"
#include "error.hpp"
#include "log.hpp"
//#include <boost/algorithm/string/split.hpp>
//#include <boost/algorithm/string/classification.hpp>

#include <boost/tokenizer.hpp>
#include <boost/filesystem.hpp>
#include <boost/scope_exit.hpp>
#include <boost/system/system_error.hpp>
#include <boost/algorithm/string.hpp>

#include <cstdlib>
#ifdef _WIN32
# include <windows.h>
# include <Shlwapi.h>
#else
# include <glob.h>
#endif

namespace fs = boost::filesystem;

namespace configure {

	Filesystem::Filesystem(Build& build)
		: _build(build)
	{}

	Filesystem::~Filesystem() {}

	std::vector<Path> glob(Path const& base_dir, std::string const& pattern)
	{
		std::vector<Path> res;
		std::string full_pattern = (base_dir / pattern).string();
#ifdef _WIN32
		WIN32_FIND_DATA find_data;
		auto handle = ::FindFirstFile(full_pattern.c_str(), &find_data);
		if (handle == INVALID_HANDLE_VALUE)
		{
			auto err = ::GetLastError();
			if (err == ERROR_FILE_NOT_FOUND && fs::is_directory(base_dir))
				return res;
			CONFIGURE_THROW(
				error::InvalidPath("Invalid glob pattern")
					<< error::path(base_dir / pattern)
					<< error::nested(
						std::make_exception_ptr(
							boost::system::system_error(
								err,
								boost::system::system_category(),
								"FindFirstFile()"
							)
						)
					)
			);
		}
		BOOST_SCOPE_EXIT((&handle)){ ::FindClose(handle); } BOOST_SCOPE_EXIT_END

		std::vector<char> buffer;
		do {
			if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;
# ifdef UNICODE
#error "NOPE "
			int size = ::WideCharToMultiByte(
				CP_UTF8,
				0,
				find_data.cFileName,
				-1, NULL, 0, NULL, NULL
			);
			if (size > 0)
			{
				buffer.resize(size);
				::WideCharToMultiByte(
					CP_UTF8,
					0,
					find_data.cFileName,
					-1,
					static_cast<BYTE*>(&buffer[0]),
					buffer.size(),
					NULL,
					NULL
				);
				res.push_back(std::string(&buffer[0], size));
			}
			else
				throw std::runtime_error("Couldn't encode a filename to UTF8");
# else // !UNICODE
			if (!PathMatchSpec(find_data.cFileName, pattern.c_str()))
				log::debug("Discarding", &find_data.cFileName[0]);
			else
				res.push_back(base_dir / &find_data.cFileName[0]);

# endif
			log::debug("Found", res.back(), "that matches", pattern);

		} while (::FindNextFile(handle, &find_data));
#else // !_WIN32
		glob_t glob_state;

		int const ret = ::glob(
			full_pattern.c_str(),
			GLOB_NOSORT,
			nullptr,
			&glob_state
		);
		struct glob_guard {
			glob_t* state;
			glob_guard(glob_t* state) : state(state) {}
			~glob_guard() { ::globfree(state); }
		} guard(&glob_state);

		if (ret != 0)
		{
			if (ret == GLOB_NOMATCH) return res;
			if (ret == GLOB_NOSPACE) throw std::bad_alloc();
			if (ret == GLOB_ABORTED) throw std::runtime_error("Read error");
			throw std::runtime_error("Unknown glob error");
		}
		int idx = 0;
		while (char const* path = glob_state.gl_pathv[idx])
		{
			res.push_back(path);
			idx += 1;
		}
#endif
		return res;
	}

	std::vector<Path> rglob(Path const& dir, std::string const& pattern)
	{
		std::vector<Path> res = glob(dir, pattern);
		fs::recursive_directory_iterator it(dir), end;
		for (; it != end; ++it)
		{
			if (fs::is_directory(it->status()))
			{
				auto dir_res = glob(*it, pattern);
				res.insert(res.end(), dir_res.begin(), dir_res.end());
			}
		}
		return res;
	}

	std::vector<NodePtr> Filesystem::glob(Path const& dir,
	                                      std::string const& pattern)
	{
		Path base_dir =
		    dir.is_absolute() ? dir : _build.project_directory() / dir;
		auto paths = configure::glob(base_dir, pattern);
		std::vector<NodePtr> res;
		res.reserve(paths.size());
		for (auto&& p: paths)
			res.emplace_back(_build.source_node(std::move(p)));
		return res;
	}

	std::vector<NodePtr> Filesystem::glob(std::string const& pattern)
	{
		return this->glob(_build.project_directory(), pattern);
	}

	std::vector<NodePtr> Filesystem::rglob(Path const& dir,
	                                       std::string const& pattern)
	{
		Path base_dir =
		    dir.is_absolute() ? dir : _build.project_directory() / dir;
		auto paths = configure::rglob(base_dir, pattern);
		std::vector<NodePtr> res;
		res.reserve(paths.size());
		for (auto&& p: paths)
			res.emplace_back(_build.source_node(std::move(p)));
		return res;
	}

	std::vector<Path> list_directory(Path const& dir)
	{
		std::vector<Path> res;

		fs::directory_iterator it(dir), end;
		for (; it != end; ++it)
			res.push_back(*it);

		return res;
	}

	std::vector<NodePtr> Filesystem::list_directory(Path const& dir)
	{
		if (!dir.is_absolute())
			return this->list_directory(_build.project_directory() / dir);

		std::vector<NodePtr> res;
		fs::directory_iterator it(dir), end;
		for (; it != end; ++it)
		{
			if (fs::is_directory(it->status()))
				res.push_back(_build.directory_node(*it));
			else
				res.push_back(_build.file_node(*it));
		}

		return res;
	}

	NodePtr& Filesystem::find_file(std::vector<Path> const& directories,
	                               Path const& file)
	{

		for (auto& dir: directories)
		{
			auto path = dir / file;
			if (fs::is_regular_file(path))
				return _build.file_node(path);
		}
		CONFIGURE_THROW(
		    error::FileNotFound("Cannot find the requested file")
				<< error::path(file)
		);
	}

#ifdef _WIN32
# define PATH_SEP ";"
#else
# define PATH_SEP ":"
#endif

	boost::optional<Path> Filesystem::which(std::string const& program_name)
	{
		Path program(program_name);
		if ((program.is_absolute() || program.is_relative()) && fs::is_regular_file(program))
			return fs::absolute(program);
		char const* PATH = ::getenv("PATH");
		if (PATH == nullptr)
			return boost::none;
		std::vector<std::string> out;
		boost::char_separator<char> sep(PATH_SEP);
		std::string path(PATH);
		boost::tokenizer<boost::char_separator<char>> tokenizer(path, sep);
		for (auto&& el: tokenizer)
		{
			Path full = Path(el) / program;
			//while (fs::is_symlink(full))
			//{
			//	full = fs::read_symlink(full);
			//	if (!full.is_absolute())
			//		full = Path(el) / full;
			//}

			if (fs::is_regular_file(full))
				return full;
		}
#ifdef _WIN32
		if (!boost::iends_with(program.string(), ".exe"))
			return which(program.string() + ".exe");
#endif
		return boost::none;
	}

	NodePtr& Filesystem::copy(Path src, Path dst)
	{ return this->copy(_build.source_node(std::move(src)), std::move(dst)); }

	NodePtr& Filesystem::copy(NodePtr& src_node, Path dst)
	{
		auto& dst_node = _build.target_node(std::move(dst));
		ShellCommand cmd;
		cmd.append(
			_build.option<Path>("CP", "Copy program", "cp"),
			src_node,
			dst_node
		);
		_build.add_rule(
			Rule()
				.add_source(src_node)
				.add_target(dst_node)
				.add_shell_command(std::move(cmd))
		);
		return dst_node;
	}

}
