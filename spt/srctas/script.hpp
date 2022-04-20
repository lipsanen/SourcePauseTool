#pragma once

#include "framebulk.hpp"
#include "utils.hpp"
#include <string>
#include <vector>

namespace srctas
{
	struct Script
	{
	public:
		Script(){};
		void Clear();
		Error WriteToFile(const char* filepath);
		static Error ParseFrom(const char* filepath, Script* output);
		std::vector<FrameBulk> m_vFrameBulks;
	};
} // namespace srctas