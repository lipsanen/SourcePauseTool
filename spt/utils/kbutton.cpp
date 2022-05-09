#include "stdafx.h"
#include "convar.hpp"
#include "dbg.h"
#include <cstdlib>

void kbutton_t::KeyDown(const char* c)
{
	int		k = -1;
	if (c && c[0])
	{
		k = atoi(c);
	}

	if (k == this->down[0] || k == this->down[1])
		return;		// repeating key

	if (!this->down[0])
		this->down[0] = k;
	else if (!this->down[1])
		this->down[1] = k;
	else
	{
		if (c[0])
		{
			DevMsg(1, "Three keys down for a button '%c' '%c' '%c'!\n", this->down[0], this->down[1], c);
		}
		return;
	}

	if (this->state & 1)
		return;		// still down
	this->state |= 1 + 2;	// down + impulse down
}

void kbutton_t::KeyUp(const char* c)
{
	if (!c || !c[0])
	{
		this->down[0] = this->down[1] = 0;
		this->state = 4;	// impulse up
		return;
	}

	int k = atoi(c);

	if (this->down[0] == k)
		this->down[0] = 0;
	else if (this->down[1] == k)
		this->down[1] = 0;
	else
		return;		// key up without coresponding down (menu pass through)

	if (this->down[0] || this->down[1])
	{
		//Msg ("Keys down for button: '%c' '%c' '%c' (%d,%d,%d)!\n", b->down[0], b->down[1], c, b->down[0], b->down[1], c);
		return;		// some other key is still holding it down
	}

	if (!(this->state & 1))
		return;		// still up (this should not happen)

	this->state &= ~1;		// now up
	this->state |= 4; 		// impulse up
}

float kbutton_t::KeyState()
{
	float		val = 0.0;
	int			impulsedown, impulseup, down;

	impulsedown = this->state & 2;
	impulseup = this->state & 4;
	down = this->state & 1;

	if (impulsedown && !impulseup)
	{
		// pressed and held this frame?
		val = down ? 0.5 : 0.0;
	}

	if (impulseup && !impulsedown)
	{
		// released this frame?
		val = down ? 0.0 : 0.0;
	}

	if (!impulsedown && !impulseup)
	{
		// held the entire frame?
		val = down ? 1.0 : 0.0;
	}

	if (impulsedown && impulseup)
	{
		if (down)
		{
			// released and re-pressed this frame
			val = 0.75;
		}
		else
		{
			// pressed and released this frame
			val = 0.25;
		}
	}

	// clear impulses
	this->state &= 1;
	return val;
}
