#include "framebulk.hpp"
#include "utils.hpp"
#include <vector>

namespace srctas
{
	using namespace std::placeholders;

	typedef void (*SetFunc)(FrameBulk& bulk, bool state);
	static ErrorReturn HandleLetter(std::size_t& strIndex, const std::string& line, char setLetter)
	{
		char letter = line[strIndex];
		ErrorReturn rval;
		if (letter == setLetter)
		{
			++strIndex;
			rval.Success = true;
		}
		else if (letter == '-')
		{
			++strIndex;
			rval.Success = true;
		}
		else
		{
			FormatString(rval.Error, "Flag has to be %c or -", setLetter);
			rval.Success = false;
		}

		return rval;
	}

#define DEFINE_BULK_FUNC(name, letter) \
	static ErrorReturn Handle##name##(FrameBulk & bulk, std::size_t & strIndex, const std::string& line) \
	{ \
		auto rval = HandleLetter(strIndex, line, ##letter##); \
		if (rval.Success) \
		{ \
			bulk.##name## = true; \
		} \
		return rval; \
	}

	DEFINE_BULK_FUNC(Strafe, 's');
	DEFINE_BULK_FUNC(Lgagst, 'l');
	DEFINE_BULK_FUNC(AutoJump, 'j');
	DEFINE_BULK_FUNC(Duckspam, 'd');
	DEFINE_BULK_FUNC(Jumpbug, 'b');

	DEFINE_BULK_FUNC(Forward, 'f');
	DEFINE_BULK_FUNC(Left, 'l');
	DEFINE_BULK_FUNC(Right, 'r');
	DEFINE_BULK_FUNC(Back, 'b');
	DEFINE_BULK_FUNC(Up, 'u');
	DEFINE_BULK_FUNC(Down, 'd');

	DEFINE_BULK_FUNC(Jump, 'j');
	DEFINE_BULK_FUNC(Duck, 'd');
	DEFINE_BULK_FUNC(Use, 'u');
	DEFINE_BULK_FUNC(Attack1, '1');
	DEFINE_BULK_FUNC(Attack2, '2');
	DEFINE_BULK_FUNC(Reload, 'r');
	DEFINE_BULK_FUNC(Walk, 'w');
	DEFINE_BULK_FUNC(Sprint, 's');

	static void HandleNumber(std::size_t& strIndex,
	                         const std::string& line,
	                         int& value,
	                         bool& success,
	                         char& letter)
	{
		letter = line[strIndex];
		++strIndex;
		value = letter - '0';
		if (value >= 0 && value <= 9)
		{
			success = true;
		}
		else
		{
			success = false;
		}
	}

	static ErrorReturn HandleStrafeType(FrameBulk& bulk, std::size_t& strIndex, const std::string& line)
	{
		ErrorReturn rval;
		int value;
		char letter;

		HandleNumber(strIndex, line, value, rval.Success, letter);

		if (!rval.Success)
		{
			FormatString(rval.Error, "Strafe type %c is not a number", letter);
		}
		else
		{
			bulk.StrafeType = value;
		}

		return rval;
	}

	static ErrorReturn HandleJumpType(FrameBulk& bulk, std::size_t& strIndex, const std::string& line)
	{
		ErrorReturn rval;
		int value;
		char letter;

		HandleNumber(strIndex, line, value, rval.Success, letter);

		if (!rval.Success)
		{
			FormatString(rval.Error, "Jump type %c is not a number", letter);
		}
		else
		{
			bulk.JumpType = value;
		}

		return rval;
	}

	static std::vector<IndexHandler>& GetHandlers()
	{
		static std::vector<IndexHandler> handlers;
		handlers.push_back(IndexHandler{HandleStrafe, 0});
		handlers.push_back(IndexHandler{HandleStrafeType, 0});
		handlers.push_back(IndexHandler{HandleJumpType, 0});
		handlers.push_back(IndexHandler{HandleLgagst, 0});
		handlers.push_back(IndexHandler{HandleAutoJump, 0});
		handlers.push_back(IndexHandler{HandleDuckspam, 0});
		handlers.push_back(IndexHandler{HandleJumpbug, 0});

		handlers.push_back(IndexHandler{HandleForward, 1});
		handlers.push_back(IndexHandler{HandleLeft, 1});
		handlers.push_back(IndexHandler{HandleRight, 1});
		handlers.push_back(IndexHandler{HandleBack, 1});
		handlers.push_back(IndexHandler{HandleUp, 1});
		handlers.push_back(IndexHandler{HandleDown, 1});

		handlers.push_back(IndexHandler{HandleJump, 2});
		handlers.push_back(IndexHandler{HandleDuck, 2});
		handlers.push_back(IndexHandler{HandleUse, 2});
		handlers.push_back(IndexHandler{HandleAttack1, 2});
		handlers.push_back(IndexHandler{HandleAttack2, 2});
		handlers.push_back(IndexHandler{HandleReload, 2});
		handlers.push_back(IndexHandler{HandleWalk, 2});
		handlers.push_back(IndexHandler{HandleSprint, 2});

		return handlers;
	}

	ErrorReturn FrameBulk::ParseFrameBulk(const std::string& line, FrameBulk& bulk)
	{
		int field = 0;
		std::size_t handlerIndex = 0;
		std::vector<IndexHandler>& handlers = GetHandlers();

		for (std::size_t i = 0; i < line.size();)
		{
			char c = line[i];
			if (c == '|')
			{
				++field;
				++i;

				// Skip any remaining handlers in this field
				while (handlerIndex < handlers.size() && handlers[handlerIndex].Field < field)
				{
					++handlerIndex;
				}
			}
			else if (handlerIndex < handlers.size() && handlers[handlerIndex].Field == field)
			{
				auto returnVal = handlers[handlerIndex].handler(bulk, i, line);
				if (!returnVal.Success)
				{
					return returnVal;
				}

				++handlerIndex;
			}
		}

		return true;
	}

	ErrorReturn::ErrorReturn(bool value)
	{
		Success = value;
	}

	ErrorReturn::ErrorReturn(std::string value, bool bvalue)
	{
		Error = value;
		Success = bvalue;
	}
} // namespace tas