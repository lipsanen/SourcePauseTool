#pragma once
#include <memory>
#include <vgui_controls\Panel.h>
#include <vgui_controls\label.h>

namespace scripts
{
	class CTASGUI : public vgui::Panel
	{
	public:
		CTASGUI(const char* panelName);
		~CTASGUI() override;
	private:
	};

	extern CTASGUI* pTASGUI;

	void OpenTASGUI();
}