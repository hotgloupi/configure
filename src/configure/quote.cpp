#include "quote.hpp"

#include <boost/algorithm/string/join.hpp>

namespace configure {

	static char const* unix_special_characters = "`^~<>|;()$*'\\&# \t\n\r";

	static char const* windows_special_characters = "^<>|'&# \t\n\r";

	template<CommandParser target>
	static bool needs_quote(char const c)
	{
		char const* tab = nullptr;
		switch (target)
		{
		case CommandParser::unix_shell:
		case CommandParser::make:
			tab = unix_special_characters;
			break;
		case CommandParser::windows_shell:
		case CommandParser::nmake:
			tab = windows_special_characters;
			break;
		default:
			std::abort();
		}

		while (*tab != '\0' && *tab != c)
			tab++;
		return *tab != '\0';
	}

	template<CommandParser target>
	std::string quote_arg(std::string const& arg)
	{
		bool enclosing_quotes = std::any_of(arg.begin(), arg.end(), &needs_quote<target>);

		std::string res;
		res.reserve(arg.size() + 2);
		if (enclosing_quotes)
			res.push_back('"');

		int backslashes = 0;
		for (auto c: arg)
		{
			if ((target == CommandParser::unix_shell ||
			     target == CommandParser::make) &&
			    (c == '\\' || c == '\'' || c == '`' || c == '$' || c == '"'))
				res.push_back('\\');
			if (target == CommandParser::windows_shell ||
			    target == CommandParser::nmake)
			{
				if (c == '\\') backslashes += 1;
				else if (c == '"')
				{
					while (backslashes > 0)
					{
						backslashes -= 1;
						res.push_back('\\');
					}
					res.push_back('\\');
				}
				else backslashes = 0;
			}

			if (c == '$')
			{
				if (target == CommandParser::make)
					res.push_back('$');
			}

			if (target == CommandParser::nmake ||
			    target == CommandParser::windows_shell)
			{
				if (c == '%')
					res.push_back('%');
			}
			res.push_back(c);
		}

		if (enclosing_quotes)
		{
			while (backslashes > 0)
			{
				backslashes -= 1;
				res.push_back('\\');
			}
			res.push_back('"');
		}
		return res;
	}

	template<CommandParser target>
	std::string quote(std::vector<std::string> const& cmd)
	{
		std::vector<std::string> res;
		for (auto& arg: cmd)
			res.push_back(quote_arg<target>(arg));
		return boost::join(res, " ");
	}

#define INSTANCIATE(e) \
	template \
	std::string quote<CommandParser::e>(std::vector<std::string> const&); \
/**/
	INSTANCIATE(unix_shell);
	INSTANCIATE(windows_shell);
	INSTANCIATE(make);
	INSTANCIATE(nmake);
#undef INSTANCIATE

	std::string quote_arg(CommandParser target, std::string const& arg)
	{
		switch (target)
		{
#define CASE(value) \
		case CommandParser::value: \
			return quote_arg<CommandParser::value>(arg)
		CASE(unix_shell);
		CASE(windows_shell);
		CASE(nmake);
		CASE(make);
#undef CASE
		default:
			std::abort();
		}
	}
}
