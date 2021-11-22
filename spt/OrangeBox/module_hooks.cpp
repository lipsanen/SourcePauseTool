#include "stdafx.h"

#include "..\ipc\ipc-spt.hpp"
#include "..\sptlib-wrapper.hpp"
#include "..\utils\ent_utils.hpp"
#include "cvars.hpp"
#include "scripts\srctas_reader.hpp"
#include "scripts\tests\test.hpp"
#include "vgui\graphics.hpp"
#include "..\features\autojump.hpp"
#include "..\features\generic.hpp"

namespace ModuleHooks
{
	void ConnectSignals()
	{
		generic_.FrameSignal.Connect(ipc::Loop);
		_afterframes.AfterFramesSignal.Connect(&scripts::g_TASReader, &scripts::SourceTASReader::OnAfterFrames);
		_afterframes.AfterFramesSignal.Connect(&scripts::g_Tester, &scripts::Tester::OnAfterFrames);

		generic_.AdjustAngles.Connect(&scripts::g_Tester, &scripts::Tester::DataIteration);
#ifndef OE
		generic_.AdjustAngles.Connect(vgui::DrawLines);
#endif
	}
} // namespace ModuleHooks