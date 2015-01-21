--- @module configure.dependency
local M = {}

M.Dependency = {}

--- Create a new dependency.
function M.Dependency:new(object)
	object = object or {}
	setmetatable(object, self)
	self.__index = self
	return object
end

return M
