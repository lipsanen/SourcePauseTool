#include "gtest\gtest.h"
#include "SPTLib2\math.hpp"

TEST(Math, NormalizeDeg)
{
	EXPECT_EQ(utils::NormalizeDeg(181), -179);
}
