#include "stdafx.h"
#include "dlfcn.h"
#include "link.h"
#include "string.h"
#include <array>
#include <future>
#include <vector>
#include "SPTLib/MemUtils.hpp"

namespace platform
{
	typedef void* (*CreateInterfaceFn)(const char *pName, int *pReturnCode);

	void* GetInterface(const char *filename, const char *interfaceSymbol) {
		void* handle = nullptr;
		MemUtils::GetModuleInfo(L"client.so", &handle, nullptr, nullptr);
		if (!handle) {
			EngineDevWarning("Failed to open module %s!\n", filename);
			return nullptr;
		}

		CreateInterfaceFn CreateInterface = (CreateInterfaceFn)dlsym(handle, "CreateInterface");

		if (!CreateInterface) {
			EngineDevWarning("Failed to find symbol CreateInterface for %s!\n", filename);
			return nullptr;
		}

		int ret;
		void *fn = CreateInterface(interfaceSymbol, &ret);

		if (ret) {
			EngineDevWarning("SAR: Failed to find interface with symbol %s in %s!\n", interfaceSymbol, filename);
			return nullptr;
		}

		return fn;
	}
}
