--- All modules
-- @module configure.modules
local M = {}

setmetatable(M, {
	__index = function (self, module)
		return require('configure.modules.' .. module)
	end
})

return M
