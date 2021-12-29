#pragma once

#include "..\feature.hpp"
#include "datamap_wrapper.hpp"

enum class PropMode
{
	Server,
	Client,
	None
};

// Initializes ent utils stuff
class EntUtils : public Feature
{
public:
	virtual bool ShouldLoadFeature()
	{
		return true;
	}

	virtual void LoadFeature() override;
	virtual void InitHooks() override;
	virtual void PreHook() override;
	virtual void UnloadFeature() override;

	void WalkDatamap(std::string key);
	void PrintDatamaps();
	int GetPlayerOffset(const std::string& key, bool server);
	void* GetPlayer(bool server);
	template<typename T>
	T GetPlayerProp(int offset, bool server);
	template<typename T>
	T GetPlayerProp(const std::string& key, bool server);
	template<typename T>
	T GetPlayerAutoProp(const std::string& key);

protected:
	bool tablesProcessed = false;
	bool playerDatamapSearched = false;
	utils::DatamapWrapper* __playerdatamap = nullptr;

	PropMode GetAutoMode();
	void AddMap(datamap_t* map, bool server);
	utils::DatamapWrapper* GetDatamapWrapper(const std::string& key);
	utils::DatamapWrapper* GetPlayerDatamapWrapper();
	void ProcessTablesLazy();
	std::vector<uintptr_t> serverPatterns;
	std::vector<uintptr_t> clientPatterns;
	std::vector<utils::DatamapWrapper*> wrappers;

	std::unordered_map<std::string, utils::DatamapWrapper*> nameToMapWrapper;
};

extern EntUtils spt_entutils;

template<typename T>
inline T EntUtils::GetPlayerProp(int offset, bool server)
{
	void* player = GetPlayer(server);
	if (!player)
		return T();
	else
		return *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(player) + offset);
}

template<typename T>
inline T EntUtils::GetPlayerProp(const std::string& key, bool server)
{
	int offset = GetPlayerOffset(key, server);
	if (offset == utils::INVALID_DATAMAP_OFFSET)
		return T();
	else
		return GetPlayerProp(offset);
}

template<typename T>
inline T EntUtils::GetPlayerAutoProp(const std::string& key)
{
	auto mode = GetAutoMode();
	if (mode == PropMode::Server)
		return GetPlayerServerProp(key);
	else if (mode == PropMode::Client)
		return GetPlayerClientProp(key);
	else
		return T();
}
