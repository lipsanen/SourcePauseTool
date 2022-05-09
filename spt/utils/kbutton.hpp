#pragma once

struct kbutton_t
{
	// key nums holding it down
	int		down[2];
	// low bit is down state
	int		state;

	float KeyState();
	void KeyUp(const char* c);
	void KeyDown(const char* c);

	kbutton_t()
	{
		down[0] = down[1] = state = 0;
	}
};
