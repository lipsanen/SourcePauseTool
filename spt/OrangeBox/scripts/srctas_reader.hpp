#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include "variable_container.hpp"
#include "condition.hpp"
#include "range_variable.hpp"
#include "parsed_script.hpp"

namespace scripts
{
	extern const std::string SCRIPT_EXT;

	class SourceTASReader
	{
	public:
		SourceTASReader();
		void ExecuteScriptAndPause(const std::string& script, int pauseTick);
		void ExecuteScript(const std::string& script);
		void StartSearch(const std::string& script);
		void ReadScript(bool search);
		void SearchResult(scripts::SearchResult result);
		void OnAfterFrames();
		int GetCurrentScriptLength();
		const ParsedScript& GetCurrentScript();
		void ResetConvars();
	private:
		bool iterationFinished;
		bool freezeVariables;
		std::string fileName;
		std::ifstream scriptStream;
		std::istringstream lineStream;
		std::string origLine;
		std::string editedLine;
		int currentLineNumber;
		long long int currentTick;
		SearchType searchType;

		VariableContainer variables;
		ParsedScript currentScript;
		std::map<std::string, void(SourceTASReader::*)(const std::string&)> propertyHandlers;
		std::vector<std::unique_ptr<Condition>> conditions;

		void CommonExecuteScript(bool search, int maxLength);
		void Reset();
		void ResetIterationState();
		void Execute(int maxLength);

		bool ParseLine();
		void SetNewLine();
		void ReplaceVariables();

		void InitPropertyHandlers();
		void ParseProps();
		void ParseProp();
		void HandleSettings(const std::string& value);
		void HandleSave(const std::string& value);
		void HandleDemo(const std::string& value);
		void HandleSearch(const std::string& value);
		void HandlePlaybackSpeed(const std::string& value);
		void HandleTickRange(const std::string& value);
		void HandleTicksFromEndRange(const std::string& value);
		
		void HandleXPos(const std::string& value) { HandlePosVel(value, Axis::AxisX, true); }
		void HandleYPos(const std::string& value) { HandlePosVel(value, Axis::AxisY, true); }
		void HandleZPos(const std::string& value) { HandlePosVel(value, Axis::AxisZ, true); }

		void HandleXVel(const std::string& value) { HandlePosVel(value, Axis::AxisX, false); }
		void HandleYVel(const std::string& value) { HandlePosVel(value, Axis::AxisY, false); }
		void HandleZVel(const std::string& value) { HandlePosVel(value, Axis::AxisZ, false); }
		void HandleAbsVel(const std::string& value) { HandlePosVel(value, Axis::Abs, false); }
		void Handle2DVel(const std::string& value) { HandlePosVel(value, Axis::TwoD, false); }
		void HandlePosVel(const std::string& value, Axis axis, bool isPos);

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
