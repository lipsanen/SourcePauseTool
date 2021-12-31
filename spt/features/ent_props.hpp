#pragma once

#include "..\feature.hpp"
#include "datamap_wrapper.hpp"

enum class PropMode
{
	Server,
	Client,
	None
};

struct _InternalPlayerField
{
	int serverOffset = utils::INVALID_DATAMAP_OFFSET;
	int clientOffset = utils::INVALID_DATAMAP_OFFSET;
	;
	bool preferServer = true;

	template<typename T>
	T GetValue() const;
	template<typename T>
	T* GetPtr();
	bool ClientOffsetFound()
	{
		return clientOffset != utils::INVALID_DATAMAP_OFFSET;
	}
	bool ServerOffsetFound()
	{
		return serverOffset != utils::INVALID_DATAMAP_OFFSET;
	}
};

template<typename T>
struct PlayerField
{
	_InternalPlayerField field;

	T GetValue() const
	{
		return field.GetValue<T>();
	}
	T* GetPtr()
	{
		return field.GetPtr<T>();
	}
	bool ClientOffsetFound()
	{
		return field.clientOffset != utils::INVALID_DATAMAP_OFFSET;
	}
	bool ServerOffsetFound()
	{
		return field.serverOffset != utils::INVALID_DATAMAP_OFFSET;
	}
	bool Found()
	{
		return ClientOffsetFound() || ServerOffsetFound();
	}
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
	PlayerField<T> GetPlayerField(const std::string& key,
	                              bool getServer = true,
	                              bool getClient = true,
	                              bool preferServer = true);
	int GetFieldOffset(const std::string& mapKey, const std::string& key, bool server);

protected:
	bool tablesProcessed = false;
	bool playerDatamapSearched = false;
	utils::DatamapWrapper* __playerdatamap = nullptr;

	PropMode GetAutoMode();
	void AddMap(datamap_t* map, bool server);
	_InternalPlayerField _GetPlayerField(const std::string& key, bool getServer, bool getClient, bool preferServer);
	utils::DatamapWrapper* GetDatamapWrapper(const std::string& key);
	utils::DatamapWrapper* GetPlayerDatamapWrapper();
	void ProcessTablesLazy();
	std::vector<patterns::MatchedPattern> serverPatterns;
	std::vector<patterns::MatchedPattern> clientPatterns;
	std::vector<utils::DatamapWrapper*> wrappers;
	std::unordered_map<std::string, utils::DatamapWrapper*> nameToMapWrapper;
};

extern EntUtils spt_entutils;

template<typename T>
inline T _InternalPlayerField::GetValue() const
{
	void* svplayer = spt_entutils.GetPlayer(true);
	void* clplayer = spt_entutils.GetPlayer(false);
	if (preferServer)
	{
		if (svplayer)
		{
			return *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(svplayer) + serverOffset);
		}
		else if (clplayer)
		{
			return *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(clplayer) + clientOffset);
		}
	}
	else
	{
		if (clplayer)
		{
			return *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(clplayer) + clientOffset);
		}
		else if (svplayer)
		{
			return *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(svplayer) + serverOffset);
		}
	}

	return T();
}

template<typename T>
inline T* _InternalPlayerField::GetPtr()
{
	void* svplayer = GetPlayer(true);
	void* clplayer = GetPlayer(false);

	if (preferServer)
	{
		if (svplayer && serverOffset != utils::INVALID_DATAMAP_OFFSET)
		{
			return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(serverOffset) + serverOffset);
		}
		else if (clplayer && serverOffset != utils::INVALID_DATAMAP_OFFSET)
		{
			return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(clientOffset) + clientOffset);
		}
	}
	else
	{
		if (clplayer && serverOffset != utils::INVALID_DATAMAP_OFFSET)
		{
			return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(serverOffset) + serverOffset);
		}
		else if (svplayer && serverOffset != utils::INVALID_DATAMAP_OFFSET)
		{
			return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(clientOffset) + clientOffset);
		}
	}

	return nullptr;
}

template<typename T>
inline PlayerField<T> EntUtils::GetPlayerField(const std::string& key,
                                               bool getServer,
                                               bool getClient,
                                               bool preferServer)
{
	PlayerField<T> field;
	field.field = _GetPlayerField(key, getServer, getClient, preferServer);
	return field;
}
