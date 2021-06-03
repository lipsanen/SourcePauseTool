#pragma once

#include "..\OrangeBox\spt-serverplugin.hpp"

// clang-format off

namespace fcps {
	bool FindClosestPassableSpaceOverride(CBaseEntity *pEntity, const Vector &vIndecisivePush, unsigned int fMask);
}
