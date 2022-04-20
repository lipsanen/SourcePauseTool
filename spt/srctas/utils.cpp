#include "stdafx.h"
#include "utils.hpp"
#include <stdarg.h>

namespace srctas
{
	std::string format(const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		char BUFFER[1024];
		std::string output;
		vsnprintf(BUFFER, sizeof(BUFFER), format, args);
		va_end(args);

		return BUFFER;
	}
} // namespace srctas