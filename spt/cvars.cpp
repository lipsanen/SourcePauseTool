#include "stdafx.hpp"
#include "cvars.hpp"

#include <stdarg.h>
#include <algorithm>

#include "SPTLib\MemUtils.hpp"
#include "thirdparty/x86.h"
#include "tier1\tier1.h"

#include "spt\features\cvar.hpp"
#include "spt\feature.hpp"
#include "convar.hpp"
#include "interfaces.hpp"
#include "sptlib-wrapper.hpp"

struct FeatureCommand
{
	void* owner; // Feature*
	ConCommandBase* command;
	bool dynamicCommand; // dynamically allocated
	bool dynamicName;
	bool unhideOnUnregister;
};
static std::vector<FeatureCommand> cmd_to_feature;
static ConCommandBase** pluginCommandListHead = nullptr;

void Cvar_InitConCommandBase(ConCommandBase& concommand, void* owner)
{
	auto found = std::find_if(cmd_to_feature.begin(),
	                          cmd_to_feature.end(),
	                          [concommand](const FeatureCommand& fc) { return fc.command == &concommand; });
	if (found != cmd_to_feature.end())
	{
		Warning("Two commands trying to init concommand %s!\n", concommand.GetName());
		return;
	}

	FeatureCommand cmd = {owner, &concommand, false, false, false};
	cmd_to_feature.push_back(cmd);
}

void FormatConCmd(const char* fmt, ...)
{
	static char BUFFER[8192];

	va_list args;
	va_start(args, fmt);
	vsprintf_s(BUFFER, ARRAYSIZE(BUFFER), fmt, args);
	va_end(args);

	EngineConCmd(BUFFER);
}

typedef ICvar*(__cdecl* _GetCvarIF)();

extern "C" ICvar* GetCVarIF()
{
	static ICvar* ptr = nullptr;

	if (ptr != nullptr)
	{
		return ptr;
	}
	else
	{
		void* moduleHandle;
		void* moduleBase;
		std::size_t moduleSize;

		if (MemUtils::GetModuleInfo(L"vstdlib.dll", &moduleHandle, &moduleBase, &moduleSize))
		{
			_GetCvarIF func = (_GetCvarIF)MemUtils::GetSymbolAddress(moduleHandle, "GetCVarIF");
			ptr = func();
		}

		return ptr;
	}
}

static void RemoveCommandFromList(ConCommandBase* command)
{
	ConCommandBase* pPrev = NULL;
	for (ConCommandBase* pCommand = ConCommandBase::s_pConCommandBases; pCommand;
	     pCommand = pCommand->m_pNext)
	{
		if (pCommand != (ConCommandBase*)command)
		{
			pPrev = pCommand;
			continue;
		}

		if (pPrev == NULL)
			ConCommandBase::s_pConCommandBases = pCommand->m_pNext;
		else
			pPrev->m_pNext = pCommand->m_pNext;

		pCommand->m_pNext = NULL;
		break;
	}
}

#ifdef OE
static ConCommandBase** GetGlobalCommandListHead()
{
	// According to the SDK, ICvar::GetCommands is position 9 on the vtable
	void** icvarVtable = *(void***)g_pCVar;
	uint8_t* addr = (uint8_t*)icvarVtable[9];
	// Follow along thunked function
	if (*addr == X86_JMPIW)
	{
		uint32_t offset = *(uint32_t*)(addr + 1);
		addr += x86_len(addr) + offset;
	}
	// Check if it's the right instruction
	if (*addr == X86_MOVEAXII)
		return *(ConCommandBase***)(addr + 1);
	return NULL;
}

static void UnregisterConCommand(ConCommandBase* pCommandToRemove)
{
	static bool searchedForList = false;
	static ConCommandBase** globalCommandListHead = nullptr;

	// Look for the global command list head once
	if (!searchedForList)
	{
		globalCommandListHead = GetGlobalCommandListHead();
		searchedForList = true;
	}

	if (!globalCommandListHead)
		return;

	// Not registered? Don't bother
	if (!pCommandToRemove->IsRegistered())
		return;

	reinterpret_cast<ConCommandBase_guts*>(pCommandToRemove)->m_bRegistered = false;
	RemoveCommandFromList(globalCommandListHead, pCommandToRemove);
}

class CPluginConVarAccessor : public IConCommandBaseAccessor
{
public:
	virtual bool RegisterConCommandBase(ConCommandBase* pCommand)
	{
		pCommand->AddFlags(FCVAR_PLUGIN); // just because

		// Unlink from plugin only list
		// Necessary because RegisterConCommandBase skips the command if it's next isn't null
		pCommand->SetNext(0);

		// Link to engine's list instead
		g_pCVar->RegisterConCommandBase(pCommand);
		return true;
	}
};

static CPluginConVarAccessor g_ConVarAccessor;
#endif // OE

// Returns the address of ConCommandBase::s_pConCommandBases
// This is where commands are stored before being registered
static ConCommandBase** GetPluginCommandListHead()
{
#ifdef OE
	// First thing that happens in this function is loading a register with the list head
	uint8_t* registerAddr = (uint8_t*)ConCommandBaseMgr::OneTimeInit;
#else
	// Close to the end of ConVar_Register, the list head is set to null
	// There's a convenient jump close to the start of the function that takes
	// us a couple instructions past that point
	uint8_t* registerAddr = (uint8_t*)ConVar_Register;
#endif
	// Not sure if it's only a debug build thing, but the function address might be
	// in a jmp table. Follow the jmp if that's the case
	if (*registerAddr == X86_JMPIW)
	{
		uint32_t offset = *(uint32_t*)(registerAddr + 1);
		registerAddr += x86_len(registerAddr) + offset;
	}
#ifdef OE
	return *(ConCommandBase***)(registerAddr + 2);
#else
	int jzOffset = 0, _len = 0;
	for (uint8_t* addr = registerAddr; _len != -1 && addr - registerAddr < 32; _len = x86_len(addr), addr += _len)
	{
		// Check for a JZ (both short jump and near jump opcodes)
		if (addr[0] == X86_JZ)
			jzOffset = addr[1];
		else if (addr[0] == X86_2BYTE && addr[1] == X86_2B_JZII)
			jzOffset = *(uint32_t*)(addr + 2);

		if (jzOffset)
		{
			// Take the ride down the jump and go back a couple instructions
			addr += x86_len(addr) + jzOffset - 11;
			// Make sure that we landed on the instruction we wanted
			if (addr[0] != X86_MOVMIW)
				return NULL;
			return *(ConCommandBase***)(addr + 2);
		}
	}
	return NULL;
#endif
}

const char* WrangleLegacyCommandName(const char* name, bool useTempStorage, bool* allocated)
{
	static char* tmpStr = nullptr;
	static size_t tmpLen = 0;

	bool plusMinus = (name[0] == '+' || name[0] == '-');
	char* newName = const_cast<char*>(strstr(name, "spt_"));
	if (newName == (name + plusMinus) || (newName && !plusMinus))
	{
		if (allocated)
			*allocated = false;
		return plusMinus ? name : newName;
	}

	if (allocated)
		*allocated = true;

	size_t allocLen = newName ? strlen(newName) + 2 // ex: +y_spt_duckspam
	                          : strlen(name) + 5;   // ex: tas_pause, +myprefix_something

	// (re)allocate new string
	char* allocStr;
	if (useTempStorage)
	{
		if (allocLen > tmpLen)
		{
			delete[] tmpStr;
			tmpStr = new char[allocLen];
			tmpLen = allocLen;
		}
		allocStr = tmpStr;
	}
	else
	{
		allocStr = new char[allocLen];
	}

	if (newName)
	{
		allocStr[0] = name[0];
		strcpy(allocStr + 1, newName);
	}
	else
	{
		allocStr[0] = name[0];
		strcpy(allocStr + plusMinus, "spt_");
		strcat(allocStr, name + plusMinus);
	}
	return allocStr;
}

// Check if spt_ prefix is at the start. If not, create a new command and hide the old one with
// the legacy name. Put prefix at the start if we don't find it at all
static void HandleBackwardsCompatibility(FeatureCommand& featCmd, const char* cmdName)
{
	ConCommandBase* cmd = featCmd.command;
	bool allocatedName;
	const char* newName = WrangleLegacyCommandName(cmdName, false, &allocatedName);

	// New commands are added to the beginning of the linked list, so it's safe to do this
	// while iterating through the list. s_pAccessor needs to be NULL here or else the
	// constructor calls below will break the local command list
	ConCommandBase* newCommand;
	if (cmd->IsCommand())
	{
		ConCommand_guts* cmdGuts = reinterpret_cast<ConCommand_guts*>(cmd);
		newCommand = new ConCommand(newName,
		                            cmdGuts->m_fnCommandCallback,
		                            cmdGuts->m_pszHelpString,
		                            cmdGuts->m_nFlags,
		                            cmdGuts->m_fnCompletionCallback);
	}
	else
	{
		newCommand = new ConVarProxy((ConVar*)cmd, newName, ((ConVar_guts*)cmd)->m_nFlags);
	}

	// If a legacy command didn't originally have FCVAR_HIDDEN set, we need to hide it now and
	// unhide it when unregistering because if the commands are initialized again, they would all be hidden
	if (!cmd->IsFlagSet(FCVAR_HIDDEN))
	{
		cmd->AddFlags(FCVAR_HIDDEN);
		featCmd.unhideOnUnregister = true;
	}

	// After the push the featCmd reference can be invalid!!!
	FeatureCommand newCmd = { featCmd.owner, newCommand, true, allocatedName, false };
	cmd_to_feature.push_back(newCmd);
}

void Cvar_RegisterSPTCvars()
{
	if (!g_pCVar)
		return;

	ConCommandBase* cmd = ConCommandBase::s_pConCommandBases;

	while (cmd != NULL)
	{
		const char* cmdName = cmd->GetName();
		if (strlen(cmdName) == 0) // There's a CEmptyConVar in the list for some reason
		{
			cmd = (ConCommandBase*)cmd->GetNext();
			continue;
		}

		auto inittedCmd = std::find_if(cmd_to_feature.begin(),
			cmd_to_feature.end(),
			[cmd](const FeatureCommand& fc) { return fc.command == cmd; });

		if (inittedCmd == cmd_to_feature.end())
		{
			DevWarning("Command %s was unloaded, because it was not initialized!\n", cmdName);
			ConCommandBase* todelete = cmd;
			cmd = (ConCommandBase*)cmd->GetNext();
			RemoveCommandFromList(todelete);
			continue;
		}
		else
		{
			cmd = (ConCommandBase*)cmd->GetNext();
		}

		HandleBackwardsCompatibility(*inittedCmd, cmdName);
#ifdef OE
		if (cmd->IsBitSet(FCVAR_HIDDEN))
			spt_cvarstuff.hidden_cvars.push_back(cmd);
#endif
	}

	cmd = ConCommandBase::s_pConCommandBases;

	while (cmd != NULL)
	{
		auto next = cmd->GetNext();
		g_pCVar->RegisterConCommand(cmd);
		cmd = next;
	}
}

void Cvar_UnregisterSPTCvars()
{
	if (!g_pCVar)
		return;

	for (auto& cmd : cmd_to_feature)
	{
		g_pCVar->UnregisterConCommand(cmd.command);
		if (cmd.dynamicCommand)
		{
			if (cmd.dynamicName)
				delete[] cmd.command->m_pszName;
			delete cmd.command;
		}
		// Refer to comment in HandleBackwardsCompatibility
		if (cmd.unhideOnUnregister)
			cmd.command->m_nFlags &= ~FCVAR_HIDDEN;
	}

	cmd_to_feature.clear();
}

#ifdef SSDK2013
CON_COMMAND(spt_dummy, "") {}

// Hack: ConCommand::AutoCompleteSuggest is broken in the SDK. Replacing it with the game's one.
void ReplaceAutoCompleteSuggest()
{
	if (!_record)
		return;
	ConCommand_guts* cmd = (ConCommand_guts*)_record;

	const int vtidx_AutoCompleteSuggest = 10;
	ConCommand_guts* dummy = (ConCommand_guts*)&spt_dummy_command;

	void* temp;
	Feature::AddVFTableHook(VFTableHook(dummy->vfptr,
	                                    vtidx_AutoCompleteSuggest,
	                                    cmd->vfptr[vtidx_AutoCompleteSuggest],
	                                    &temp),
	                        "spt-2013");
}

#endif
