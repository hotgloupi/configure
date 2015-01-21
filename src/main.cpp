
#include <configure/Application.hpp>
#include <configure/error.hpp>

#include <iostream>

int main(int ac, char** av)
{
	try {
		configure::Application app(ac, av);
		app.run();
	} catch (...) {
		std::cerr << configure::error_string() << std::endl;
		return 1;
	}
}
