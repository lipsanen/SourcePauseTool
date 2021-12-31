#include "stdafx.h"
#include "matchAllPattern.hpp"

namespace patterns
{
	void _get_all_matches(std::vector<MatchedPattern>* result,
	                      const void* start,
	                      size_t length,
	                      patterns::PatternWrapper* begin,
	                      patterns::PatternWrapper* end)
	{
		std::vector<MatchedPattern> out;
		for (auto pattern = begin; pattern != end; ++pattern)
		{
			const void* currentStart = start;
			size_t currentLength = length;
			uintptr_t address = 1;
			do
			{
				address = MemUtils::find_pattern(currentStart, currentLength, *pattern);
				if (address)
				{
					MatchedPattern match;
					match.ptnNumber = pattern - begin;
					match.ptr = address;

					out.push_back(match);
					currentStart = reinterpret_cast<void*>(address + 1);
					currentLength = reinterpret_cast<uintptr_t>(start)
					                - reinterpret_cast<uintptr_t>(currentStart) + length;
				}
			} while (address);
		}

		*result = std::move(out);
	}

	std::future<void> get_all_matches(std::vector<MatchedPattern>* result,
	                                  const void* start,
	                                  size_t length,
	                                  patterns::PatternWrapper* begin,
	                                  patterns::PatternWrapper* end)
	{
		return std::async(_get_all_matches, result, start, length, begin, end);
	}
} // namespace patterns