#include "stdafx.h"

#ifndef OE
#include "..\..\sptlib-wrapper.hpp"
#include "..\..\utils\ent_utils.hpp"
#include "..\..\utils\math.hpp"
#include "..\modules.hpp"
#include "..\modules\ClientDLL.hpp"
#include "..\..\features\shadow.hpp"
#include "..\..\features\playerio.hpp"
#include "overlays.hpp"
#include "portal_camera.hpp"

CameraInformation rearViewMirrorOverlay()
{
	CameraInformation info;

	auto pos = utils::GetPlayerEyePosition();
	auto angles = utils::GetPlayerEyeAngles();

	info.x = pos.x;
	info.y = pos.y;
	info.z = pos.z;
	info.pitch = angles[PITCH];
	info.yaw = utils::NormalizeDeg(angles[YAW] + 180);
	return info;
}

CameraInformation havokViewMirrorOverlay()
{
	CameraInformation info;
	Vector havokpos = GetPlayerHavokPos();

	constexpr float duckOffset = 28;
	constexpr float standingOffset = 64;

	auto ducked = playerio::GetFlagsDucking();
	auto angles = utils::GetPlayerEyeAngles();

	info.x = havokpos.x;
	info.y = havokpos.y;

	if (ducked)
		info.z = havokpos.z + duckOffset;
	else
		info.z = havokpos.z + standingOffset;

	info.pitch = angles[PITCH];
	info.yaw = utils::NormalizeDeg(angles[YAW]);
	return info;
}

CameraInformation sgOverlay()
{
	CameraInformation info;
	Vector pos;
	QAngle va;

	calculateSGPosition(pos, va);
	info.x = pos.x;
	info.y = pos.y;
	info.z = pos.z;
	info.pitch = va[PITCH];
	info.yaw = utils::NormalizeDeg(va[YAW]);

	return info;
}

CameraInformation agOverlay()
{
	CameraInformation info;
	Vector pos;
	QAngle va;

	calculateAGPosition(pos, va);
	info.x = pos.x;
	info.y = pos.y;
	info.z = pos.z;
	info.pitch = va[PITCH];
	info.yaw = utils::NormalizeDeg(va[YAW]);

	return info;
}
#endif