#include <FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdMath.h>

namespace
{
  ezSimdVec4f SimdDegree(float degree)
  {
    return ezSimdVec4f(ezAngle::Degree(degree));
  }
}

EZ_CREATE_SIMPLE_TEST(SimdMath, SimdMath)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Sin")
  {
    EZ_TEST_BOOL(ezSimdMath::Sin(SimdDegree(0.0f)).IsEqual(ezSimdVec4f(0.0f), 0.000001f).AllSet());
    EZ_TEST_BOOL(ezSimdMath::Sin(SimdDegree(90.0f)).IsEqual(ezSimdVec4f(1.0f), 0.000001f).AllSet());
    EZ_TEST_BOOL(ezSimdMath::Sin(SimdDegree(180.0f)).IsEqual(ezSimdVec4f(0.0f), 0.000001f).AllSet());
    EZ_TEST_BOOL(ezSimdMath::Sin(SimdDegree(270.0f)).IsEqual(ezSimdVec4f(-1.0f), 0.000001f).AllSet());

    EZ_TEST_BOOL(ezSimdMath::Sin(SimdDegree(45.0f)).IsEqual(ezSimdVec4f(0.7071067f), 0.000001f).AllSet());
    EZ_TEST_BOOL(ezSimdMath::Sin(SimdDegree(135.0f)).IsEqual(ezSimdVec4f(0.7071067f), 0.000001f).AllSet());
    EZ_TEST_BOOL(ezSimdMath::Sin(SimdDegree(225.0f)).IsEqual(ezSimdVec4f(-0.7071067f), 0.000001f).AllSet());
    EZ_TEST_BOOL(ezSimdMath::Sin(SimdDegree(315.0f)).IsEqual(ezSimdVec4f(-0.7071067f), 0.000001f).AllSet());
  }

  /*
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Cos")
  {
    EZ_TEST_BOOL(ezMath::Cos(ezAngle::Degree(0.0f)), 1.0f, 0.000001f);
    EZ_TEST_BOOL(ezMath::Cos(ezAngle::Degree(90.0f)), 0.0f, 0.000001f);
    EZ_TEST_BOOL(ezMath::Cos(ezAngle::Degree(180.0f)), -1.0f, 0.000001f);
    EZ_TEST_BOOL(ezMath::Cos(ezAngle::Degree(270.0f)), 0.0f, 0.000001f);

    EZ_TEST_BOOL(ezMath::Cos(ezAngle::Degree(45.0f)), 0.7071067f, 0.000001f);
    EZ_TEST_BOOL(ezMath::Cos(ezAngle::Degree(135.0f)), -0.7071067f, 0.000001f);
    EZ_TEST_BOOL(ezMath::Cos(ezAngle::Degree(225.0f)), -0.7071067f, 0.000001f);
    EZ_TEST_BOOL(ezMath::Cos(ezAngle::Degree(315.0f)), 0.7071067f, 0.000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Tan")
  {
    EZ_TEST_BOOL(ezMath::Tan(ezAngle::Degree(0.0f)), 0.0f, 0.000001f);
    EZ_TEST_BOOL(ezMath::Tan(ezAngle::Degree(45.0f)), 1.0f, 0.000001f);
    EZ_TEST_BOOL(ezMath::Tan(ezAngle::Degree(-45.0f)), -1.0f, 0.000001f);
    EZ_TEST_BOOL(ezMath::Tan(ezAngle::Degree(90.00001f)) < 1000000.0f);
    EZ_TEST_BOOL(ezMath::Tan(ezAngle::Degree(89.99999f)) > 1000000.0f);

    // Testing the period of tan(x) centered at 0 and the adjacent ones
    ezAngle angle = ezAngle::Degree(-89.0f);
    while (angle.GetDegree() < 89.0f)
    {
      float fTan = ezMath::Tan(angle);
      float fTanPrev = ezMath::Tan(ezAngle::Degree(angle.GetDegree() - 180.0f));
      float fTanNext = ezMath::Tan(ezAngle::Degree(angle.GetDegree() + 180.0f));
      float fSin = ezMath::Sin(angle);
      float fCos = ezMath::Cos(angle);

      EZ_TEST_BOOL(fTan - fTanPrev, 0.0f, 0.001f);
      EZ_TEST_BOOL(fTan - fTanNext, 0.0f, 0.001f);
      EZ_TEST_BOOL(fTan - (fSin / fCos), 0.0f, 0.0005f);
      angle += ezAngle::Degree(1.234f);
    }
  }
  */

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ASin")
  {
    EZ_TEST_BOOL(ezSimdMath::ASin(ezSimdVec4f(0.0f)).IsEqual(SimdDegree(0.0f), 0.0001f).AllSet());
    EZ_TEST_BOOL(ezSimdMath::ASin(ezSimdVec4f(1.0f)).IsEqual(SimdDegree(90.0f), 0.00001f).AllSet());
    EZ_TEST_BOOL(ezSimdMath::ASin(ezSimdVec4f(-1.0f)).IsEqual(SimdDegree(-90.0f), 0.00001f).AllSet());

    EZ_TEST_BOOL(ezSimdMath::ASin(ezSimdVec4f(0.7071067f)).IsEqual(SimdDegree(45.0f), 0.0001f).AllSet());
    EZ_TEST_BOOL(ezSimdMath::ASin(ezSimdVec4f(-0.7071067f)).IsEqual(SimdDegree(-45.0f), 0.0001f).AllSet());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ACos")
  {
    EZ_TEST_BOOL(ezSimdMath::ACos(ezSimdVec4f(0.0f)).IsEqual(SimdDegree(90.0f), 0.0001f).AllSet());
    EZ_TEST_BOOL(ezSimdMath::ACos(ezSimdVec4f(1.0f)).IsEqual(SimdDegree(0.0f), 0.00001f).AllSet());
    EZ_TEST_BOOL(ezSimdMath::ACos(ezSimdVec4f(-1.0f)).IsEqual(SimdDegree(180.0f), 0.0001f).AllSet());

    EZ_TEST_BOOL(ezSimdMath::ACos(ezSimdVec4f(0.7071067f)).IsEqual(SimdDegree(45.0f), 0.0001f).AllSet());
    EZ_TEST_BOOL(ezSimdMath::ACos(ezSimdVec4f(-0.7071067f)).IsEqual(SimdDegree(135.0f), 0.0001f).AllSet());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ATan")
  {
    EZ_TEST_BOOL(ezSimdMath::ATan(ezSimdVec4f(0.0f)).IsEqual(SimdDegree(0.0f), 0.0000001f).AllSet());
    EZ_TEST_BOOL(ezSimdMath::ATan(ezSimdVec4f(1.0f)).IsEqual(SimdDegree(45.0f), 0.00001f).AllSet());
    EZ_TEST_BOOL(ezSimdMath::ATan(ezSimdVec4f(-1.0f)).IsEqual(SimdDegree(-45.0f), 0.00001f).AllSet());
    EZ_TEST_BOOL(ezSimdMath::ATan(ezSimdVec4f(10000000.0f)).IsEqual(SimdDegree(90.0f), 0.00002f).AllSet());
    EZ_TEST_BOOL(ezSimdMath::ATan(ezSimdVec4f(-10000000.0f)).IsEqual(SimdDegree(-90.0f), 0.00002f).AllSet());
  }
}
