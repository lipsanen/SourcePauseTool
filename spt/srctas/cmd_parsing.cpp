#include "stdafx.h"
#include "cmd_parsing.hpp"
#include <ctype.h>
#include <string>

namespace srctas
{
	CommandArg CommandArg::ParseArg(const char*& src)
	{
		CommandArg arg0;
		char* dest = arg0.commandString;

		for (std::size_t i = 0; i < ARG0_BUFFER_SIZE - 1 && *src && !std::isspace(*src) && *src != ';'; ++i)
		{
			// Skip past leading quotation marks
			if (*src == '"')
			{
				++src;
				continue;
			}

			*dest = tolower(*src);
			++dest;
			++src;
		}
		*dest = '\0';

		// Skip whitespace and quotation marks after we have parsed the arg
		while (std::isspace(*src) || *src == '"')
			++src;

		return arg0;
	}

	void RemoveKeyCode(char* command)
	{
		while (command && *command && (std::isspace(*command) || *command == '"'))
			++command;

		if(!command || *command == '\0' || (*command != '+' && *command != '-'))
			return;

		while(!std::isspace(*command) && *command != ';')
			++command;

		*command = '\0';
	}
} // namespace srctas