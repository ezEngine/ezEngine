#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdMath.h>

namespace
{
  ezSimdVec4f SimdDegree(float fDegree)
  {
    return ezSimdVec4f(ezAngle::MakeFromDegree(fDegree));
  }
} // namespace

EZ_CREATE_SIMPLE_TEST(SimdMath, SimdMath)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Exp")
  {
    float testVals[] = {0.0f, 1.0f, 2.0f};
    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(testVals); ++i)
    {
      const float v = testVals[i];
      const float r = ezMath::Exp(v);
      EZ_TEST_BOOL(ezSimdMath::Exp(ezSimdVec4f(v)).IsEqual(ezSimdVec4f(r), 0.000001f).AllSet());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Ln")
  {
    float testVals[] = {1.0f, 2.7182818284f, 7.3890560989f};
    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(testVals); ++i)
    {
      const float v = testVals[i];
      const float r = ezMath::Ln(v);
      EZ_TEST_BOOL(ezSimdMath::Ln(ezSimdVec4f(v)).IsEqual(ezSimdVec4f(r), 0.000001f).AllSet());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Log2")
  {
    float testVals[] = {1.0f, 2.0f, 4.0f};
    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(testVals); ++i)
    {
      const float v = testVals[i];
      const float r = ezMath::Log2(v);
      EZ_TEST_BOOL(ezSimdMath::Log2(ezSimdVec4f(v)).IsEqual(ezSimdVec4f(r), 0.000001f).AllSet());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Log2i")
  {
    int testVals[] = {0, 1, 2, 3, 4, 6, 7, 8};
    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(testVals); ++i)
    {
      const int v = testVals[i];
      const int r = ezMath::Log2i(v);
      EZ_TEST_BOOL((ezSimdMath::Log2i(ezSimdVec4i(v)) == ezSimdVec4i(r)).AllSet());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Log10")
  {
    float testVals[] = {1.0f, 10.0f, 100.0f};
    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(testVals); ++i)
    {
      const float v = testVals[i];
      const float r = ezMath::Log10(v);
      EZ_TEST_BOOL(ezSimdMath::Log10(ezSimdVec4f(v)).IsEqual(ezSimdVec4f(r), 0.000001f).AllSet());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Pow2")
  {
    float testVals[] = {0.0f, 1.0f, 2.0f};
    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(testVals); ++i)
    {
      const float v = testVals[i];
      const float r = ezMath::Pow2(v);
      EZ_TEST_BOOL(ezSimdMath::Pow2(ezSimdVec4f(v)).IsEqual(ezSimdVec4f(r), 0.000001f).AllSet());
    }
  }

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
    EZ_TEST_BOOL((ezSimdMath::Tan(SimdDegree(89.9999f)) > ezSimdVec4f(100000.0f)).AllSet());

    // Testing the period of tan(x) centered at 0 and the adjacent ones
    ezAngle angle = ezAngle::MakeFromDegree(-89.0f);
    while (angle.GetDegree() < 89.0f)
    {
      ezSimdVec4f simdAngle(angle.GetRadian());

      ezSimdVec4f fTan = ezSimdMath::Tan(simdAngle);
      ezSimdVec4f fTanPrev = ezSimdMath::Tan(SimdDegree(angle.GetDegree() - 180.0f));
      ezSimdVec4f fTanNext = ezSimdMath::Tan(SimdDegree(angle.GetDegree() + 180.0f));
      ezSimdVec4f fSin = ezSimdMath::Sin(simdAngle);
      ezSimdVec4f fCos = ezSimdMath::Cos(simdAngle);

      EZ_TEST_BOOL((fTan - fTanPrev).IsEqual(ezSimdVec4f::ZeroVector(), 0.002f).AllSet());
      EZ_TEST_BOOL((fTan - fTanNext).IsEqual(ezSimdVec4f::ZeroVector(), 0.002f).AllSet());
      EZ_TEST_BOOL((fTan - fSin.CompDiv(fCos)).IsEqual(ezSimdVec4f::ZeroVector(), 0.0005f).AllSet());
      angle += ezAngle::MakeFromDegree(1.234f);
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
