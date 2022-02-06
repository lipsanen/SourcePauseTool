#include "pch.h"
#include "CppUnitTest.h"
#include "framebulk.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace srctastest
{
	TEST_CLASS(FrameBulkUnitTests)
	{
	public:
		TEST_METHOD(ParseEmptyBulk)
		{
			std::string line = "-------|------|--------|-|-|1|";
			srctas::FrameBulk bulk;
			auto rval = srctas::FrameBulk::ParseFrameBulk(line, bulk);
			Assert::IsTrue(rval.Success);

			Assert::IsFalse(bulk.Movement.Strafe);
			Assert::AreEqual(bulk.Movement.StrafeType, -1);
			Assert::AreEqual(bulk.Movement.JumpType, -1);
			Assert::IsFalse(bulk.Movement.Lgagst);
			Assert::IsFalse(bulk.Movement.AutoJump);
			Assert::IsFalse(bulk.Movement.Duckspam);
			Assert::IsFalse(bulk.Movement.Jumpbug);

			Assert::IsFalse(bulk.Forward);
			Assert::IsFalse(bulk.Left);
			Assert::IsFalse(bulk.Right);
			Assert::IsFalse(bulk.Back);
			Assert::IsFalse(bulk.Up);
			Assert::IsFalse(bulk.Down);

			Assert::IsFalse(bulk.Jump);
			Assert::IsFalse(bulk.Duck);
			Assert::IsFalse(bulk.Use);
			Assert::IsFalse(bulk.Attack1);
			Assert::IsFalse(bulk.Attack2);
			Assert::IsFalse(bulk.Reload);
			Assert::IsFalse(bulk.Walk);
			Assert::IsFalse(bulk.Sprint);

			Assert::IsFalse(bulk.AimSet);
			Assert::AreEqual(bulk.AimPitch, 0.0);
			Assert::AreEqual(bulk.AimYaw.Yaw, 0.0);
			Assert::IsTrue(bulk.AimYaw.Auto);
			Assert::AreEqual(bulk.AimFrames, 0);
			Assert::AreEqual(bulk.Cone, 0);

			Assert::IsFalse(bulk.Movement.StrafeYawSet);
			Assert::AreEqual(bulk.Movement.StrafeYaw, 0.0);

			Assert::AreEqual(bulk.Frames, 1);
			Assert::AreEqual(bulk.Commands, std::string(""));
		}

		TEST_METHOD(ParseField1)
		{
			std::string line = "s12ljdb|------|--------|-|-|1|";
			srctas::FrameBulk bulk;
			auto rval = srctas::FrameBulk::ParseFrameBulk(line, bulk);
			Assert::IsTrue(rval.Success);
			Assert::IsTrue(bulk.Movement.Strafe);
			Assert::AreEqual(bulk.Movement.StrafeType, 1);
			Assert::AreEqual(bulk.Movement.JumpType, 2);
			Assert::IsTrue(bulk.Movement.Lgagst);
			Assert::IsTrue(bulk.Movement.AutoJump);
			Assert::IsTrue(bulk.Movement.Duckspam);
			Assert::IsTrue(bulk.Movement.Jumpbug);

		}

		TEST_METHOD(ParseField2)
		{
			std::string line = "-------|flrbud|--------|-|-|1|";
			srctas::FrameBulk bulk;
			auto rval = srctas::FrameBulk::ParseFrameBulk(line, bulk);
			Assert::IsTrue(rval.Success);
			Assert::IsTrue(bulk.Forward);
			Assert::IsTrue(bulk.Left);
			Assert::IsTrue(bulk.Right);
			Assert::IsTrue(bulk.Back);
			Assert::IsTrue(bulk.Up);
			Assert::IsTrue(bulk.Down);
		}

		TEST_METHOD(ParseField3)
		{
			std::string line = "-------|------|jdu12rws|-|-|1|";
			srctas::FrameBulk bulk;
			auto rval = srctas::FrameBulk::ParseFrameBulk(line, bulk);
			Assert::IsTrue(rval.Success);
			Assert::IsTrue(bulk.Jump);
			Assert::IsTrue(bulk.Duck);
			Assert::IsTrue(bulk.Use);
			Assert::IsTrue(bulk.Attack1);
			Assert::IsTrue(bulk.Attack2);
			Assert::IsTrue(bulk.Reload);
			Assert::IsTrue(bulk.Walk);
			Assert::IsTrue(bulk.Sprint);
		}

		TEST_METHOD(ParseAim1)
		{
			std::string line = "-------|------|--------|1.0 2.1 3 4|-|1|";
			srctas::FrameBulk bulk;
			auto rval = srctas::FrameBulk::ParseFrameBulk(line, bulk);
			Assert::IsTrue(rval.Success);
			Assert::AreEqual(bulk.AimPitch, 1.0);
			Assert::AreEqual(bulk.AimYaw.Yaw, 2.1);
			Assert::IsFalse(bulk.AimYaw.Auto);
			Assert::AreEqual(bulk.AimFrames, 3);
			Assert::AreEqual(bulk.Cone, 4);
		}

		TEST_METHOD(ParseAim2)
		{
			std::string line = "-------|------|--------|1.0 2.1 3|-|1|";
			srctas::FrameBulk bulk;
			auto rval = srctas::FrameBulk::ParseFrameBulk(line, bulk);
			Assert::IsTrue(rval.Success);
			Assert::AreEqual(bulk.AimPitch, 1.0);
			Assert::AreEqual(bulk.AimYaw.Yaw, 2.1);
			Assert::IsFalse(bulk.AimYaw.Auto);
			Assert::AreEqual(bulk.AimFrames, 3);
			Assert::AreEqual(bulk.Cone, 0);
		}

		TEST_METHOD(ParseAimYawAuto)
		{
			std::string line = "-------|------|--------|1.0 a 3|-|1|";
			srctas::FrameBulk bulk;
			auto rval = srctas::FrameBulk::ParseFrameBulk(line, bulk);
			Assert::IsTrue(rval.Success);
			Assert::AreEqual(bulk.AimPitch, 1.0);
			Assert::IsTrue(bulk.AimYaw.Auto);
			Assert::AreEqual(bulk.AimFrames, 3);
			Assert::AreEqual(bulk.Cone, 0);
		}

		TEST_METHOD(ParseStrafeYaw)
		{
			std::string line = "-------|------|--------|-|12.3|1|";
			srctas::FrameBulk bulk;
			auto rval = srctas::FrameBulk::ParseFrameBulk(line, bulk);
			Assert::IsTrue(rval.Success);
			Assert::IsTrue(bulk.Movement.StrafeYawSet);
			Assert::AreEqual(bulk.Movement.StrafeYaw, 12.3);
		}

		TEST_METHOD(ParseCommand)
		{
			std::string line = "-------|------|--------|-|-|1|echo Hello world!";
			srctas::FrameBulk bulk;
			auto rval = srctas::FrameBulk::ParseFrameBulk(line, bulk);
			Assert::IsTrue(rval.Success);
			Assert::AreEqual(bulk.Commands, std::string("echo Hello world!"));
		}
	};
}
