#pragma once
#include "spt/sptlib-wrapper.hpp"

#if defined(__GNUC__)
#define __cdecl __attribute__((cdecl))
#define __fastcall __attribute__((fastcall))
#define __stdcall
#define DLL_EXT ".so"
#endif