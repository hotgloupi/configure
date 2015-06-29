Feature: Cached properties

	Scenario Outline: Can set property to <value>
		Given a project configuration
		"""
		return function(build)
			local node = build:source_node(Path:new('test.txt'))
			local prop = node:set_cached_property('my-prop', function() return <value> end)
			assert(prop == <value>)
			assert(node:property('my-prop') == <value>)
		end
		"""
		And a source file test.txt
		"""
		Not important
		"""
		When I configure the build
		Then the build is configured

		Examples:
			| value |
			| 42    |
			| true  |
			| false  |
			| "some string"  |
			| Path:new("Some/path")  |

	Scenario: Cached property is cached
		Given a project configuration
		"""
		return function(build)
			local node = build:source_node(Path:new('test.txt'))
			local opt = build:int_option("P", "")
			local prop = node:set_cached_property('my-prop', function() return opt end)
			assert(prop == 42)
		end
		"""
		And a source file test.txt
		"""
		Not important
		"""
		When I configure with build P=42
		And I configure with build P=12
		Then build variable P in build equals "12"

	Scenario: Cached property are updated
		Given a project configuration
		"""
		return function(build)
			local node = build:source_node(Path:new('test.txt'))
			local opt = build:int_option("P", "")
			local prop = node:set_cached_property('my-prop', function() return opt end)
			assert(prop == opt)
		end
		"""
		And a source file test.txt
		"""
		Not important
		"""
		When I configure with build P=42
		And a source file test.txt
		"""
		Not important 2
		"""
		And I configure with build P=12
		Then it should pass
