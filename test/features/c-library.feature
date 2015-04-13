@c
Feature: C library

	Scenario: Creating static library
		Given a source file my.c
		"""
		int my() { return 42; }
		"""
		And a source file test.c
		"""
		int my();
		int main() {
			if (my() == 42) return 0;
			return 1;
		}
		"""
		And a project configuration
		"""
		c = require('configure.lang.c')
		function configure(build)
			local compiler = c.compiler.find{build = build}
			libtest = compiler:link_static_library{
				name = 'test',
				sources = {'my.c'}
			}
			exe = compiler:link_executable{
				name = 'test',
				sources = {'test.c'},
				libraries = {libtest,}
			}
		end
		"""
		When I configure and build
		Then I can launch bin/test

	Scenario: Creating dynamic library
		Given a source file my.c
		"""
		#if defined(__GNUC__) || defined(__clang__)
		# define DLL_PUBLIC __attribute__ ((visibility ("default")))
		#else
		# define DLL_PUBLIC __declspec(dllexport)
		#endif
		DLL_PUBLIC int libtest_main()
		{ return 0; }
		"""
		And a source file test.c
		"""
		int libtest_main();
		int main()
		{
			return libtest_main();
		}
		"""
		And a project configuration
		"""
		c = require('configure.lang.c')
		function configure(build)
			local compiler = c.compiler.find{build = build}
			libtest = compiler:link_shared_library{
				name = 'test',
				sources = {'my.c'}
			}
			exe = compiler:link_executable{
				name = 'test',
				sources = {'test.c'},
				libraries = {libtest,}
			}
		end
		"""
		When I configure and build
		Then I can launch bin/test
