#pragma once

#include <memory>

namespace configure {

	class Application;
	class Build;
	class BuildGraph;
	class Command;
	class Environ;
	class Filesystem;
	class Generator;
	class Node;
	class Platform;
	class PropertyMap;
	class Rule;
	class ShellArg;
	class ShellCommand;

	typedef std::shared_ptr<Command> CommandPtr;
	typedef std::shared_ptr<Node> NodePtr;
	typedef std::shared_ptr<ShellArg> ShellArgPtr;

}
