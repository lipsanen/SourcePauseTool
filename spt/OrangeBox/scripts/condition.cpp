#include "stdafx.h"

#include "condition.hpp"

#include "..\..\utils\ent_utils.hpp"
#include "..\..\utils\math.hpp"
#include "..\..\utils\property_getter.hpp"
#include "..\modules.hpp"
#include "..\modules\ClientDLL.hpp"

namespace scripts
{
	TickRangeCondition::TickRangeCondition(int low, int high, bool reverse)
	    : lowTick(low), highTick(high), reverse(reverse)
	{
		if (low >= high)
			throw std::exception("Low tick should be lower than high");
	}

	bool TickRangeCondition::IsTrue(int tick, int totalTicks) const
	{
		int value;

		if (reverse)
			value = totalTicks - tick;
		else
			value = tick;

		return value >= lowTick && value <= highTick;
	}

	bool TickRangeCondition::ShouldTerminate(int tick, int totalTicks) const
	{
		if (reverse)
			return (totalTicks - tick) < lowTick;
		else
			return tick > highTick;
	}

	PosSpeedCondition::PosSpeedCondition(float low, float high, Axis axis, bool isPos)
	    : low(low), high(high), axis(axis), isPos(isPos)
	{
		if (low >= high)
			throw std::exception("Low value should be lower than high");
	}

	bool PosSpeedCondition::IsTrue(int tick, int totalTicks) const
	{
		if (!utils::playerEntityAvailable())
			return false;

		Vector v;

		if (isPos)
			v = clientDLL.GetPlayerEyePos();
		else
			v = clientDLL.GetPlayerVelocity();

		float val;

		switch (axis)
		{
		case Axis::AxisX:
			val = v.x;
			break;
		case Axis::AxisY:
			val = v.y;
			break;
		case Axis::AxisZ:
			val = v.z;
			break;
		case Axis::TwoD:
			val = v.Length2D();
			break;
		case Axis::Abs:
			val = v.Length();
			break;
		default:
			throw std::exception("Unknown type for speed/position condition");
		}

		return val >= low && val <= high;
	}

	bool PosSpeedCondition::ShouldTerminate(int tick, int totalTicks) const
	{
		return false;
	}

	JBCondition::JBCondition(float z) : height(z) {}

	bool JBCondition::IsTrue(int tick, int totalTicks) const
	{
		return utils::CanJB(height).canJB;
	}

	bool JBCondition::ShouldTerminate(int tick, int totalTicks) const
	{
		return false;
	}

	AliveCondition::AliveCondition() {}

	bool AliveCondition::IsTrue(int tick, int totalTicks) const
	{
		return !utils::playerEntityAvailable() || utils::GetProperty<int>(0, "m_iHealth") > 0;
	}

	bool AliveCondition::ShouldTerminate(int tick, int totalTicks) const
	{
		return utils::playerEntityAvailable() && utils::GetProperty<int>(0, "m_iHealth") <= 0;
	}

	LoadCondition::LoadCondition() {}

	bool LoadCondition::IsTrue(int tick, int totalTicks) const
	{
		return !utils::playerEntityAvailable();
	}

	bool LoadCondition::ShouldTerminate(int tick, int totalTicks) const
	{
		return false;
	}

	VelAngleCondition::VelAngleCondition(float low, float high, AngleAxis axis) : low(low), high(high), axis(axis)
	{
		if (NormalizeDeg(high - low) < 0)
			throw std::exception("Low should be lower than high");
	}

	bool VelAngleCondition::IsTrue(int tick, int totalTicks) const
	{
		if (!utils::playerEntityAvailable())
			return false;

		Vector v = clientDLL.GetPlayerVelocity();
		QAngle angles;
		VectorAngles(v, Vector(0, 0, 1), angles);
		float f;

		if (axis == AngleAxis::Pitch)
			f = angles.x;
		else
			f = angles.y;

		return NormalizeDeg(f - low) >= 0 && NormalizeDeg(f - high) <= 0;
	}

	bool VelAngleCondition::ShouldTerminate(int tick, int totalTicks) const
	{
		return false;
	}

	template<typename T>
	static TokenList GetListFromVector(int count)
	{
		TokenList out;
		for (int i = 0; i < count; ++i)
			out.push(0.0);
		return out;
	}

	TokenMap SYMBOLS;

	IfCondition::IfCondition(const std::string& expressionString)
	{
		try
		{
			this->expression = calculator(expressionString.c_str());
			this->expression.eval(SYMBOLS)
			    .asBool(); // Test run to detect any errors with mistyped variable names
		}
		catch (std::exception& what)
		{
			static std::string error;
			std::ostringstream oss;
			oss << "error parsing if condition, " << what.what();
			error = oss.str();
			throw std::exception(error.c_str());
		}
	}

	packToken canjb(TokenMap scope)
	{
		// Get the argument list:
		TokenList list = scope["args"].asList();
		auto height = list.operator[](0).asDouble();
		return utils::CanJB(height).canJB;
	}

	bool IfCondition::IsTrue(int tick, int totalTicks) const
	{
		return expression.eval(SYMBOLS).asBool();
	}

	bool IfCondition::ShouldTerminate(int tick, int totalTicks) const
	{
		return false;
	}

	template<typename T>
	static void CopyVectorIntoList(const char* identifier, T* vec, int count)
	{
		auto list = SYMBOLS[identifier].asList();
		for (int i = 0; i < count; ++i)
			list.operator[](i) = vec[i];
	}

	void UpdateSymbolTable()
	{
		auto vel = clientDLL.GetPlayerVelocity();
		auto pos = clientDLL.GetPlayerEyePos();
		QAngle angles;
		VectorAngles(vel, Vector(0, 0, 1), angles);

		CopyVectorIntoList("vel", reinterpret_cast<float*>(&vel), 3);
		CopyVectorIntoList("velang", reinterpret_cast<float*>(&angles), 3);
		CopyVectorIntoList("pos", reinterpret_cast<float*>(&pos), 3);
		SYMBOLS["hp"] = utils::GetProperty<int>(0, "m_iHealth");
		SYMBOLS["vel2d"] = vel.Length2D();
		SYMBOLS["loading"] = !utils::playerEntityAvailable();
	}

	void InitConditions()
	{
		cparse_startup();
		SYMBOLS["vel"] = GetListFromVector<double>(3);
		SYMBOLS["velang"] = GetListFromVector<double>(3);
		SYMBOLS["pos"] = GetListFromVector<double>(3);
		SYMBOLS["hp"] = 0;
		SYMBOLS["vel2d"] = 0;
		SYMBOLS["canjb"] = CppFunction(&canjb);
		SYMBOLS["loading"] = true;
	}
} // namespace scripts
