Feature: Exercise configure command line

	Scenario: Select one build out of multiple
		Given a project configuration
		"""
		return function(build)
			build:string_option("TEST", "Testing vars")
		end
		"""
		When I configure with build1 test=build_one
		And I configure with build2 test=build_two
		Then build variable TEST in build1 equals "build_one"
		Then build variable TEST in build2 equals "build_two"

