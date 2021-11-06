#pragma once

#include "modules\ClientDLL.hpp"
#include "modules\ServerDLL.hpp"

extern ClientDLL clientDLL;
extern ServerDLL serverDLL;

#ifdef TRACE
#define TRACE_MSG(X) DevMsg(X)
#else
#define TRACE_MSG(X)
#endif

#define TRACE_ENTER() TRACE_MSG("ENTER: " __FUNCTION__ "\n")
#define TRACE_EXIT() TRACE_MSG("EXIT: " __FUNCTION__ "\n")
