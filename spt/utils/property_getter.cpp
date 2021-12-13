#include "stdafx.h"
#include "property_getter.hpp"
#include <vector>
#include "ent_utils.hpp"

#ifdef OE
#define GAME_DLL
#include "..\dlls\cbase.h"
#include "..\game_shared\physics_shared.h"
#else
#define GAME_DLL
#include "cbase.h"
#endif

namespace utils
{
	static std::unordered_map<std::string, PropMap> classToOffsetsMap;
	static std::unordered_map<std::string, DataMap> nameToDatamap;

	PropMap FindOffsets(IClientEntity* ent)
	{
		PropMap out;
		if (!ent)
		{
			out.foundOffsets = false;
			return out;
		}

		auto clientClass = ent->GetClientClass();
		std::vector<utils::propValue> props;
		utils::GetAllProps(clientClass->m_pRecvTable, ent, props);

		for (auto prop1 : props)
		{
			out.props[prop1.name] = prop1.prop;
		}

		out.foundOffsets = true;
		return out;
	}

	DataMap FindFieldOffsets(CBaseEntity* ent)
	{
		DataMap out;
		if (!ent)
		{
			out.foundOffsets = false;
			return out;
		}

		auto datamap = ent->GetDataDescMap();

		while (datamap != nullptr)
		{
			for (int i = 0; i < datamap->dataNumFields; ++i)
			{
				auto* field = &datamap->dataDesc[i];
				if (field->fieldOffset[0] > 0)
				{
					out.typedescriptions[field->fieldName] = field;
				}
			}

			datamap = datamap->baseMap;
		}

		out.foundOffsets = true;
		return out;
	}

	int GetOffset(int entindex, const std::string& key)
	{
		auto prop = GetRecvProp(entindex, key);
		if (prop)
			return prop->GetOffset();
		else
			return INVALID_OFFSET;
	}

	int GetDatamapOffset(int entindex, const std::string& key)
	{
		auto prop = GetTypeDescription(entindex, key);
		if (prop)
			return prop->fieldOffset[0];
		else
			return INVALID_OFFSET;
	}

	typedescription_t* GetTypeDescription(int index, const std::string& key)
	{
		auto ent = utils::GetServerEntity(index);
		auto datamap = ent->GetDataDescMap();
		std::string name = datamap->dataClassName;

		if (nameToDatamap.find(name) == nameToDatamap.end())
		{
			auto map = FindFieldOffsets(ent);
			if (!map.foundOffsets)
				return nullptr;
			nameToDatamap[name] = map;
		}

		auto& typeMap = nameToDatamap[name].typedescriptions;

		if (typeMap.find(key) != typeMap.end())
			return typeMap[key];
		else
			return nullptr;
	}

	RecvProp* GetRecvProp(int entindex, const std::string& key)
	{
		auto ent = GetClientEntity(entindex);
		std::string className = ent->GetClientClass()->m_pNetworkName;

		if (classToOffsetsMap.find(className) == classToOffsetsMap.end())
		{
			auto map = FindOffsets(ent);
			if (!map.foundOffsets)
				return nullptr;
			classToOffsetsMap[className] = map;
		}

		auto& classMap = classToOffsetsMap[className].props;

		if (classMap.find(key) != classMap.end())
			return classMap[key];
		else
			return nullptr;
	}
} // namespace utils