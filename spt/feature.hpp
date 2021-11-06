#pragma once
#include <array>
#include <vector>
#include <functional>
#include <unordered_map>
#include "SPTLib\patterns.hpp"
#include "utils\patterncontainer.hpp"
#include "OrangeBox\patterns.hpp"

typedef std::function<void(patterns::PatternWrapper*, int)> _PatternCallback;
enum class ModuleEnum
{
	client = 0,
	engine,
	inputsystem,
	server,
	vguimatsurface,
	vphysics
};
const int HOOKED_MODULE_COUNT = 6;

#define ADD_RAW_HOOK(moduleName, name) \
	AddRawHook(ModuleEnum::moduleName, \
	              reinterpret_cast<void**>(&ORIG_##name##), \
	              reinterpret_cast<void*>(HOOKED_ ##name##));
#define FIND_PATTERN(moduleName, name) \
	AddPatternHook(patterns::##moduleName## ::##name##, \
	               ModuleEnum::moduleName, \
	               #name, \
	               reinterpret_cast<void**>(&ORIG_ ##name##), \
	               nullptr, \
	               nullptr);
#define HOOK_FUNCTION(moduleName, name) \
	AddPatternHook(patterns::##moduleName## ::##name##, \
	               ModuleEnum::moduleName, \
	               #name, \
	               reinterpret_cast<void**>(&ORIG_ ##name##), \
	               reinterpret_cast<void*>(HOOKED_ ##name##), \
	               nullptr);
#define PATTERN_CALLBACK [&](patterns::PatternWrapper * found, int index)
#define FIND_PATTERN_WITH_CALLBACK(moduleName, name, callback) \
	AddPatternHook(patterns::##moduleName## ::##name##, \
	               ModuleEnum::moduleName, \
	               #name, \
	               reinterpret_cast<void**>(&ORIG_ ##name##), \
	               nullptr, \
	               callback);
#define HOOK_FUNCTION_WITH_CALLBACK(moduleName, name, callback) \
	AddPatternHook(patterns::##moduleName## ::##name##, \
	               ModuleEnum::moduleName, \
	               #name, \
	               reinterpret_cast<void**>(&ORIG_ ##name##), \
	               reinterpret_cast<void*>(HOOKED_ ##name##), \
	               callback);

struct PatternHook
{
	PatternHook(patterns::PatternWrapper* patternArr,
	            size_t size,
	            const char* patternName,
	            void** origPtr,
	            void* functionHook,
	            _PatternCallback callback)
	{
		this->patternArr = patternArr;
		this->size = size;
		this->patternName = patternName;
		this->origPtr = origPtr;
		this->functionHook = functionHook;
		this->callback = callback;
	}

	patterns::PatternWrapper* patternArr;
	size_t size;
	const char* patternName;
	void** origPtr;
	void* functionHook;
	_PatternCallback callback;
};

struct OffsetHook
{
	int32_t offset;
	const char* patternName;
	void** origPtr;
	void* functionHook;
};

struct RawHook
{
	const char* patternName;
	void** origPtr;
	void* functionHook;
};

struct ModuleHookData
{
	std::vector<PatternHook> patternHooks;
	std::vector<VFTableHook> vftableHooks;
	std::vector<OffsetHook> offsetHooks;

	std::vector<std::pair<void**, void*>> funcPairs;
	std::vector<void**> hookedFunctions;
	std::vector<VFTableHook> existingVTableHooks;
	void HookModule(const std::wstring& moduleName);
	void UnhookModule(const std::wstring& moduleName);
};

class Feature
{
public:
	virtual bool ShouldLoadFeature() = 0;
	virtual void InitHooks() = 0;
	virtual void LoadFeature() = 0;
	virtual void UnloadFeature() = 0;

	static void LoadFeatures();
	static void UnloadFeatures();

	template<size_t PatternLength>
	static void AddPatternHook(const std::array<patterns::PatternWrapper, PatternLength>& patterns,
	                           ModuleEnum moduleName,
	                           const char* patternName,
	                           void** origPtr = nullptr,
	                           void* functionHook = nullptr,
	                           _PatternCallback callback = nullptr);
	static void AddRawHook(ModuleEnum moduleName, void** origPtr, void* functionHook);
	static void AddPatternHook(PatternHook hook, ModuleEnum moduleEnum);
	static void AddVFTableHook(VFTableHook hook, ModuleEnum moduleEnum);
	static void AddOffsetHook(ModuleEnum moduleEnum,
	                          int offset,
	                          const char* patternName,
	                          void** origPtr = nullptr,
	                          void* functionHook = nullptr);
	static int GetPatternIndex(void** origPtr);

	Feature();

private:
	static void Hook();
	static void Unhook();

	bool moduleLoaded;
};

template<size_t PatternLength>
inline void Feature::AddPatternHook(const std::array<patterns::PatternWrapper, PatternLength>& p,
                                    ModuleEnum moduleEnum,
                                    const char* patternName,
                                    void** origPtr,
                                    void* functionHook,
                                    _PatternCallback callback)
{
	AddPatternHook(PatternHook(const_cast<patterns::PatternWrapper*>(p.data()),
	                           PatternLength,
	                           patternName,
	                           origPtr,
	                           functionHook,
	                           callback),
	               moduleEnum);
}
