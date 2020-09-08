#include "stdafx.h"

#include "..\sptlib-wrapper.hpp"
#include "..\utils\ent_utils.hpp"
#include "cvars.hpp"
#include "modules.hpp"
#include "modules\EngineDLL.hpp"
#include "scripts\srctas_reader.hpp"
#include "scripts\tests\test.hpp"
#include "..\ipc\ipc-spt.hpp"

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
		clientDLL.FrameSignal.Connect(ipc::Loop);

		clientDLL.AfterframesSignal.Connect(&scripts::g_TASReader, &scripts::SourceTASReader::OnAfterFrames);
		clientDLL.AfterframesSignal.Connect(&scripts::g_Tester, &scripts::Tester::OnAfterFrames);
		clientDLL.AfterframesSignal.Connect(&PauseOnDemoTick);
#if !defined(OE) && !defined(P2)
		clientDLL.AfterframesSignal.Connect(&utils::CheckPiwSave);
#endif

		clientDLL.TickSignal.Connect(&scripts::g_Tester, &scripts::Tester::DataIteration);

#ifndef OE
		clientDLL.TickSignal.Connect(&vgui_matsurfaceDLL, &VGui_MatSurfaceDLL::NewTick);
		clientDLL.OngroundSignal.Connect(&vgui_matsurfaceDLL, &VGui_MatSurfaceDLL::OnGround);

		serverDLL.JumpSignal.Connect(&vgui_matsurfaceDLL, &VGui_MatSurfaceDLL::Jump);
#endif
	}
} // namespace ModuleHooks