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

	template<typename Iterator>
	void get_all_matches(std::vector<MatchedPattern>* result,
	                     const void* start,
	                     size_t length,
	                     Iterator begin,
	                     Iterator end)
	{
		for (auto pattern = begin; pattern != end; ++pattern)
		{
			const void* currentStart = start;
			size_t currentLength = length;
			uintptr_t address;
			do
			{
				address = MemUtils::find_pattern(currentStart, currentLength, *pattern);
				if (address)
				{
					MatchedPattern match;
					match.ptnNumber = pattern - begin;
					match.ptr = address;

					result->push_back(match);
					currentStart = reinterpret_cast<void*>(address + 1);
					currentLength = reinterpret_cast<uintptr_t>(start)
					                - reinterpret_cast<uintptr_t>(currentStart) + length;
				}
			} while (address);
		}
	}

	template<typename Iterator>
	auto get_all_matches_async(std::vector<MatchedPattern>* result,
	                           const void* start,
	                           size_t length,
	                           Iterator begin,
	                           Iterator end)
	{
		return std::async(get_all_matches<Iterator>, result, start, length, begin, end);
	}
} // namespace patterns