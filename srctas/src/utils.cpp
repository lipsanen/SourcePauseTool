#include "utils.hpp"
#include <cstdarg>
#include <stdio.h>

namespace srctas
{
	void FormatString(std::string& output, const char* value, ...)
	{
		static char FORMAT_BUFFER[256];

		va_list args;
		va_start(args, value);
		vsprintf_s(FORMAT_BUFFER, value, args);
		output = FORMAT_BUFFER;
	}
}