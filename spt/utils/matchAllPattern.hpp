#pragma once

#include <future>
#include "SPTLib\MemUtils.hpp"

namespace patterns
{
	struct MatchedPattern
	{
		uintptr_t ptr;
		int ptnNumber;
	};

	std::future<void> get_all_matches(std::vector<MatchedPattern>* result,
	                                  const void* start,
	                                  size_t length,
	                                  patterns::PatternWrapper* begin,
	                                  patterns::PatternWrapper* end);
} // namespace patterns