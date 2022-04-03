#if defined(__GNUC__)
#define __cdecl __attribute__((cdecl))
#define __fastcall __attribute__((fastcall))
#define __stdcall
#endif