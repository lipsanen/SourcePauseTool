#pragma once

#include <string>

namespace srctas
{
	struct Error
	{
		std::string m_sMessage;
		bool m_bError = false;
	};

	std::string format(const char* format, ...);
} // namespace srctas