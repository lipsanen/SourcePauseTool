#pragma once

#include "convar.h"
#include "kbutton.hpp"

#ifdef OE
#include "interfaces.hpp"

#undef CON_COMMAND
#undef CON_COMMAND_F

struct ArgsWrapper
{
	ArgsWrapper(){};

	int ArgC()
	{
		return interfaces::engine->Cmd_Argc();
	};

	const char* Arg(int arg)
	{
		return interfaces::engine->Cmd_Argv(arg);
	};
};

#define CON_COMMAND(name, description) \
	static void name(ArgsWrapper args); \
	static void name##_wrapper() \
	{ \
		if (!interfaces::engine) \
			return; \
		ArgsWrapper args; \
		name(args); \
	} \
	static ConCommand name##_command(#name, name##_wrapper, description); \
	static void name(ArgsWrapper args)

#define CON_COMMAND_F(name, description, flags) \
	static void name(ArgsWrapper args); \
	static void name##_wrapper() \
	{ \
		if (!interfaces::engine) \
			return; \
		ArgsWrapper args; \
		name(args); \
	} \
	static ConCommand name##_command(#name, name##_wrapper, description, flags); \
	static void name(ArgsWrapper args)

#endif

#define CON_COMMAND_TOGGLE(toggle, description) static kbutton_t keyState_ ## toggle; \
static void toggle ## _plusFunc(const CCommand& args) \
{	\
	keyState_ ## toggle.KeyDown(args[1]);\
}	\
static void toggle ## _minusFunc(const CCommand& args) \
{	\
	keyState_ ## toggle.KeyUp(args[1]);\
}	\
static ConCommand toggle ## _plus("+" #toggle, toggle ## _plusFunc, description); \
static ConCommand toggle ## _minus("-" #toggle, toggle ## _minusFunc, description);
