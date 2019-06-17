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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Cos")
  {
    EZ_TEST_BOOL(ezSimdMath::Cos(SimdDegree(0.0f)).IsEqual(ezSimdVec4f(1.0f), 0.000001f).AllSet());
    EZ_TEST_BOOL(ezSimdMath::Cos(SimdDegree(90.0f)).IsEqual(ezSimdVec4f(0.0f), 0.000001f).AllSet());
    EZ_TEST_BOOL(ezSimdMath::Cos(SimdDegree(180.0f)).IsEqual(ezSimdVec4f(-1.0f), 0.000001f).AllSet());
    EZ_TEST_BOOL(ezSimdMath::Cos(SimdDegree(270.0f)).IsEqual(ezSimdVec4f(0.0f), 0.000001f).AllSet());

    EZ_TEST_BOOL(ezSimdMath::Cos(SimdDegree(45.0f)).IsEqual(ezSimdVec4f(0.7071067f), 0.000001f).AllSet());
    EZ_TEST_BOOL(ezSimdMath::Cos(SimdDegree(135.0f)).IsEqual(ezSimdVec4f(-0.7071067f), 0.000001f).AllSet());
    EZ_TEST_BOOL(ezSimdMath::Cos(SimdDegree(225.0f)).IsEqual(ezSimdVec4f(-0.7071067f), 0.000001f).AllSet());
    EZ_TEST_BOOL(ezSimdMath::Cos(SimdDegree(315.0f)).IsEqual(ezSimdVec4f(0.7071067f), 0.000001f).AllSet());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Tan")
  {
    EZ_TEST_BOOL(ezSimdMath::Tan(SimdDegree(0.0f)).IsEqual(ezSimdVec4f(0.0f), 0.000001f).AllSet());
    EZ_TEST_BOOL(ezSimdMath::Tan(SimdDegree(45.0f)).IsEqual(ezSimdVec4f(1.0f), 0.000001f).AllSet());
    EZ_TEST_BOOL(ezSimdMath::Tan(SimdDegree(-45.0f)).IsEqual(ezSimdVec4f(-1.0f), 0.000001f).AllSet());
    EZ_TEST_BOOL((ezSimdMath::Tan(SimdDegree(90.00001f)) < ezSimdVec4f(1000000.0f)).AllSet());
    EZ_TEST_BOOL((ezSimdMath::Tan(SimdDegree(89.99999f)) > ezSimdVec4f(1000000.0f)).AllSet());

    // Testing the period of tan(x) centered at 0 and the adjacent ones
    ezAngle angle = ezAngle::Degree(-89.0f);
    while (angle.GetDegree() < 89.0f)
    {
      ezSimdVec4f simdAngle(angle.GetRadian());

      ezSimdVec4f fTan = ezSimdMath::Tan(simdAngle);
      ezSimdVec4f fTanPrev = ezSimdMath::Tan(SimdDegree(angle.GetDegree() - 180.0f));
      ezSimdVec4f fTanNext = ezSimdMath::Tan(SimdDegree(angle.GetDegree() + 180.0f));
      ezSimdVec4f fSin = ezSimdMath::Sin(simdAngle);
      ezSimdVec4f fCos = ezSimdMath::Cos(simdAngle);

      EZ_TEST_BOOL((fTan - fTanPrev).IsEqual(ezSimdVec4f::ZeroVector(), 0.001f).AllSet());
      EZ_TEST_BOOL((fTan - fTanNext).IsEqual(ezSimdVec4f::ZeroVector(), 0.001f).AllSet());
      EZ_TEST_BOOL((fTan - fSin.CompDiv(fCos)).IsEqual(ezSimdVec4f::ZeroVector(), 0.0005f).AllSet());
      angle += ezAngle::Degree(1.234f);
    }
  }

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
