#pragma once
#include <cstdint>

namespace srctas
{
	const std::size_t ARG0_BUFFER_SIZE = 512;

	struct CommandArg
	{
		char commandString[ARG0_BUFFER_SIZE];
		static CommandArg ParseArg(const char*& command);
	};
} // namespace srctas
