#include "stdafx.h"

#include <algorithm>
#include <random>
#include "math.hpp"

static std::mt19937 rng;

namespace utils
{
	// Return angle in [-180; 180).
	double NormalizeDeg(double a)
	{
		a = std::fmod(a, 360.0);
		if (a >= 180.0)
			a -= 360.0;
		else if (a < -180.0)
			a += 360.0;
		return static_cast<float>(a);
	}

	float RandomFloat(float min, float max)
	{
		std::uniform_real_distribution<float> uniformRandom(min, max);

		return uniformRandom(rng);
	}

	void NormalizeQAngle(QAngle& angle)
	{
		angle.x = NormalizeDeg(angle.x);
		angle.y = NormalizeDeg(angle.y);
	}

	void GetMiddlePoint(const QAngle& angle1, const QAngle& angle2, QAngle& out)
	{
		auto delta = angle2 - angle1;
		NormalizeQAngle(delta);
		delta /= 2;
		out = angle1 + delta;
		NormalizeQAngle(out);
	}

	// Return angle in [-Pi; Pi).
	double NormalizeRad(double a)
	{
		a = std::fmod(a, M_PI * 2);
		if (a >= M_PI)
			a -= 2 * M_PI;
		else if (a < -M_PI)
			a += 2 * M_PI;
		return a;
	}
} // namespace utils

#ifdef _LINUX
// Add missing libc++ functions because of some fast-math bug
#include <math.h>

extern "C" {
    double __exp_finite(double x) { return exp(x); }
    double __log_finite(double x) { return log(x); }
    double __pow_finite(double x, double y) { return pow(x, y); }
	float __atan2_finite(double x, double y) { return atan2(x, y); }

    float __expf_finite(float x) { return expf(x); }
    float __logf_finite(float x) { return logf(x); }
    float __powf_finite(float x, float y) { return powf(x, y); }
	float __atan2f_finite(float x, float y) { return atan2f(x, y); }
}
#endif