#include "gtest/gtest.h"
#include "spt/utils/kbutton.hpp"

TEST(kbutton, BaseStateWorks)
{
	kbutton_t button;
	EXPECT_EQ(button.KeyState(), 0.0f);
}

TEST(kbutton, KeyDownWorks)
{
	kbutton_t button;
	button.KeyDown(nullptr);
	EXPECT_EQ(button.KeyState(), 0.5f);
	EXPECT_EQ(button.KeyState(), 1.0f);
}

TEST(kbutton, KeyUpWorks)
{
	kbutton_t button;
	button.KeyDown(nullptr);
	EXPECT_EQ(button.KeyState(), 0.5f);
	EXPECT_EQ(button.KeyState(), 1.0f);
	button.KeyUp(nullptr);
	EXPECT_EQ(button.KeyState(), 0.0f);
}

TEST(kbutton, OneKeyPressWorks)
{
	kbutton_t button;
	button.KeyDown("100");
	button.KeyUp("100");
	EXPECT_EQ(button.KeyState(), 0.25f);
}

TEST(kbutton, MultipleKeyWorks)
{
	kbutton_t button;
	button.KeyDown("100");
	button.KeyDown("101");
	EXPECT_EQ(button.KeyState(), 0.5f);
	EXPECT_EQ(button.KeyState(), 1.0f);
	button.KeyUp("100");
	EXPECT_EQ(button.KeyState(), 1.0f);
	button.KeyUp("101");
	EXPECT_EQ(button.KeyState(), 0.0f);
}

TEST(kbutton, ImpulseWorks)
{
	kbutton_t button;
	button.KeyDown("100");
	button.KeyDown("101");
	EXPECT_EQ(button.KeyState(), 0.5f);
	EXPECT_EQ(button.KeyState(), 1.0f);
	button.KeyUp(nullptr);
	EXPECT_EQ(button.KeyState(), 0.0f);
}
