#include "stdafx.h"
#include "convar.h"
#include "OrangeBox/modules.hpp"
#include "utils/math.hpp"
#include "utils/ent_utils.hpp"
#include "OrangeBox/cvars.hpp"
#include "spt/sptlib-wrapper.hpp"
#include "tier1/checksum_md5.h"

#if !defined(OE) && !defined(P2)

// I'll copy paste the game's rng stuff here since I dont wanna affect how the game is running by using the in-game rng

#define IA 16807
#define IM 2147483647
#define IQ 127773
#define IR 2836
#define NDIV (1 + (IM - 1) / NTAB)
#define MAX_RANDOM_RANGE 0x7FFFFFFFUL
#define NTAB 32

#define AM (1.0 / IM)
#define EPS 1.2e-7
#define RNMX (1.0 - EPS)

class RandomStream
{
public:
	void SetSeed(int iSeed);
	int GenerateRandomNumber();
	float RandomFloat(float flLow, float flHigh);
	int RandomInt(int iLow, int iHigh);

private:
	int m_idum;
	int m_iy;
	int m_iv[NTAB];
};

void RandomStream::SetSeed(int iSeed)
{
	m_idum = ((iSeed < 0) ? iSeed : -iSeed);
	m_iy = 0;
}

int RandomStream::GenerateRandomNumber()
{
	int j;
	int k;

	if (m_idum <= 0 || !m_iy)
	{
		if (-(m_idum) < 1)
			m_idum = 1;
		else
			m_idum = -(m_idum);

		for (j = NTAB + 7; j >= 0; j--)
		{
			k = (m_idum) / IQ;
			m_idum = IA * (m_idum - k * IQ) - IR * k;
			if (m_idum < 0)
				m_idum += IM;
			if (j < NTAB)
				m_iv[j] = m_idum;
		}
		m_iy = m_iv[0];
	}
	k = (m_idum) / IQ;
	m_idum = IA * (m_idum - k * IQ) - IR * k;
	if (m_idum < 0)
		m_idum += IM;
	j = m_iy / NDIV;

	m_iy = m_iv[j];
	m_iv[j] = m_idum;

	return m_iy;
}

float RandomStream::RandomFloat(float flLow, float flHigh)
{
	// float in [0,1)
	float fl = AM * GenerateRandomNumber();
	if (fl > RNMX)
	{
		fl = RNMX;
	}
	return (fl * (flHigh - flLow)) + flLow; // float in [low,high)
}

int RandomStream::RandomInt(int iLow, int iHigh)
{
	//ASSERT(lLow <= lHigh);
	unsigned int maxAcceptable;
	unsigned int x = iHigh - iLow + 1;
	unsigned int n;
	if (x <= 1 || MAX_RANDOM_RANGE < x - 1)
	{
		return iLow;
	}

	// The following maps a uniform distribution on the interval [0,MAX_RANDOM_RANGE]
	// to a smaller, client-specified range of [0,x-1] in a way that doesn't bias
	// the uniform distribution unfavorably. Even for a worst case x, the loop is
	// guaranteed to be taken no more than half the time, so for that worst case x,
	// the average number of times through the loop is 2. For cases where x is
	// much smaller than MAX_RANDOM_RANGE, the average number of times through the
	// loop is very close to 1.
	//
	maxAcceptable = MAX_RANDOM_RANGE - ((MAX_RANDOM_RANGE + 1) % x);
	do
	{
		n = GenerateRandomNumber();
	} while (n > maxAcceptable);

	return iLow + (n % x);
}

// Rng cones
#define VECTOR_CONE_PRECALCULATED vec3_origin
#define VECTOR_CONE_1DEGREES Vector(0.00873, 0.00873, 0.00873)
#define VECTOR_CONE_2DEGREES Vector(0.01745, 0.01745, 0.01745)
#define VECTOR_CONE_3DEGREES Vector(0.02618, 0.02618, 0.02618)
#define VECTOR_CONE_4DEGREES Vector(0.03490, 0.03490, 0.03490)
#define VECTOR_CONE_5DEGREES Vector(0.04362, 0.04362, 0.04362)
#define VECTOR_CONE_6DEGREES Vector(0.05234, 0.05234, 0.05234)
#define VECTOR_CONE_7DEGREES Vector(0.06105, 0.06105, 0.06105)
#define VECTOR_CONE_8DEGREES Vector(0.06976, 0.06976, 0.06976)
#define VECTOR_CONE_9DEGREES Vector(0.07846, 0.07846, 0.07846)
#define VECTOR_CONE_10DEGREES Vector(0.08716, 0.08716, 0.08716)
#define VECTOR_CONE_15DEGREES Vector(0.13053, 0.13053, 0.13053)
#define VECTOR_CONE_20DEGREES Vector(0.17365, 0.17365, 0.17365)

static bool GetCone(int cone, Vector& out)
{
#define DegreeCase(degree) case degree: out = VECTOR_CONE_##degree##DEGREES; return true

	switch(cone)
	{
	DegreeCase(1);
	DegreeCase(2);
	DegreeCase(3);
	DegreeCase(4);
	DegreeCase(5);
	DegreeCase(6);
	DegreeCase(7);
	DegreeCase(8);
	DegreeCase(9);
	DegreeCase(10);
	DegreeCase(15);
	DegreeCase(20);
	default:
		return false;
	}

}

static void GetRandomXY(float& x, float& y, int commandOffset)
{
	static RandomStream random;

	float z;
	float shotBiasMin = -1.0f;
	float shotBiasMax = 1.0f;
	float bias = 1.0f;

	// 1.0 gaussian, 0.0 is flat, -1.0 is inverse gaussian
	float shotBias = ((shotBiasMax - shotBiasMin) * bias) + shotBiasMin;
	float flatness = (fabsf(shotBias) * 0.5);

	int command_number = serverDLL.GetCommandNumber() + commandOffset;
	int predictionRandomSeed = MD5_PseudoRandom(command_number) & 0x7fffffff;
	int seed = predictionRandomSeed & 255;
	random.SetSeed(seed);

	do
	{
		x = random.RandomFloat(-1, 1) * flatness + random.RandomFloat(-1, 1) * (1 - flatness);
		y = random.RandomFloat(-1, 1) * flatness + random.RandomFloat(-1, 1) * (1 - flatness);
		if (shotBias < 0)
		{
			x = (x >= 0) ? 1.0 - x : -1.0 - x;
			y = (y >= 0) ? 1.0 - y : -1.0 - y;
		}
		z = x * x + y * y;
	} while (z > 1);
}

// Iteratively improves the optimal aim angle
static void GetAimAngleIterative(const QAngle& target, QAngle& current, int commandOffset, const Vector& vecSpread)
{
	// v = aim angle after spread
	// z = target aim angle
	// w = current aim angle(initially z)
	// w += v - z
	// However this is just an approximate solution, the spread applied changes when you change w
	// since the corresponding right/up vectors used in spread calculations change as well.
	// Iteratively updating the w vector converges to the correct result if spread is not too large.
	float x, y;
	GetRandomXY(x, y, commandOffset);

	Vector forward, right, up, vecDir;
	QAngle resultingAngle;

	AngleVectors(current, &forward, &right, &up);
	Vector result = forward + x * vecSpread.x * right + y * vecSpread.y * up; // This is the shot vector

	VectorAngles(result, resultingAngle); // Get the corresponding view angle

	// Then calculate the difference to the target angle wanted
	double diff[2];
	for (int i = 0; i < 2; ++i)
		diff[i] = utils::NormalizeDeg(utils::NormalizeDeg(resultingAngle[i]) - target[i]);

	current[0] -= diff[0];
	current[1] -= diff[1];
}

#define PUNCH_DAMPING 9.0f // bigger number makes the response more damped, smaller is less damped
// currently the system will overshoot, with larger damping values it won't
#define PUNCH_SPRING_CONSTANT 65.0f // bigger number increases the speed at which the view corrects

static QAngle DecayPunchAngle(QAngle m_vecPunchAngle, QAngle m_vecPunchAngleVel, int frames)
{
	const float frametime = 0.015f;
	for (int i = 0; i < frames; ++i)
	{
		if (m_vecPunchAngle.LengthSqr() > 0.001 || m_vecPunchAngleVel.LengthSqr() > 0.001)
		{
			m_vecPunchAngle += m_vecPunchAngleVel * frametime;
			float damping = 1 - (PUNCH_DAMPING * frametime);

			if (damping < 0)
			{
				damping = 0;
			}
			m_vecPunchAngleVel *= damping;

			// torsional spring
			// UNDONE: Per-axis spring constant?
			float springForceMagnitude = PUNCH_SPRING_CONSTANT * frametime;
			springForceMagnitude = clamp(springForceMagnitude, 0, 2);
			m_vecPunchAngleVel -= m_vecPunchAngle * springForceMagnitude;

			// don't wrap around
			m_vecPunchAngle.Init(clamp(m_vecPunchAngle.x, -89, 89),
			                     clamp(m_vecPunchAngle.y, -179, 179),
			                     clamp(m_vecPunchAngle.z, -89, 89));
		}
		else
		{
			m_vecPunchAngle.Init(0, 0, 0);
		}
	}

	return m_vecPunchAngle;
}

CON_COMMAND(tas_aim, "boom, headshot *music*")
{
	if (args.ArgC() != 5)
	{
		Msg("Usage: tas_aim <pitch> <yaw> <frames> <cone>\nWeapon cones(in degrees):\n\t- AR2: 3\n\t- Pistol & SMG: 5\n");
		return;
	}
	else if (!utils::playerEntityAvailable())
	{
		Msg("Trying to aim while map not loaded in!\n");
		return;
	}

	float pitch = std::atof(args.Arg(1));
	float yaw = std::atof(args.Arg(2));
	int frames = std::atoi(args.Arg(3));
	int cone = std::atoi(args.Arg(4));
	const int MAX_FRAMES = 10000;

	if (frames <= 0)
	{
		Msg("Frames has to be >= 0\n");
		return;
	}
	else if (frames > MAX_FRAMES)
	{
		Msg("Frame count cannot be higher than 10,000.\n");
		return;
	}

	QAngle angle(pitch, yaw, 0);
	QAngle aimAngle = angle;

	if (cone != 0)
	{
		Vector vecSpread;
		if (!GetCone(cone, vecSpread))
		{
			Msg("Couldn't find cone: %s\n", args.Arg(4));
			return;
		}

		// Even the first approximation seems to be relatively accurate and it seems to converge after 2nd iteration
		for (int i = 0; i < 2; ++i)
			GetAimAngleIterative(angle, aimAngle, frames, vecSpread);
	}

	QAngle punchAngle, punchAnglevel;
	utils::GetPunchAngleInformation(punchAngle, punchAnglevel);

	QAngle futurePunchAngle = DecayPunchAngle(punchAngle, punchAnglevel, frames);
	QAngle diff = futurePunchAngle - punchAngle;

	QAngle va;
	EngineGetViewAngles(reinterpret_cast<float*>(&va));
	va += diff;

	float pitchSpeed = utils::NormalizeDeg(aimAngle[PITCH] - va[PITCH]) / frames;
	float yawSpeed = utils::NormalizeDeg(aimAngle[YAW] - va[YAW]) / frames;
	_y_spt_pitchspeed.SetValue(pitchSpeed);
	_y_spt_yawspeed.SetValue(yawSpeed);

	clientDLL.AddIntoAfterframesQueue(afterframes_entry_t(frames, "_y_spt_pitchspeed 0; _y_spt_yawspeed 0"));
}

#endif