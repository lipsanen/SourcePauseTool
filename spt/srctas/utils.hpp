#pragma once

#include <string>
#include "platform.hpp"

namespace srctas
{
	struct DLL_EXPORT Error
	{
		std::string m_sMessage;
		bool m_bError = false;
	};

	std::string format(const char* format, ...);
} // namespace srctas