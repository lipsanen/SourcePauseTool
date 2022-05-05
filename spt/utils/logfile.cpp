#include "stdafx.h"
#include "logfile.hpp"
#include <algorithm>
#include <filesystem>
#include <stdarg.h>

void LogFile::Log(const char* filepath, const char* fmt, ...)
{
	if (strcmp(filepath, m_sFilepath.c_str()) != 0)
	{
		if (m_outputStream.is_open())
		{
			m_outputStream.close();
		}
		std::filesystem::path p(filepath);
		std::filesystem::create_directories(p.parent_path());
		m_sFilepath = filepath;
		m_outputStream.open(m_sFilepath, std::ios::app);
	}

	// Couldn't open file or invalid filepath, exit
	if (!m_outputStream.good())
	{
		return;
	}

	const int BUFFER_SIZE = 8192;
	char BUFFER[BUFFER_SIZE];
	int bytes = strlen(fmt);
	m_outputStream.write(fmt, bytes);
	m_outputStream.flush();
}
/* what sort of coaxing do I have to do to write log files correctly using either c or c++ stdlib, atm I'm having the issue that if I delete a portion of a file while it's being written to, the c++ ofstream api doesnt handle it properly and replaces the deleted bytes with all zeros*/
