#include "stdafx.h"

#include <vector>

#include "ljstats.hpp"
#include "OrangeBox\modules.hpp"
#include "utils\math.hpp"
#include "OrangeBox\cvars.hpp"

namespace ljstats
{
	static Vector jumpSpot;
	static Vector currentVelocity;
	static Vector previousPos;
	static AccelDirection prevAccelDir;
	static SegmentStats currentSegment;
	static JumpStats currentJump;
	JumpStats lastJump;

	static bool inJump;
	static bool firstTick;
	static int groundTicks = 0;
	const float EPS = 0.001f;
	const int MIN_GROUND_TICKS = 10;

	AccelDirection GetDirection()
	{
		Vector newVel = clientDLL.GetPlayerVelocity();
		Vector delta = newVel - currentVelocity;

		if (delta.Length2D() < EPS)
		{
			return AccelDirection::Forward;
		}

		QAngle oldAngle, newAngle;
		VectorAngles(currentVelocity, oldAngle);
		VectorAngles(newVel, newAngle);

		float yaw = static_cast<float>(utils::NormalizeDeg(newAngle.y - oldAngle.y));

		if (yaw < 0)
		{
			return AccelDirection::Right;
		}
		else if (yaw == 0)
		{
			return AccelDirection::Forward;
		}
		else
		{
			return AccelDirection::Left;
		}
	}

	void EndSegment()
	{
		currentJump.segments.push_back(currentSegment);
		currentSegment.Reset();
	}

	void EndJump()
	{
		inJump = false;
		currentJump.endSpot = clientDLL.GetPlayerAbsPos();
		EndSegment();
		lastJump = currentJump;
	}

	void OnJump()
	{
		if (!y_spt_hud_ljstats.GetBool() || groundTicks < MIN_GROUND_TICKS)
			return;

		groundTicks = 0;
		jumpSpot = previousPos = clientDLL.GetPlayerAbsPos();
		prevAccelDir = AccelDirection::Forward;
		currentJump.Reset();
		inJump = true;
		firstTick = true;
	}

	void CollectStats(bool last)
	{
		float distance = (clientDLL.GetPlayerAbsPos() - previousPos).Length2D();

		if(firstTick)
		{
			currentVelocity = clientDLL.GetPlayerVelocity();
			currentSegment.Reset();
			currentJump.startVel = currentVelocity.Length2D();
			firstTick = false;
		}
		else
		{
			Vector newVel = clientDLL.GetPlayerVelocity();
			AccelDirection direction = GetDirection();

			if (direction != currentSegment.dir && direction != AccelDirection::Forward)
			{
				EndSegment();
				currentSegment.dir = direction;
			}

			float accel = newVel.Length2D() - currentVelocity.Length2D();
			currentSegment.totalAccel += accel;


			if (accel > EPS)
			{
				currentSegment.positiveAccel += accel;
				++currentSegment.gainTicks;
				++currentSegment.accelTicks;
			}
			else if (accel < -EPS)
			{
				currentSegment.negativeAccel -= accel;
				++currentSegment.accelTicks;
			}

			++currentSegment.ticks;
			if (!last)
			{
				currentVelocity = newVel;
				prevAccelDir = direction;
			}
		}

		currentSegment.distanceCovered += distance;
		previousPos = clientDLL.GetPlayerAbsPos();
	}

	void OnTick()
	{
		bool onground = clientDLL.OnGround();

		if (onground)
		{
			++groundTicks;
		}

		if (!inJump)
			return;

		Vector pos = clientDLL.GetPlayerAbsPos();
		bool last = pos.z <= jumpSpot.z || onground;

		CollectStats(last);

		if (last)
		{
			EndJump();
			return;
		}

	}

	void SegmentStats::Reset()
	{
		ticks = 0;
		gainTicks = 0;
		accelTicks = 0;
		dir = AccelDirection::Forward;
		totalAccel = 0;
		positiveAccel = 0;
		negativeAccel = 0;
		distanceCovered = 0;
		startVel = clientDLL.GetPlayerVelocity().Length2D();
	}

	float SegmentStats::Sync()
	{
		return (static_cast<float>(gainTicks) / accelTicks) * 100;
	}

	int JumpStats::StrafeCount()
	{
		int count = 0;
		for (auto& strafe : segments) {
			if (strafe.dir != AccelDirection::Forward) {
				++count;
			}
		}

		return count;
	}

	int JumpStats::TotalTicks()
	{
		int ticks = 0;
		for (auto& strafe : segments) {
			ticks += strafe.ticks;
		}
		return ticks;
	}

	float JumpStats::Sync()
	{
		int gained = 0;
		int total = 0;

		for (auto& segment : segments) {
			gained += segment.gainTicks;
			total += segment.accelTicks;
		}

		if (total == 0)
		{
			return 0;
		}
		else
		{
			return (static_cast<float>(gained) / total) * 100;
		}
	}

	float JumpStats::AccelPerTick()
	{
		float accel = 0;
		int accelTicks = 0;

		for (auto& segment : segments) {
			accel += segment.totalAccel;
			accelTicks += segment.accelTicks;
		}

		if (accelTicks == 0)
		{
			return 0;
		}
		else
		{
			return accel / accelTicks;
		}
	}

	float JumpStats::TotalAccel()
	{
		float accel = 0;

		for (auto& segment : segments) {
			accel += segment.totalAccel;
		}

		return accel;
	}

	float JumpStats::NegativeAccel()
	{
		float accel = 0;

		for (auto& segment : segments) {
			accel += segment.negativeAccel;
		}

		return accel;
	}

	float JumpStats::PositiveAccel()
	{
		float accel = 0;

		for (auto& segment : segments) {
			accel += segment.positiveAccel;
		}

		return accel;
	}

	float JumpStats::TimeAccelerating()
	{
		int accelTicks = 0;
		int totalTicks = 0;

		for (auto& segment : segments) {
			accelTicks += segment.accelTicks;
			totalTicks += segment.ticks;
		}

		if (totalTicks == 0)
		{
			return 0;
		}
		else
		{
			return static_cast<double>(accelTicks) / totalTicks * 100;
		}
	}

	float JumpStats::Length()
	{
		Vector delta = endSpot - jumpSpot;
		const float COLLISION_HULL_WIDTH = 32.0f;

		return delta.Length2D() + COLLISION_HULL_WIDTH;
	}

	float JumpStats::Prestrafe()
	{
		return startVel;
	}

	float JumpStats::Straightness()
	{
		float totalDistance = 0;
		Vector delta = endSpot - jumpSpot;
		float length = delta.Length2D();

		for (auto& segment : segments) {
			totalDistance += segment.distanceCovered;
		}
		
		if (totalDistance < EPS) {
			return 0;
		}
		else {
			return length / totalDistance * 100;
		}
	}

	void JumpStats::Reset()
	{
		endSpot.Zero();
		jumpSpot = clientDLL.GetPlayerAbsPos();
		segments.clear();
		startVel = 0.0f;
	}
} // namespace ljstats
