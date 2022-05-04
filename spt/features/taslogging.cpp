#include "stdafx.h"
#include "taslogging.hpp"
#include <stdarg.h>

TASLoggingFeature spt_taslogging;

ConVar tas_log("tas_log", "0", 0, "Set to 1 for TAS logging.");
ConVar tas_logfile("tas_logfile", "tas.log", 0, "Set path of the logfile for TAS logging.");

void TASLoggingFeature::Log(const char* fmt, ...)
{
	if (!tas_log.GetBool())
	{
		return;
	}

	if (OpenStream())
	{
		char BUFFER[4096];
		va_list vl;
		va_start(vl, fmt);
		vsnprintf(BUFFER, sizeof(BUFFER), fmt, vl);
		m_outputStream << BUFFER << std::endl;
		va_end(vl);
	}
}

bool TASLoggingFeature::OpenStream()
{
	if (strcmp(m_sFilepath.c_str(), tas_logfile.GetString()) == 0)
		return true;

	std::string newFile = tas_logfile.GetString();

	if(m_outputStream.is_open())
	{
		m_outputStream.close();
	}

	m_outputStream.open(newFile, std::ios_base::app);

	if (m_outputStream.is_open())
	{
		m_sFilepath = newFile;
		return true;
	}
	else
	{
		return false;
	}
}

bool TASLoggingFeature::ShouldLoadFeature()
{
	return true;
}

void TASLoggingFeature::InitHooks() {}

void TASLoggingFeature::LoadFeature()
{
	InitConcommandBase(tas_log);
	InitConcommandBase(tas_logfile);
}

void TASLoggingFeature::UnloadFeature() {}
