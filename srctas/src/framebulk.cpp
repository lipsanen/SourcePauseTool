#include "framebulk.hpp"
#include "utils.hpp"
#include <algorithm>
#include <charconv>
#include <sstream>
#include <vector>

namespace srctas
{
	using namespace std::placeholders;

	struct _InternalFramebulk;

	typedef ErrorReturn(*BulkFunc)(_InternalFramebulk& bulk, std::size_t& lineIndex, const std::string& line);

	struct IndexHandler
	{
		BulkFunc handler;
		int Field;
	};

	typedef void (*SetFunc)(_InternalFramebulk& bulk, bool state);
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
	static ErrorReturn Handle##name##(_InternalFramebulk & bulk, std::size_t & strIndex, const std::string& line) \
	{ \
		return HandleLetter(strIndex, line, bulk.##name##, ##letter##); \
	}

	DEFINE_BULK_FUNC(Strafe, 's');
	DEFINE_BULK_FUNC(Lgagst, 'l');
	DEFINE_BULK_FUNC(AutoJump, 'j');
	DEFINE_BULK_FUNC(Duckspam, 'd');
	DEFINE_BULK_FUNC(Jumpbug, 'b');

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

	static ErrorReturn HandleStrafeType(_InternalFramebulk& bulk, std::size_t& strIndex, const std::string& line)
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

	static ErrorReturn HandleJumpType(_InternalFramebulk& bulk, std::size_t& strIndex, const std::string& line)
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

	static ErrorReturn HandleAim(_InternalFramebulk& bulk, std::size_t& strIndex, const std::string& line)
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

	static ErrorReturn HandleStrafeYaw(_InternalFramebulk& bulk, std::size_t& strIndex, const std::string& line)
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
			auto result = std::from_chars(start, end, bulk.StrafeYaw);

			if (result.ec == std::errc())
			{
				rval.Success = true;
				bulk.StrafeYawSet = true;
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

	static ErrorReturn HandleFrames(_InternalFramebulk& bulk, std::size_t& strIndex, const std::string& line)
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

	static ErrorReturn HandleCommand(_InternalFramebulk& bulk, std::size_t& strIndex, const std::string& line)
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

		handlers.push_back(IndexHandler{HandleJump, 1});
		handlers.push_back(IndexHandler{HandleDuck, 1});
		handlers.push_back(IndexHandler{HandleUse, 1});
		handlers.push_back(IndexHandler{HandleAttack1, 1});
		handlers.push_back(IndexHandler{HandleAttack2, 1});
		handlers.push_back(IndexHandler{HandleReload, 1});
		handlers.push_back(IndexHandler{HandleWalk, 1});
		handlers.push_back(IndexHandler{HandleSprint, 1});

		handlers.push_back(IndexHandler{HandleAim, 2});
		handlers.push_back(IndexHandler{HandleStrafeYaw, 3});
		handlers.push_back(IndexHandler{HandleFrames, 4});
		handlers.push_back(IndexHandler{HandleCommand, 5});

		return handlers;
	}

	_InternalFramebulk::_InternalFramebulk()
	{
		Strafe = false;
		StrafeType = -1;
		JumpType = -1;
		Lgagst = false;
		AutoJump = false;
		Duckspam = false;
		Jumpbug = false;
		StrafeYawSet = false;
		StrafeYaw = 0;

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

	ErrorReturn _InternalFramebulk::ParseFrameBulk(const std::string& line, _InternalFramebulk& bulk)
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

	ErrorReturn Framebulk::ParseFrameBulk(const std::string& line, Framebulk& bulk)
	{
		_InternalFramebulk internalFramebulk;
		ErrorReturn error = _InternalFramebulk::ParseFrameBulk(line, internalFramebulk);
		Framebulk framebulk;

		if (!error.Success)
		{
			return error;
		}

		error = AimState::AimStateFromInternal(line, framebulk.aimState, internalFramebulk);

		if (!error.Success)
		{
			return error;
		}

		error = ButtonsInput::ButtonsInputFromInternal(line, framebulk.buttonsInput, internalFramebulk);

		if (!error.Success)
		{
			return error;
		}

		error = MovementInput::MovementInputFromInternal(line, framebulk.movementInput, internalFramebulk);

		if (!error.Success)
		{
			return error;
		}

		framebulk.Commands = internalFramebulk.Commands;
		framebulk.Frames = internalFramebulk.Frames;

		return error;
	}

	ErrorReturn MovementInput::MovementInputFromInternal(const std::string& line, MovementInput& state, _InternalFramebulk& bulk)
	{
		ErrorReturn error(true);

		if (bulk.Strafe && bulk.StrafeType == -1)
		{
			error.Success = false;
			error.Error = "Strafe type must be set if strafing.";
			return error;
		}

		if (bulk.Strafe && bulk.JumpType == -1)
		{
			error.Success = false;
			error.Error = "Jump type must be set if strafing.";
			return error;
		}

		if (bulk.Strafe && bulk.StrafeYawSet)
		{
			error.Success = false;
			error.Error = "Strafe yaw must be set if strafing.";
			return error;
		}

		state.AutoJump = bulk.AutoJump;
		state.Duckspam = bulk.Duckspam;
		state.Jumpbug = bulk.Jumpbug;
		state.JumpType = bulk.JumpType;
		state.Lgagst = bulk.Lgagst;
		state.Strafe = bulk.Strafe;
		state.StrafeType = bulk.StrafeType;
		state.StrafeYaw = bulk.StrafeYaw;

		return error;
	}

	ErrorReturn ButtonsInput::ButtonsInputFromInternal(const std::string& line, ButtonsInput& state, _InternalFramebulk& bulk)
	{
		state.Attack1 = bulk.Attack1;
		state.Attack2 = bulk.Attack2;
		state.Duck = bulk.Duck;
		state.Jump = bulk.Jump;
		state.Reload = bulk.Reload;
		state.Sprint = bulk.Sprint;
		state.Use = bulk.Use;
		state.Walk = bulk.Walk;

		return true;
	}

	ErrorReturn AimState::AimStateFromInternal(const std::string& line, AimState& state, _InternalFramebulk& bulk)
	{
		state.AimFrames = bulk.AimFrames;
		state.AimPitch = bulk.AimPitch;
		state.AimSet = bulk.AimSet;
		state.AimYaw = bulk.AimYaw.Yaw;
		state.Auto = bulk.AimYaw.Auto;
		state.Cone = bulk.Cone;

		return true;
	}
} // namespace tas