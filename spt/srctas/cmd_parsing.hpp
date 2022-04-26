#pragma once
#include <cstdint>
#include <cstddef>
#include "platform.hpp"

namespace srctas
{
	const std::size_t ARG0_BUFFER_SIZE = 512;

	struct DLL_EXPORT CommandArg
	{
		char commandString[ARG0_BUFFER_SIZE];
		static CommandArg ParseArg(const char*& command);
	};

	void DLL_EXPORT RemoveKeyCode(char* command);
} // namespace srctas
