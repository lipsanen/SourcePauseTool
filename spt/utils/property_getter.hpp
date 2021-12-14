#pragma once

#include <map>
#include <string>
#include <unordered_map>
#include "cdll_int.h"
#include "engine\ivmodelinfo.h"
#include "client_class.h"
#include "datamap.h"
#include "icliententity.h"
#include "icliententitylist.h"
#include "ent_utils.hpp"

namespace utils
{
	const int INVALID_OFFSET = -1;

	struct PropMap
	{
		std::map<std::string, RecvProp*> props;
		bool foundOffsets;
	};

	struct DataMap
	{
		std::unordered_map<std::string, typedescription_t*> typedescriptions;
		bool foundOffsets;
	};

	datamap_t* GetDataDescMap(void* entity);
	int GetOffset(int entindex, const std::string& key);
	int GetDatamapOffset(int entindex, const std::string& key);
	typedescription_t* GetTypeDescription(int index, const std::string& key);
	RecvProp* GetRecvProp(int entindex, const std::string& key);

	template<typename T>
	T GetProperty(int entindex, const std::string& key)
	{
		auto ent = GetClientEntity(entindex);

		if (!ent)
			return T();

		int offset = GetOffset(entindex, key);
		if (offset == INVALID_OFFSET)
			return T();
		else
		{
			return *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(ent) + offset);
		}
	}

	template<typename T>
	T GetField(int entindex, const std::string& key)
	{
		auto ent = utils::GetServerEntity(entindex);

		if (!ent)
			return T();

		int offset = GetDatamapOffset(entindex, key);
		if (offset == INVALID_OFFSET)
			return T();
		else
		{
			return *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(ent) + offset);
		}
	}

	template<typename T>
	T GetPlayerDatamapProperty(const std::string& key)
	{
		auto ent = GetServerEntity(1);

		if (!ent)
			return T();

		int offset = GetDatamapOffset(1, key);
		if (offset == INVALID_OFFSET)
			return T();
		else
		{
			return *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(ent) + offset);
		}
	}

	template<typename T>
	T* GetPlayerDatamapPtr(const std::string& key)
	{
		auto ent = GetServerEntity(1);

		if (!ent)
			return nullptr;

		int offset = GetDatamapOffset(1, key);
		if (offset == INVALID_OFFSET)
			return nullptr;
		else
		{
			return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(ent) + offset);
		}
	}

} // namespace utils