--- C++ Boost libraries
-- @module configure.lang.cxx.libraries

local M = {}

--- Find Boost libraries
--
-- @param args
-- @param args.compiler A compiler instance
-- @param[opt] args.components A list of components (defaults to all)
-- @param[opt] args.version The required version (defaults to the latest found)
-- @param[opt] args.env_prefix A prefix for all environment variables (default to BOOST)
-- @param args.
function M.find(args)
	local env_prefix = args.env_prefix or 'BOOST'
	local build = args.compiler.build
	local fs = build:fs()
	local boost_include_dir = build:lazy_path_option(
		env_prefix .. '-include-dir',
		"Boost include directory",
		function ()
			return fs:find_file(
				args.compiler:system_include_directories(),
				'boost/version.hpp'
			):path():parent_path():parent_path()
		end
	)
	local boost_version_header = build:file_node(boost_include_dir / 'boost/version.hpp')
	if not boost_version_header:path():exists() then
		build:error("Couldn't find 'boost/version.hpp' in", boost_include_dir)
	end

end

return M
