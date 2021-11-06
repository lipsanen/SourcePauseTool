#include "stdafx.h"

#include "..\ipc\ipc-spt.hpp"
#include "..\sptlib-wrapper.hpp"
#include "..\utils\ent_utils.hpp"
#include "cvars.hpp"
#include "modules.hpp"
#include "modules\EngineDLL.hpp"
#include "scripts\srctas_reader.hpp"
#include "scripts\tests\test.hpp"
#include "vgui\graphics.hpp"
#include "..\features\autojump.hpp"
#include "..\features\generic.hpp"

namespace ModuleHooks
{
	void PauseOnDemoTick()
	{
#ifdef OE
		if (engineDLL.Demo_IsPlayingBack() && !engineDLL.Demo_IsPlaybackPaused())
		{
			auto tick = y_spt_pause_demo_on_tick.GetInt();
			if (tick != 0)
			{
				if (tick < 0)
					tick += engineDLL.Demo_GetTotalTicks();

				if (tick == engineDLL.Demo_GetPlaybackTick())
					EngineConCmd("demo_pause");
			}
		}
#endif
	}

	void ConnectSignals()
	{
		generic_.FrameSignal.Connect(ipc::Loop);
		_afterframes.AfterFramesSignal.Connect(&scripts::g_TASReader, &scripts::SourceTASReader::OnAfterFrames);
		_afterframes.AfterFramesSignal.Connect(&scripts::g_Tester, &scripts::Tester::OnAfterFrames);
		_afterframes.AfterFramesSignal.Connect(&PauseOnDemoTick);
#if !defined(OE) && !defined(P2)
		_afterframes.AfterFramesSignal.Connect(&utils::CheckPiwSave);
#endif

		generic_.TickSignal.Connect(&scripts::g_Tester, &scripts::Tester::DataIteration);

#ifndef OE
		generic_.TickSignal.Connect(&vgui_matsurfaceDLL, &VGui_MatSurfaceDLL::NewTick);
		generic_.TickSignal.Connect(vgui::DrawLines);
		generic_.OngroundSignal.Connect(&vgui_matsurfaceDLL, &VGui_MatSurfaceDLL::OnGround);

		_autojump.JumpSignal.Connect(&vgui_matsurfaceDLL, &VGui_MatSurfaceDLL::Jump);
#endif
	}
} // namespace ModuleHooks