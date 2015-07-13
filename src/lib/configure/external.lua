--- @module configure.external

local M = {
	Project = require('configure.external.Project'),
	AutotoolsProject = require('configure.external.AutotoolsProject'),
	CMakeProject = require('configure.external.CMakeProject'),
}

return M
