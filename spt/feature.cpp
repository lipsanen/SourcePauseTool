#include "stdafx.h"
#include <future>
#include "feature.hpp"
#include "SPTLib\sptlib.hpp"
#include "dbg.h"
#include "SPTLib\Windows\detoursutils.hpp"

static ModuleHookData moduleHookData[HOOKED_MODULE_COUNT];
static std::unordered_map<uintptr_t, int> patternIndices;

static std::vector<Feature*>& GetFeatures()
{
	static std::vector<Feature*> features;
	return features;
}

void Feature::LoadFeatures()
{
	for (auto feature : GetFeatures())
	{
		if (!feature->moduleLoaded && feature->ShouldLoadFeature())
		{
			feature->InitHooks();
		}
	}

	Hook();

	for (auto feature : GetFeatures())
	{
		if (!feature->moduleLoaded && feature->ShouldLoadFeature())
		{
			feature->LoadFeature();
			feature->moduleLoaded = true;
		}
	}
}

void Feature::UnloadFeatures()
{
	for (auto feature : GetFeatures())
	{
		if (feature->moduleLoaded)
		{
			feature->UnloadFeature();
			feature->moduleLoaded = false;
		}
	}

	Unhook();
}

void Feature::AddVFTableHook(VFTableHook hook, ModuleEnum moduleEnum)
{
	auto& mhd = moduleHookData[static_cast<int>(moduleEnum)];
	mhd.vftableHooks.push_back(hook);
}

Feature::Feature()
{
	moduleLoaded = false;
	GetFeatures().push_back(this);
}

void Feature::Hook()
{
	moduleHookData[static_cast<int>(ModuleEnum::client)].HookModule(L"client.dll");
	moduleHookData[static_cast<int>(ModuleEnum::engine)].HookModule(L"engine.dll");
	moduleHookData[static_cast<int>(ModuleEnum::inputsystem)].HookModule(L"inputsystem.dll");
	moduleHookData[static_cast<int>(ModuleEnum::server)].HookModule(L"server.dll");
	moduleHookData[static_cast<int>(ModuleEnum::vguimatsurface)].HookModule(L"vguimatsurface.dll");
	moduleHookData[static_cast<int>(ModuleEnum::vphysics)].HookModule(L"vphysics.dll");
}

void Feature::Unhook()
{
	moduleHookData[static_cast<int>(ModuleEnum::client)].UnhookModule(L"client.dll");
	moduleHookData[static_cast<int>(ModuleEnum::engine)].UnhookModule(L"engine.dll");
	moduleHookData[static_cast<int>(ModuleEnum::inputsystem)].UnhookModule(L"inputsystem.dll");
	moduleHookData[static_cast<int>(ModuleEnum::server)].UnhookModule(L"server.dll");
	moduleHookData[static_cast<int>(ModuleEnum::vguimatsurface)].UnhookModule(L"vguimatsurface.dll");
	moduleHookData[static_cast<int>(ModuleEnum::vphysics)].UnhookModule(L"vphysics.dll");
}

void Feature::AddOffsetHook(ModuleEnum moduleEnum,
                            int offset,
                            const char* patternName,
                            void** origPtr,
                            void* functionHook)
{
	auto& mhd = moduleHookData[static_cast<int>(moduleEnum)];
	mhd.offsetHooks.push_back(OffsetHook{offset, patternName, origPtr, functionHook});
}

int Feature::GetPatternIndex(void **origPtr) {
  uintptr_t ptr = reinterpret_cast<uintptr_t>(origPtr);
  if (patternIndices.find(ptr) != patternIndices.end()) {
    return patternIndices[ptr];
  } else {
    return -1;
  }
}

void Feature::AddRawHook(ModuleEnum moduleName, void** origPtr, void* functionHook)
{
	auto& hookData = moduleHookData[static_cast<int>(moduleName)];
	hookData.funcPairs.emplace_back(origPtr, functionHook);
	hookData.hookedFunctions.emplace_back(origPtr);
}

void Feature::AddPatternHook(PatternHook hook, ModuleEnum moduleEnum)
{
	auto& mhd = moduleHookData[static_cast<int>(moduleEnum)];
	mhd.patternHooks.push_back(hook);
}

void ModuleHookData::UnhookModule(const std::wstring& moduleName)
{
	DetoursUtils::DetachDetours(moduleName, hookedFunctions.size(), &hookedFunctions[0]);

	for (auto& vft_hook : existingVTableHooks)
		MemUtils::HookVTable(vft_hook.vftable, vft_hook.index, *vft_hook.origPtr);
}

void ModuleHookData::HookModule(const std::wstring& moduleName)
{
	void* handle;
	void* moduleStart;
	size_t moduleSize;

	if (MemUtils::GetModuleInfo(moduleName, &handle, &moduleStart, &moduleSize))
	{
		DevMsg("Hooking %s (start: %p; size: %x)...\n", Convert(moduleName).c_str(), moduleStart, moduleSize);
	}
	else
	{
		DevMsg("Couldn't hook %s, not loaded\n", Convert(moduleName).c_str());
		return;
	}

	std::vector<std::future<patterns::PatternWrapper*>> hooks;
	hooks.reserve(patternHooks.size());

	for (auto& pattern : patternHooks)
	{
		hooks.emplace_back(MemUtils::find_unique_sequence_async(*pattern.origPtr,
		                                                        moduleStart,
		                                                        moduleSize,
		                                                        pattern.patternArr,
		                                                        pattern.patternArr + pattern.size));
	}

	funcPairs.reserve(funcPairs.size() + patternHooks.size());
	hookedFunctions.reserve(hookedFunctions.size() + patternHooks.size());

	for (std::size_t i = 0; i < hooks.size(); ++i)
	{
		auto foundPattern = hooks[i].get();
		auto modulePattern = patternHooks[i];

		if (*modulePattern.origPtr)
		{
			if (modulePattern.functionHook)
			{
				funcPairs.emplace_back(modulePattern.origPtr, modulePattern.functionHook);
				hookedFunctions.emplace_back(modulePattern.origPtr);
			}

			DevMsg("[%s] Found %s at %p (using the %s pattern).\n",
			       Convert(moduleName).c_str(),
			       modulePattern.patternName,
			       *modulePattern.origPtr,
			       foundPattern->name());
                        patternIndices[reinterpret_cast<uintptr_t>(
                            modulePattern.origPtr)] =
                            foundPattern - modulePattern.patternArr;
                }
		else
		{
			DevWarning("[%s] Could not find %s.\n", Convert(moduleName).c_str(), modulePattern.patternName);
		}

		if (modulePattern.callback)
		{
			modulePattern.callback(foundPattern, foundPattern - modulePattern.patternArr);
		}
	}

	for (auto& offset : offsetHooks)
	{
		*offset.origPtr = reinterpret_cast<char*>(moduleStart) + offset.offset;

		DevMsg("[%s] Found %s at %p via a fixed offset.\n",
		       Convert(moduleName).c_str(),
		       offset.patternName,
		       *offset.origPtr);

		if (offset.functionHook)
		{
			funcPairs.emplace_back(offset.origPtr, offset.functionHook);
			hookedFunctions.emplace_back(offset.origPtr);
		}
	}

	if (!vftableHooks.empty())
	{
		for (auto& vft_hook : vftableHooks)
			MemUtils::HookVTable(vft_hook.vftable, vft_hook.index, *vft_hook.origPtr);
	}

	if (!funcPairs.empty())
	{
		for (auto& entry : funcPairs)
			MemUtils::MarkAsExecutable(*(entry.first));

		DetoursUtils::AttachDetours(moduleName, funcPairs.size(), &funcPairs[0]);
	}

	// Clear any hooks that were added
	offsetHooks.clear();
	patternHooks.clear();
	// VTable hooks have to be stored for the unhooking code
	existingVTableHooks.insert(existingVTableHooks.end(), vftableHooks.begin(), vftableHooks.end());
	vftableHooks.clear();
}
