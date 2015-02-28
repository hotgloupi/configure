#pragma once

#include "fwd.hpp"

#include <string>
#include <vector>

#include <boost/filesystem/path.hpp>
#include <boost/variant/variant.hpp>

namespace configure {

	class ShellFormatter
	{
	public:
		virtual ~ShellFormatter();

	public:
		virtual std::string operator ()(std::string value) const;
		virtual std::string operator ()(boost::filesystem::path const& value) const;
		virtual std::string operator ()(Node const& value) const;
	};

	// Dynamic shell argument
	class ShellArg
	{
	public:
		virtual ~ShellArg();
		virtual
		std::vector<std::string> string(Build const& build,
		                                DependencyLink const& link,
		                                ShellFormatter const& formatter) const = 0;
		virtual std::string dump() const;
	};

	// Store one shell command.
	class ShellCommand
	{
	public:
		typedef boost::variant<
			std::string,
			boost::filesystem::path,
			ShellArgPtr,
			NodePtr
		> Arg;
	private:
		std::vector<Arg> _args;

	public:
		ShellCommand()
		{}

		ShellCommand(ShellCommand&& other)
			: _args(std::move(other._args))
		{}

		ShellCommand(ShellCommand const& other)
			: _args(other._args)
		{}

		ShellCommand& operator =(ShellCommand const& other)
		{
			if (this != &other)
				_args = other._args;
			return *this;
		}

		ShellCommand& operator =(ShellCommand&& other)
		{
			if (this != &other)
				_args = std::move(other._args);
			return *this;
		}

		~ShellCommand();

		// Raw arguments
		std::vector<Arg> const& args() const
		{ return _args; }

		// Add one or more arguments.
		template<typename T, typename... Args>
		void append(T&& arg, Args&&... args)
		{
			_insert(std::forward<T>(arg));
			this->append(std::forward<Args>(args)...);
		}

		// Extend arguments with a container.
		template<typename T>
		void extend(T&& arg)
		{
			for (auto&& el: std::forward<T>(arg))
				this->append(el); // XXX use std::move() ?
		}

		// Generate a shell command.
		std::vector<std::string>
		string(Build const& build,
		       DependencyLink const& link,
		       ShellFormatter const& formatter) const;

		std::vector<std::string>
		string(Build const& build,
		       DependencyLink const& link) const;

		// Dump command without any build context (for debugging purposes).
		std::vector<std::string> dump() const;

	private:
		void append() {} // End case

		template<typename T>
		void _insert(T&& arg)
		{ _args.push_back(Arg(std::forward<T>(arg))); }

		void _insert(char const* str)
		{ _args.push_back(std::string(str)); }
	};

}
