#include "tier0/dbg.h"
#include "Color.h"
#include <cstdio>
#include <cstdarg>

static void color_wrapper(int level, const Color& c, const char* msg, ...) {
	va_list a_list;
	va_start(a_list, msg);
	vprintf(msg, a_list);
	va_end(a_list);
}

static void printf_wrapper(const char* msg, ...) {
	va_list a_list;
	va_start(a_list, msg);
	vprintf(msg, a_list);
	va_end(a_list);
}


ColorFunc ConColorMsg = color_wrapper;
PrintFunc ConMsg = printf_wrapper;
PrintFunc Msg = printf_wrapper;
PrintFunc Warning = printf_wrapper;
PrintFunc DevMsg = printf_wrapper;
PrintFunc DevWarning = printf_wrapper;
