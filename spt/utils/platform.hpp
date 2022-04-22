#pragma once

#   define THISCALL_MEMBER(type, name, ...)                            \
		using _##name = type(__fastcall *)(void *thisptr, int edx, ##__VA_ARGS__); \
		_##name ORIG_ ## name = nullptr;

#	define THISCALL_DETOUR(type, name, ...) THISCALL_MEMBER(type, name, ##__VA_ARGS__) \
		static type __cdecl HOOKED_ ## name (void *thisptr, int edx, ##__VA_ARGS__)

#	define THISCALL_HOOK(type, className, name, ...) \
		type __cdecl className::HOOKED_ ## name (void *thisptr, int edx, ##__VA_ARGS__)
    
#   define CDECL_MEMBER(type, name, ...)                            \
		using _##name = type(__cdecl *)(##__VA_ARGS__); \
		_##name ORIG_ ## name = nullptr;

#	define  CDECL_DETOUR(type, name, ...) CDECL_MEMBER(type, name, ##__VA_ARGS__) \
		static type __cdecl HOOKED_ ## name (__VA_ARGS__)

#	define CDECL_HOOK(type, className, name, ...) \
		type __cdecl className::HOOKED_ ## name (__VA_ARGS__)
