--- Utility functions
-- @module configure.tools

local M = {}

function M.normalize_directories(build, dirs)
	res = {}
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

return M
