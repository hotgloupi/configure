--- Utility functions
-- @module configure.tools

local M = {}

--- Convert directories to directory nodes if needed.
--
-- @param dirs a list of string, path or nodes
-- @return a list of directory `Node`s
function M.normalize_directories(build, dirs)
	return M.normalize_paths(build, dirs, Node.Kind.directory_node)
end

--- Convert a list of string, path or `Node` to a list of file `Node`s
--
-- @param dirs a list of string, path or nodes
-- @return a list of file `Node`s
function M.normalize_files(build, files)
	return M.normalize_paths(build, files, Node.Kind.file_node)
end

--- Convert a list of string, path or `Node` to a list of `Node`s
--
-- @param build Build instance
-- @param files a list of string, path or nodes
-- @tparam `Node.Kind` kind
-- @return a list of file `Node`s
function M.normalize_paths(build, paths, kind)
	local res = {}
	for _, path in ipairs(paths)
	do
		if path ~= nil then
			if type(path) == "string" then
				path = Path:new(path)
			end
			if getmetatable(path) == Path then
				if not path:is_absolute() then
					path = build:project_directory() / path
				end
				if kind == Node.Kind.file_node then
					path = build:file_node(path)
				elseif kind == Node.Kind.directory_node then
					path = build:directory_node(path)
				else
					build:error("Invalid node kind:", tostring(kind))
				end
			elseif getmetatable(path) ~= Node then
				error("Expected string, Path or Node, got " .. tostring(path))
			elseif path:kind() ~= kind then
				build:error("Expected a", tostring(kind), "node, got " .. tostring(path:kind()))
			end
			table.append(res, path)
		end
	end
	return res
end

--- Convert a string, `Node` or a `Path` to a `Path` instance
--
-- @param object
-- @treturn Path
function M.path(object)
	if type(object) == "string" then
		return Path:new(object)
	elseif getmetatable(object) == Node then
		return object:path()
	elseif getmetatable(object) == Path then
		return object
	end
	error("Cannot convert '" .. tostring(object) .. "' to a Path")
end

function M.unique(list)
	local res = {}
	for _, v in ipairs(list) do
		local found = false
		for _, o in ipairs(res) do
			if o == v then found = true; break end
		end
		if not found then
			table.append(res, v)
		end
	end
	return res
end

return M
