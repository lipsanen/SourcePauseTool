#include "stdafx.h"
#include "../cvars.hpp"
#include "../sptlib-wrapper.hpp"
#include "convar.hpp"
#include "interfaces.hpp"
#include "reset.hpp"

const char* RESET_VARS[] = { "cl_forwardspeed", "cl_sidespeed", "cl_yawspeed"};
const int RESET_VARS_COUNT = ARRAYSIZE(RESET_VARS);

void srctas::ResetConvars()
{
#ifndef OE
	ConCommandBase* cmd = interfaces::g_pCVar->GetCommands();

	// Loops through the console variables and commands
	while (cmd != NULL)
	{
		const char* name = cmd->GetName();
		// Reset any variables that have been marked to be reset for TASes
		if (!cmd->IsCommand() && name != NULL && cmd->IsFlagSet(FCVAR_TAS_RESET))
		{
			auto convar = interfaces::g_pCVar->FindVar(name);
			DevMsg("Trying to reset variable %s\n", name);

			// convar found
			if (convar != NULL)
			{
				DevMsg("Resetting var %s to value %s\n", name, convar->GetDefault());
				convar->SetValue(convar->GetDefault());
			}
			else
				throw std::exception("Unable to find listed console variable!");
		}

		// Issue minus commands to reset any keypresses
		else if (cmd->IsCommand() && cmd->GetName() != NULL && cmd->GetName()[0] == '-')
		{
			DevMsg("Running command %s\n", cmd->GetName());
			EngineConCmd(cmd->GetName());
		}

		cmd = cmd->GetNext();
	}

	// Reset any variables selected above
	for (int i = 0; i < RESET_VARS_COUNT; ++i)
	{
		auto resetCmd = interfaces::g_pCVar->FindVar(RESET_VARS[i]);
		if (resetCmd != NULL)
		{
			DevMsg("Resetting var %s to value %s\n", resetCmd->GetName(), resetCmd->GetDefault());
			resetCmd->SetValue(resetCmd->GetDefault());
		}
		else
			DevWarning("Unable to find console variable %s\n", RESET_VARS[i]);
	}
#endif
}
