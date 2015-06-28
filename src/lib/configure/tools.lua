--- Utility functions
-- @module configure.tools

local M = {}

--- Convert directories to directory nodes if needed.
--
-- @param dirs a list of string, path or nodes
-- @return a list of directory `Node`s
function M.normalize_directories(build, dirs)
	local res = {}
	for _, dir in ipairs(dirs)
	do
		if dir ~= nil then
			if type(dir) == "string" then
				dir = Path:new(dir)
			end
			if getmetatable(dir) == Path then
				if not dir:is_absolute() then
					dir = build:project_directory() / dir
				end
				dir = build:directory_node(dir)
			elseif getmetatable(dir) ~= Node then
				error("Expected string, Path or Node, got " .. tostring(dir))
			elseif not dir:is_directory() then
				error("Expected a directory node, got " .. tostring(dir))
			end
			table.append(res, dir)
		end
	end
	return res
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
