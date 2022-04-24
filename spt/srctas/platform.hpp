#pragma once

#ifdef __GNUC__
#define DLL_EXPORT
#else
#define DLL_EXPORT __declspec(dllexport)
#endif
