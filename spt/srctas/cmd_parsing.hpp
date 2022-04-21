#pragma once
#include <cstdint>
#include <cstddef>

namespace srctas
{
	const std::size_t ARG0_BUFFER_SIZE = 512;

	struct __declspec(dllexport) CommandArg
	{
		char commandString[ARG0_BUFFER_SIZE];
		static CommandArg ParseArg(const char*& command);
	};
} // namespace srctas
