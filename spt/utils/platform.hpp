#pragma once

#ifdef _WIN32
// TODO: implement for windows
#else
#   define DECL_MEMBER(type, name, ...)                            \
		using _##name = type(__cdecl *)(void *thisptr, ##__VA_ARGS__); \
		_##name ORIG_ ## name = nullptr;

#	define DECL_DETOUR(type, name, ...) DECL_MEMBER(type, name, ##__VA_ARGS__) \
		static type __cdecl HOOKED_ ## name (void *thisptr, ##__VA_ARGS__)

#	define DETOUR(type, className, name, ...) \
		type __cdecl className::HOOKED_ ## name (void *thisptr, ##__VA_ARGS__)

template<typename T>
int GetVTableIndex(T ptr)
{
    int value = *reinterpret_cast<int*>(&ptr);
    return (value - 1) / sizeof(int*);
}

#   define FIND_VTABLE(module, instance, ptr_to_method, name) AddVFTableHook( \
    VFTableHook(reinterpret_cast<void***>(instance), \
    GetVTableIndex(ptr_to_method), \
    nullptr, \
    (void**)&ORIG_ ## name), \
	#module);

#   define HOOK_VTABLE(module, instance, ptr_to_method, name) AddVFTableHook( \
    VFTableHook(reinterpret_cast<void***>(instance), \
    GetVTableIndex(ptr_to_method), \
    (void*)HOOKED_ ## name, \
    (void**)&ORIG_ ## name), \
	#module);

#endif