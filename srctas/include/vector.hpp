#pragma once

struct Vector
{
	Vector(float x, float y, float z) : x(x), y(y), z(z) {}

	float x, y, z;

	float& operator[](int i)
	{
		if (i == 0)
			return x;
		else if (i == 1)
			return y;
		else
			return z;
	}

	float operator[](int i) const
	{
		if (i == 0)
			return x;
		else if (i == 1)
			return y;
		else
			return z;
	}
};