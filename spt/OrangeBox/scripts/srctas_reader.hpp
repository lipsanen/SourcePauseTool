#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include "variable_container.hpp"
#include "..\modules\ClientDLL.hpp"
#include <map>

namespace scripts
{
	class SourceTASReader
	{
	public:
		void ExecuteScript(std::string script);
		void StartSearch(std::string script);
		void SearchResult(std::string result);
	private:
		bool inFrames;
		bool freezeVariables;
		std::string fileName;
		std::ifstream scriptStream;
		std::istringstream lineStream;
		std::string line;
		int currentLine;
		long long int currentTick;
		SearchType searchType;

		VariableContainer variables;
		std::string startCommand;
		std::vector<afterframes_entry_t> afterFramesEntries;
		std::map<std::string, std::string> props;

		bool FindSearchType();
		void CommonExecuteScript();
		void Reset();
		void ResetIterationState();
		void Execute();
		void AddAfterframesEntry(long long int tick, std::string command);
		
		bool ParseLine();
		void SetNewLine();
		void ReplaceVariables();

		void ParseProps();
		void ParseProp();
		void HandleProps();

		void ParseVariables();
		void ParseVariable();

		void ParseFrames();
		void ParseFrameBulk();

		bool isLineEmpty();
		bool IsFramesLine();
		bool IsVarsLine();
	};

	std::string GetVarIdentifier(std::string name);
	extern SourceTASReader g_TASReader;
}

