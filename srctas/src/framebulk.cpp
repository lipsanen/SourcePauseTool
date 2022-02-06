#include "framebulk.hpp"
#include "utils.hpp"
#include <algorithm>
#include <charconv>
#include <sstream>
#include <vector>

namespace srctas
{
	using namespace std::placeholders;

	typedef void (*SetFunc)(FrameBulk& bulk, bool state);
	static ErrorReturn HandleLetter(std::size_t& strIndex, const std::string& line, bool& value, char setLetter)
	{
		char letter = line[strIndex];
		ErrorReturn rval;
		if (letter == setLetter)
		{
			++strIndex;
			rval.Success = true;
			value = true;
		}
		else if (letter == '-')
		{
			++strIndex;
			rval.Success = true;
			value = false;
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
		return HandleLetter(strIndex, line, bulk.##name##, ##letter##); \
	}

#define DEFINE_BULK_FUNC_MOVEMENT(name, letter) \
	static ErrorReturn Handle##name##(FrameBulk & bulk, std::size_t & strIndex, const std::string& line) \
	{ \
		return HandleLetter(strIndex, line, bulk.Movement.##name##, ##letter##); \
	}

	DEFINE_BULK_FUNC_MOVEMENT(Strafe, 's');
	DEFINE_BULK_FUNC_MOVEMENT(Lgagst, 'l');
	DEFINE_BULK_FUNC_MOVEMENT(AutoJump, 'j');
	DEFINE_BULK_FUNC_MOVEMENT(Duckspam, 'd');
	DEFINE_BULK_FUNC_MOVEMENT(Jumpbug, 'b');

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
		else if (letter == '-')
		{
			value = -1;
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
			bulk.Movement.StrafeType = value;
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
			bulk.Movement.JumpType = value;
		}

		return rval;
	}

	static ErrorReturn HandleAim(FrameBulk& bulk, std::size_t& strIndex, const std::string& line)
	{
		ErrorReturn rval;
		std::istringstream oss;
		std::size_t lastIndex = line.find('|', strIndex);

		std::string str = line.substr(strIndex, lastIndex - strIndex);

		if (str == "-")
		{
			rval.Success = true;
			return rval;
		}

		oss.str(str);

		std::string pitch, yaw, frames, cone;
		oss >> pitch >> yaw >> frames >> cone;

		auto pitchresult = std::from_chars(pitch.c_str(), pitch.c_str() + pitch.size(), bulk.AimPitch);
		auto framesresult = std::from_chars(frames.c_str(), frames.c_str() + frames.size(), bulk.AimFrames);
		auto coneresult = std::from_chars(cone.c_str(), cone.c_str() + cone.size(), bulk.Cone);

		bool yawParsedSuccess;

		if (yaw == "a")
		{
			yawParsedSuccess = true;
			bulk.AimYaw.Auto = true;
		}
		else
		{
			auto yawresult = std::from_chars(yaw.c_str(), yaw.c_str() + yaw.size(), bulk.AimYaw.Yaw);
			bulk.AimYaw.Auto = false;
			yawParsedSuccess = yawresult.ec == std::errc();
		}

		if (pitchresult.ec == std::errc() && framesresult.ec == std::errc() && yawParsedSuccess)
		{
			bulk.AimSet = true;
			rval.Success = true;
		}
		else
		{
			rval.Success = false;
			FormatString(rval.Error, "%s is not a valid aim command. The format is <pitch> <yaw> <frames> [cone]", str.c_str());
		}

		return rval;
	}

	static ErrorReturn HandleStrafeYaw(FrameBulk& bulk, std::size_t& strIndex, const std::string& line)
	{
		ErrorReturn rval;
		if (line[strIndex] == '-' && line[strIndex+1] == '|')
		{
			rval.Success = true;
			++strIndex;
		}
		else
		{
			rval.Success = false;
			const char* start = line.c_str() + strIndex;
			const char* end = line.c_str() + line.size();
			auto result = std::from_chars(start, end, bulk.Movement.StrafeYaw);

			if (result.ec == std::errc())
			{
				rval.Success = true;
				bulk.Movement.StrafeYawSet = true;
				strIndex += result.ptr - start;
			}
			else
			{
				rval.Success = false;
				std::size_t lastIndex = line.find('|', strIndex);
				std::string substr = line.substr(strIndex, lastIndex);
				FormatString(rval.Error, "%s is not a valid yaw!", substr.c_str());
			}
		}

		return rval;
	}

	static ErrorReturn HandleFrames(FrameBulk& bulk, std::size_t& strIndex, const std::string& line)
	{
		ErrorReturn rval;
		const char* start = line.c_str() + strIndex;
		const char* end = line.c_str() + line.size();
		auto result = std::from_chars(start, end, bulk.Frames);

		if (result.ec == std::errc())
		{
			rval.Success = true;
			strIndex += result.ptr - start;
		}
		else
		{
			rval.Success = false;
			std::size_t lastIndex = line.find('|', strIndex);
			std::string substr = line.substr(strIndex, lastIndex);
			FormatString(rval.Error, "%s is not a valid number!", substr.c_str());
		}

		return rval;
	}

	static ErrorReturn HandleCommand(FrameBulk& bulk, std::size_t& strIndex, const std::string& line)
	{
		ErrorReturn rval;
		// This never fails, just grab the rest of the line
		rval.Success = true;
		bulk.Commands = line.substr(strIndex);
		strIndex = line.size();

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

		handlers.push_back(IndexHandler{HandleAim, 3});
		handlers.push_back(IndexHandler{HandleStrafeYaw, 4});
		handlers.push_back(IndexHandler{HandleFrames, 5});
		handlers.push_back(IndexHandler{HandleCommand, 6});

		return handlers;
	}

	FrameBulk::FrameBulk()
	{
		Movement.Strafe = false;
		Movement.StrafeType = -1;
		Movement.JumpType = -1;
		Movement.Lgagst = false;
		Movement.AutoJump = false;
		Movement.Duckspam = false;
		Movement.Jumpbug = false;
		Movement.StrafeYawSet = false;
		Movement.StrafeYaw = 0;

		Forward = false;
		Left = false;
		Right = false;
		Back = false;
		Up = false;
		Down = false;

		Jump = false;
		Duck = false;
		Use = false;
		Attack1 = false;
		Attack2 = false;
		Reload = false;
		Walk = false;
		Sprint = false;

		AimSet = false;
		AimPitch = 0;
		AimFrames = 0;
		Cone = 0;

		Frames = 0;
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
			else
			{
				++i;
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