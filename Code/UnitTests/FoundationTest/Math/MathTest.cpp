#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Math/Vec2.h>

/// ********************* Binary to Int conversion *********************
/// Most significant bit comes first.
/// Adapted from http://bytes.com/topic/c/answers/219656-literal-binary
///
/// Sample usage:
/// EZ_8BIT(01010101) == 85
/// EZ_16BIT(10101010, 01010101) == 43605
/// EZ_32BIT(10000000, 11111111, 10101010, 01010101) == 2164238933
/// ********************************************************************
#define OCT__(n) 0##n##LU

#define EZ_8BIT__(iBits)                                                                                                           \
  (((iBits & 000000001) ? 1 : 0) + ((iBits & 000000010) ? 2 : 0) + ((iBits & 000000100) ? 4 : 0) + ((iBits & 000001000) ? 8 : 0) + \
    ((iBits & 000010000) ? 16 : 0) + ((iBits & 000100000) ? 32 : 0) + ((iBits & 001000000) ? 64 : 0) + ((iBits & 010000000) ? 128 : 0))

#define EZ_8BIT(B) ((ezUInt8)EZ_8BIT__(OCT__(B)))

#define EZ_16BIT(B2, B1) (((ezUInt8)EZ_8BIT(B2) << 8) + EZ_8BIT(B1))

#define EZ_32BIT(B4, B3, B2, B1) \
  ((unsigned long)EZ_8BIT(B4) << 24) + ((unsigned long)EZ_8BIT(B3) << 16) + ((unsigned long)EZ_8BIT(B2) << 8) + ((unsigned long)EZ_8BIT(B1))

namespace
{
  struct UniqueInt
  {
    int i, id;
    UniqueInt(int i, int iId)
      : i(i)
      , id(iId)
    {
    }

    bool operator<(const UniqueInt& rh) { return this->i < rh.i; }

    bool operator>(const UniqueInt& rh) { return this->i > rh.i; }
  };
}; // namespace


EZ_CREATE_SIMPLE_TEST_GROUP(Math);

EZ_CREATE_SIMPLE_TEST(Math, General)
{
  // EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constants")
  //{
  //  // Macro test
  //  EZ_TEST_BOOL(EZ_8BIT(01010101) == 85);
  //  EZ_TEST_BOOL(EZ_16BIT(10101010, 01010101) == 43605);
  //  EZ_TEST_BOOL(EZ_32BIT(10000000, 11111111, 10101010, 01010101) == 2164238933);

  //  // Infinity test
  //  //                           Sign:_
  //  //                       Exponent: _______  _
  //  //                       Fraction:           _______  ________  ________
  //  ezIntFloatUnion uInf = { EZ_32BIT(01111111, 10000000, 00000000, 00000000) };
  //  EZ_TEST_BOOL(uInf.f == ezMath::FloatInfinity());

  //  // FloatMax_Pos test
  //  ezIntFloatUnion uMax = { EZ_32BIT(01111111, 01111111, 11111111, 11111111) };
  //  EZ_TEST_BOOL(uMax.f == ezMath::FloatMax_Pos());

  //  // FloatMax_Neg test
  //  ezIntFloatUnion uMin = { EZ_32BIT(11111111, 01111111, 11111111, 11111111) };
  //  EZ_TEST_BOOL(uMin.f == ezMath::FloatMax_Neg());
  //}

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Sin")
  {
    EZ_TEST_FLOAT(ezMath::Sin(ezAngle::MakeFromDegree(0.0f)), 0.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Sin(ezAngle::MakeFromDegree(90.0f)), 1.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Sin(ezAngle::MakeFromDegree(180.0f)), 0.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Sin(ezAngle::MakeFromDegree(270.0f)), -1.0f, 0.000001f);

    EZ_TEST_FLOAT(ezMath::Sin(ezAngle::MakeFromDegree(45.0f)), 0.7071067f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Sin(ezAngle::MakeFromDegree(135.0f)), 0.7071067f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Sin(ezAngle::MakeFromDegree(225.0f)), -0.7071067f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Sin(ezAngle::MakeFromDegree(315.0f)), -0.7071067f, 0.000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Cos")
  {
    EZ_TEST_FLOAT(ezMath::Cos(ezAngle::MakeFromDegree(0.0f)), 1.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Cos(ezAngle::MakeFromDegree(90.0f)), 0.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Cos(ezAngle::MakeFromDegree(180.0f)), -1.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Cos(ezAngle::MakeFromDegree(270.0f)), 0.0f, 0.000001f);

    EZ_TEST_FLOAT(ezMath::Cos(ezAngle::MakeFromDegree(45.0f)), 0.7071067f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Cos(ezAngle::MakeFromDegree(135.0f)), -0.7071067f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Cos(ezAngle::MakeFromDegree(225.0f)), -0.7071067f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Cos(ezAngle::MakeFromDegree(315.0f)), 0.7071067f, 0.000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Tan")
  {
    EZ_TEST_FLOAT(ezMath::Tan(ezAngle::MakeFromDegree(0.0f)), 0.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Tan(ezAngle::MakeFromDegree(45.0f)), 1.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Tan(ezAngle::MakeFromDegree(-45.0f)), -1.0f, 0.000001f);
    EZ_TEST_BOOL(ezMath::Tan(ezAngle::MakeFromDegree(90.00001f)) < 1000000.0f);
    EZ_TEST_BOOL(ezMath::Tan(ezAngle::MakeFromDegree(89.9999f)) > 100000.0f);

    // Testing the period of tan(x) centered at 0 and the adjacent ones
    ezAngle angle = ezAngle::MakeFromDegree(-89.0f);
    while (angle.GetDegree() < 89.0f)
    {
      float fTan = ezMath::Tan(angle);
      float fTanPrev = ezMath::Tan(ezAngle::MakeFromDegree(angle.GetDegree() - 180.0f));
      float fTanNext = ezMath::Tan(ezAngle::MakeFromDegree(angle.GetDegree() + 180.0f));
      float fSin = ezMath::Sin(angle);
      float fCos = ezMath::Cos(angle);

      EZ_TEST_FLOAT(fTan - fTanPrev, 0.0f, 0.002f);
      EZ_TEST_FLOAT(fTan - fTanNext, 0.0f, 0.002f);
      EZ_TEST_FLOAT(fTan - (fSin / fCos), 0.0f, 0.0005f);
      angle += ezAngle::MakeFromDegree(1.234f);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ASin")
  {
    EZ_TEST_FLOAT(ezMath::ASin(0.0f).GetDegree(), 0.0f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::ASin(1.0f).GetDegree(), 90.0f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::ASin(-1.0f).GetDegree(), -90.0f, 0.00001f);

    EZ_TEST_FLOAT(ezMath::ASin(0.7071067f).GetDegree(), 45.0f, 0.0001f);
    EZ_TEST_FLOAT(ezMath::ASin(-0.7071067f).GetDegree(), -45.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ACos")
  {
    EZ_TEST_FLOAT(ezMath::ACos(0.0f).GetDegree(), 90.0f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::ACos(1.0f).GetDegree(), 0.0f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::ACos(-1.0f).GetDegree(), 180.0f, 0.0001f);

    EZ_TEST_FLOAT(ezMath::ACos(0.7071067f).GetDegree(), 45.0f, 0.0001f);
    EZ_TEST_FLOAT(ezMath::ACos(-0.7071067f).GetDegree(), 135.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ATan")
  {
    EZ_TEST_FLOAT(ezMath::ATan(0.0f).GetDegree(), 0.0f, 0.0000001f);
    EZ_TEST_FLOAT(ezMath::ATan(1.0f).GetDegree(), 45.0f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::ATan(-1.0f).GetDegree(), -45.0f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::ATan(10000000.0f).GetDegree(), 90.0f, 0.00002f);
    EZ_TEST_FLOAT(ezMath::ATan(-10000000.0f).GetDegree(), -90.0f, 0.00002f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ATan2")
  {
    for (float fScale = 0.125f; fScale < 1000000.0f; fScale *= 2.0f)
    {
      EZ_TEST_FLOAT(ezMath::ATan2(0.0f, fScale).GetDegree(), 0.0f, 0.0000001f);
      EZ_TEST_FLOAT(ezMath::ATan2(fScale, fScale).GetDegree(), 45.0f, 0.00001f);
      EZ_TEST_FLOAT(ezMath::ATan2(fScale, 0.0f).GetDegree(), 90.0f, 0.00001f);
      EZ_TEST_FLOAT(ezMath::ATan2(-fScale, fScale).GetDegree(), -45.0f, 0.00001f);
      EZ_TEST_FLOAT(ezMath::ATan2(-fScale, 0.0f).GetDegree(), -90.0f, 0.00001f);
      EZ_TEST_FLOAT(ezMath::ATan2(0.0f, -fScale).GetDegree(), 180.0f, 0.0001f);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Exp")
  {
    EZ_TEST_FLOAT(1.0f, ezMath::Exp(0.0f), 0.000001f);
    EZ_TEST_FLOAT(2.7182818284f, ezMath::Exp(1.0f), 0.000001f);
    EZ_TEST_FLOAT(7.3890560989f, ezMath::Exp(2.0f), 0.000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Ln")
  {
    EZ_TEST_FLOAT(0.0f, ezMath::Ln(1.0f), 0.000001f);
    EZ_TEST_FLOAT(1.0f, ezMath::Ln(2.7182818284f), 0.000001f);
    EZ_TEST_FLOAT(2.0f, ezMath::Ln(7.3890560989f), 0.000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Log2")
  {
    EZ_TEST_FLOAT(0.0f, ezMath::Log2(1.0f), 0.000001f);
    EZ_TEST_FLOAT(1.0f, ezMath::Log2(2.0f), 0.000001f);
    EZ_TEST_FLOAT(2.0f, ezMath::Log2(4.0f), 0.000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Log2i")
  {
    EZ_TEST_BOOL(ezMath::Log2i(0) == ezUInt32(-1));
    EZ_TEST_BOOL(ezMath::Log2i(1) == 0);
    EZ_TEST_BOOL(ezMath::Log2i(2) == 1);
    EZ_TEST_BOOL(ezMath::Log2i(3) == 1);
    EZ_TEST_BOOL(ezMath::Log2i(4) == 2);
    EZ_TEST_BOOL(ezMath::Log2i(6) == 2);
    EZ_TEST_BOOL(ezMath::Log2i(7) == 2);
    EZ_TEST_BOOL(ezMath::Log2i(8) == 3);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Log10")
  {
    EZ_TEST_FLOAT(0.0f, ezMath::Log10(1.0f), 0.000001f);
    EZ_TEST_FLOAT(1.0f, ezMath::Log10(10.0f), 0.000001f);
    EZ_TEST_FLOAT(2.0f, ezMath::Log10(100.0f), 0.000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Log")
  {
    EZ_TEST_FLOAT(0.0f, ezMath::Log(2.7182818284f, 1.0f), 0.000001f);
    EZ_TEST_FLOAT(1.0f, ezMath::Log(2.7182818284f, 2.7182818284f), 0.000001f);
    EZ_TEST_FLOAT(2.0f, ezMath::Log(2.7182818284f, 7.3890560989f), 0.000001f);

    EZ_TEST_FLOAT(0.0f, ezMath::Log(2.0f, 1.0f), 0.000001f);
    EZ_TEST_FLOAT(1.0f, ezMath::Log(2.0f, 2.0f), 0.000001f);
    EZ_TEST_FLOAT(2.0f, ezMath::Log(2.0f, 4.0f), 0.000001f);

    EZ_TEST_FLOAT(0.0f, ezMath::Log(10.0f, 1.0f), 0.000001f);
    EZ_TEST_FLOAT(1.0f, ezMath::Log(10.0f, 10.0f), 0.000001f);
    EZ_TEST_FLOAT(2.0f, ezMath::Log(10.0f, 100.0f), 0.000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Pow2")
  {
    EZ_TEST_FLOAT(1.0f, ezMath::Pow2(0.0f), 0.000001f);
    EZ_TEST_FLOAT(2.0f, ezMath::Pow2(1.0f), 0.000001f);
    EZ_TEST_FLOAT(4.0f, ezMath::Pow2(2.0f), 0.000001f);

    EZ_TEST_BOOL(ezMath::Pow2(0) == 1);
    EZ_TEST_BOOL(ezMath::Pow2(1) == 2);
    EZ_TEST_BOOL(ezMath::Pow2(2) == 4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Pow")
  {
    EZ_TEST_FLOAT(1.0f, ezMath::Pow(3.0f, 0.0f), 0.000001f);
    EZ_TEST_FLOAT(3.0f, ezMath::Pow(3.0f, 1.0f), 0.000001f);
    EZ_TEST_FLOAT(9.0f, ezMath::Pow(3.0f, 2.0f), 0.000001f);

    EZ_TEST_BOOL(ezMath::Pow(3, 0) == 1);
    EZ_TEST_BOOL(ezMath::Pow(3, 1) == 3);
    EZ_TEST_BOOL(ezMath::Pow(3, 2) == 9);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Square")
  {
    EZ_TEST_FLOAT(0.0f, ezMath::Square(0.0f), 0.000001f);
    EZ_TEST_FLOAT(1.0f, ezMath::Square(1.0f), 0.000001f);
    EZ_TEST_FLOAT(4.0f, ezMath::Square(2.0f), 0.000001f);
    EZ_TEST_FLOAT(4.0f, ezMath::Square(-2.0f), 0.000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Sqrt (float)")
  {
    EZ_TEST_FLOAT(0.0f, ezMath::Sqrt(0.0f), 0.000001f);
    EZ_TEST_FLOAT(1.0f, ezMath::Sqrt(1.0f), 0.000001f);
    EZ_TEST_FLOAT(2.0f, ezMath::Sqrt(4.0f), 0.000001f);
    EZ_TEST_FLOAT(4.0f, ezMath::Sqrt(16.0f), 0.000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Sqrt (double)")
  {
    EZ_TEST_DOUBLE(0.0, ezMath::Sqrt(0.0), 0.000001);
    EZ_TEST_DOUBLE(1.0, ezMath::Sqrt(1.0), 0.000001);
    EZ_TEST_DOUBLE(2.0, ezMath::Sqrt(4.0), 0.000001);
    EZ_TEST_DOUBLE(4.0, ezMath::Sqrt(16.0), 0.000001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Root")
  {
    EZ_TEST_FLOAT(3.0f, ezMath::Root(27.0f, 3.0f), 0.000001f);
    EZ_TEST_FLOAT(3.0f, ezMath::Root(81.0f, 4.0f), 0.000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Sign")
  {
    EZ_TEST_FLOAT(0.0f, ezMath::Sign(0.0f), 0.00000001f);
    EZ_TEST_FLOAT(1.0f, ezMath::Sign(0.01f), 0.00000001f);
    EZ_TEST_FLOAT(-1.0f, ezMath::Sign(-0.01f), 0.00000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Abs")
  {
    EZ_TEST_FLOAT(0.0f, ezMath::Abs(0.0f), 0.00000001f);
    EZ_TEST_FLOAT(20.0f, ezMath::Abs(20.0f), 0.00000001f);
    EZ_TEST_FLOAT(20.0f, ezMath::Abs(-20.0f), 0.00000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Min")
  {
    EZ_TEST_FLOAT(0.0f, ezMath::Min(0.0f, 23.0f), 0.00000001f);
    EZ_TEST_FLOAT(-23.0f, ezMath::Min(0.0f, -23.0f), 0.00000001f);

    EZ_TEST_BOOL(ezMath::Min(1, 2, 3) == 1);
    EZ_TEST_BOOL(ezMath::Min(4, 2, 3) == 2);
    EZ_TEST_BOOL(ezMath::Min(4, 5, 3) == 3);

    EZ_TEST_BOOL(ezMath::Min(1, 2, 3, 4) == 1);
    EZ_TEST_BOOL(ezMath::Min(5, 2, 3, 4) == 2);
    EZ_TEST_BOOL(ezMath::Min(5, 6, 3, 4) == 3);
    EZ_TEST_BOOL(ezMath::Min(5, 6, 7, 4) == 4);

    EZ_TEST_BOOL(ezMath::Min(UniqueInt(1, 0), UniqueInt(1, 1)).id == 0);
    EZ_TEST_BOOL(ezMath::Min(UniqueInt(1, 1), UniqueInt(1, 0)).id == 1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Max")
  {
    EZ_TEST_FLOAT(23.0f, ezMath::Max(0.0f, 23.0f), 0.00000001f);
    EZ_TEST_FLOAT(0.0f, ezMath::Max(0.0f, -23.0f), 0.00000001f);

    EZ_TEST_BOOL(ezMath::Max(1, 2, 3) == 3);
    EZ_TEST_BOOL(ezMath::Max(1, 2, 0) == 2);
    EZ_TEST_BOOL(ezMath::Max(1, 0, 0) == 1);

    EZ_TEST_BOOL(ezMath::Max(1, 2, 3, 4) == 4);
    EZ_TEST_BOOL(ezMath::Max(1, 2, 3, 0) == 3);
    EZ_TEST_BOOL(ezMath::Max(1, 2, 0, 0) == 2);
    EZ_TEST_BOOL(ezMath::Max(1, 0, 0, 0) == 1);

    EZ_TEST_BOOL(ezMath::Max(UniqueInt(1, 0), UniqueInt(1, 1)).id == 0);
    EZ_TEST_BOOL(ezMath::Max(UniqueInt(1, 1), UniqueInt(1, 0)).id == 1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Clamp")
  {
    EZ_TEST_FLOAT(15.0f, ezMath::Clamp(23.0f, 12.0f, 15.0f), 0.00000001f);
    EZ_TEST_FLOAT(12.0f, ezMath::Clamp(3.0f, 12.0f, 15.0f), 0.00000001f);
    EZ_TEST_FLOAT(14.0f, ezMath::Clamp(14.0f, 12.0f, 15.0f), 0.00000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Saturate")
  {
    EZ_TEST_FLOAT(0.0f, ezMath::Saturate(-1.5f), 0.00000001f);
    EZ_TEST_FLOAT(0.5f, ezMath::Saturate(0.5f), 0.00000001f);
    EZ_TEST_FLOAT(1.0f, ezMath::Saturate(12345.0f), 0.00000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Floor")
  {
    EZ_TEST_BOOL(12 == ezMath::Floor(12.34f));
    EZ_TEST_BOOL(-13 == ezMath::Floor(-12.34f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Ceil")
  {
    EZ_TEST_BOOL(13 == ezMath::Ceil(12.34f));
    EZ_TEST_BOOL(-12 == ezMath::Ceil(-12.34f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RoundDown (float)")
  {
    EZ_TEST_FLOAT(10.0f, ezMath::RoundDown(12.34f, 5.0f), 0.0000001f);
    EZ_TEST_FLOAT(-15.0f, ezMath::RoundDown(-12.34f, 5.0f), 0.0000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RoundUp (float)")
  {
    EZ_TEST_FLOAT(15.0f, ezMath::RoundUp(12.34f, 5.0f), 0.0000001f);
    EZ_TEST_FLOAT(-10.0f, ezMath::RoundUp(-12.34f, 5.0f), 0.0000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RoundDown (double)")
  {
    EZ_TEST_DOUBLE(10.0, ezMath::RoundDown(12.34, 5.0), 0.0000001);
    EZ_TEST_DOUBLE(-15.0, ezMath::RoundDown(-12.34, 5.0), 0.0000001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RoundUp (double)")
  {
    EZ_TEST_DOUBLE(15.0, ezMath::RoundUp(12.34, 5.0), 0.0000001);
    EZ_TEST_DOUBLE(-10.0, ezMath::RoundUp(-12.34, 5.0), 0.0000001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Trunc")
  {
    EZ_TEST_BOOL(ezMath::Trunc(12.34f) == 12);
    EZ_TEST_BOOL(ezMath::Trunc(-12.34f) == -12);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FloatToInt")
  {
    EZ_TEST_BOOL(ezMath::FloatToInt(12.34f) == 12);
    EZ_TEST_BOOL(ezMath::FloatToInt(-12.34f) == -12);

#if EZ_DISABLED(EZ_PLATFORM_ARCH_X86) || (_MSC_VER <= 1916)
    EZ_TEST_BOOL(ezMath::FloatToInt(12000000000000.34) == 12000000000000);
    EZ_TEST_BOOL(ezMath::FloatToInt(-12000000000000.34) == -12000000000000);
#endif
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Round")
  {
    EZ_TEST_BOOL(ezMath::Round(12.34f) == 12);
    EZ_TEST_BOOL(ezMath::Round(-12.34f) == -12);

    EZ_TEST_BOOL(ezMath::Round(12.54f) == 13);
    EZ_TEST_BOOL(ezMath::Round(-12.54f) == -13);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RoundClosest (float)")
  {
    EZ_TEST_FLOAT(ezMath::RoundToMultiple(12.0f, 3.0f), 12.0f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::RoundToMultiple(-12.0f, 3.0f), -12.0f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::RoundToMultiple(12.34f, 7.0f), 14.0f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::RoundToMultiple(-12.34f, 7.0f), -14.0f, 0.00001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RoundClosest (double)")
  {
    EZ_TEST_DOUBLE(ezMath::RoundToMultiple(12.0, 3.0), 12.0, 0.00001);
    EZ_TEST_DOUBLE(ezMath::RoundToMultiple(-12.0, 3.0), -12.0, 0.00001);
    EZ_TEST_DOUBLE(ezMath::RoundToMultiple(12.34, 7.0), 14.0, 0.00001);
    EZ_TEST_DOUBLE(ezMath::RoundToMultiple(-12.34, 7.0), -14.0, 0.00001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RoundUp (int)")
  {
    EZ_TEST_INT(ezMath::RoundUp(12, 7), 14);
    EZ_TEST_INT(ezMath::RoundUp(-12, 7), -7);
    EZ_TEST_INT(ezMath::RoundUp(16, 4), 16);
    EZ_TEST_INT(ezMath::RoundUp(-16, 4), -16);
    EZ_TEST_INT(ezMath::RoundUp(17, 4), 20);
    EZ_TEST_INT(ezMath::RoundUp(-17, 4), -16);
    EZ_TEST_INT(ezMath::RoundUp(15, 4), 16);
    EZ_TEST_INT(ezMath::RoundUp(-15, 4), -12);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RoundDown (int)")
  {
    EZ_TEST_INT(ezMath::RoundDown(12, 7), 7);
    EZ_TEST_INT(ezMath::RoundDown(-12, 7), -14);
    EZ_TEST_INT(ezMath::RoundDown(16, 4), 16);
    EZ_TEST_INT(ezMath::RoundDown(-16, 4), -16);
    EZ_TEST_INT(ezMath::RoundDown(17, 4), 16);
    EZ_TEST_INT(ezMath::RoundDown(-17, 4), -20);
    EZ_TEST_INT(ezMath::RoundDown(15, 4), 12);
    EZ_TEST_INT(ezMath::RoundDown(-15, 4), -16);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RoundUp (unsigned int)")
  {
    EZ_TEST_INT(ezMath::RoundUp(12u, 7), 14);
    EZ_TEST_INT(ezMath::RoundUp(16u, 4), 16);
    EZ_TEST_INT(ezMath::RoundUp(17u, 4), 20);
    EZ_TEST_INT(ezMath::RoundUp(15u, 4), 16);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RoundDown (unsigned int)")
  {
    EZ_TEST_INT(ezMath::RoundDown(12u, 7), 7);
    EZ_TEST_INT(ezMath::RoundDown(16u, 4), 16);
    EZ_TEST_INT(ezMath::RoundDown(17u, 4), 16);
    EZ_TEST_INT(ezMath::RoundDown(15u, 4), 12);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Fraction")
  {
    EZ_TEST_FLOAT(ezMath::Fraction(12.34f), 0.34f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::Fraction(-12.34f), -0.34f, 0.00001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Mod (float)")
  {
    EZ_TEST_FLOAT(2.34f, ezMath::Mod(12.34f, 2.5f), 0.000001f);
    EZ_TEST_FLOAT(-2.34f, ezMath::Mod(-12.34f, 2.5f), 0.000001f);

    EZ_TEST_FLOAT(2.34f, ezMath::Mod(12.34f, -2.5f), 0.000001f);
    EZ_TEST_FLOAT(-2.34f, ezMath::Mod(-12.34f, -2.5f), 0.000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Mod (double)")
  {
    EZ_TEST_DOUBLE(2.34, ezMath::Mod(12.34, 2.5), 0.000001);
    EZ_TEST_DOUBLE(-2.34, ezMath::Mod(-12.34, 2.5), 0.000001);

    EZ_TEST_DOUBLE(2.34, ezMath::Mod(12.34, -2.5), 0.000001);
    EZ_TEST_DOUBLE(-2.34, ezMath::Mod(-12.34, -2.5), 0.000001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Invert")
  {
    EZ_TEST_FLOAT(ezMath::Invert(1.0f), 1.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Invert(2.0f), 0.5f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Invert(4.0f), 0.25f, 0.000001f);

    EZ_TEST_FLOAT(ezMath::Invert(-1.0f), -1.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Invert(-2.0f), -0.5f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Invert(-4.0f), -0.25f, 0.000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Odd")
  {
    EZ_TEST_BOOL(ezMath::IsOdd(0) == false);
    EZ_TEST_BOOL(ezMath::IsOdd(1) == true);
    EZ_TEST_BOOL(ezMath::IsOdd(2) == false);
    EZ_TEST_BOOL(ezMath::IsOdd(-1) == true);
    EZ_TEST_BOOL(ezMath::IsOdd(-2) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Even")
  {
    EZ_TEST_BOOL(ezMath::IsEven(0) == true);
    EZ_TEST_BOOL(ezMath::IsEven(1) == false);
    EZ_TEST_BOOL(ezMath::IsEven(2) == true);
    EZ_TEST_BOOL(ezMath::IsEven(-1) == false);
    EZ_TEST_BOOL(ezMath::IsEven(-2) == true);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Swap")
  {
    ezInt32 a = 1;
    ezInt32 b = 2;
    ezMath::Swap(a, b);
    EZ_TEST_BOOL((a == 2) && (b == 1));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Lerp")
  {
    EZ_TEST_FLOAT(ezMath::Lerp(-5.0f, 5.0f, 0.5f), 0.0f, 0.000001);
    EZ_TEST_FLOAT(ezMath::Lerp(0.0f, 5.0f, 0.5f), 2.5f, 0.000001);
    EZ_TEST_FLOAT(ezMath::Lerp(-5.0f, 5.0f, 0.0f), -5.0f, 0.000001);
    EZ_TEST_FLOAT(ezMath::Lerp(-5.0f, 5.0f, 1.0f), 5.0f, 0.000001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Unlerp")
  {
    EZ_TEST_FLOAT(ezMath::Unlerp(-5.0f, 5.0f, 0.0f), 0.5f, 0.000001);
    EZ_TEST_FLOAT(ezMath::Unlerp(0.0f, 5.0f, 2.5f), 0.5f, 0.000001);
    EZ_TEST_FLOAT(ezMath::Unlerp(-5.0f, 5.0f, -5.0f), 0.0f, 0.000001);
    EZ_TEST_FLOAT(ezMath::Unlerp(-5.0f, 5.0f, 5.0f), 1.0f, 0.000001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Step")
  {
    EZ_TEST_FLOAT(ezMath::Step(0.5f, 0.4f), 1.0f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::Step(0.3f, 0.4f), 0.0f, 0.00001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SmoothStep")
  {
    // Only test values that must be true for any symmetric step function.
    // How should one test smoothness?
    for (int iScale = -19; iScale <= 19; iScale += 2)
    {
      EZ_TEST_FLOAT(ezMath::SmoothStep(0.0f * iScale, 0.1f * iScale, 0.4f * iScale), 0.0f, 0.000001);
      EZ_TEST_FLOAT(ezMath::SmoothStep(0.1f * iScale, 0.1f * iScale, 0.4f * iScale), 0.0f, 0.000001);
      EZ_TEST_FLOAT(ezMath::SmoothStep(0.4f * iScale, 0.1f * iScale, 0.4f * iScale), 1.0f, 0.000001);
      EZ_TEST_FLOAT(ezMath::SmoothStep(0.25f * iScale, 0.1f * iScale, 0.4f * iScale), 0.5f, 0.000001);
      EZ_TEST_FLOAT(ezMath::SmoothStep(0.5f * iScale, 0.1f * iScale, 0.4f * iScale), 1.0f, 0.000001);

      EZ_TEST_FLOAT(ezMath::SmoothStep(0.5f * iScale, 0.4f * iScale, 0.1f * iScale), 0.0f, 0.000001);
      EZ_TEST_FLOAT(ezMath::SmoothStep(0.4f * iScale, 0.4f * iScale, 0.1f * iScale), 0.0f, 0.000001);
      EZ_TEST_FLOAT(ezMath::SmoothStep(0.1f * iScale, 0.4f * iScale, 0.1f * iScale), 1.0f, 0.000001);
      EZ_TEST_FLOAT(ezMath::SmoothStep(0.25f * iScale, 0.1f * iScale, 0.4f * iScale), 0.5f, 0.000001);
      EZ_TEST_FLOAT(ezMath::SmoothStep(0.0f * iScale, 0.4f * iScale, 0.1f * iScale), 1.0f, 0.000001);

      // For edge1 == edge2 SmoothStep should behave like Step
      EZ_TEST_FLOAT(ezMath::SmoothStep(0.0f * iScale, 0.1f * iScale, 0.1f * iScale), iScale > 0 ? 0.0f : 1.0f, 0.000001);
      EZ_TEST_FLOAT(ezMath::SmoothStep(0.2f * iScale, 0.1f * iScale, 0.1f * iScale), iScale < 0 ? 0.0f : 1.0f, 0.000001);
    }

    EZ_TEST_FLOAT(ezMath::SmoothStep(0.2f, 0.0f, 1.0f), 0.104f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::SmoothStep(0.4f, 0.2f, 0.8f), 0.259259f, 0.00001f);

    EZ_TEST_FLOAT(ezMath::SmootherStep(0.2f, 0.0f, 1.0f), 0.05792f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::SmootherStep(0.4f, 0.2f, 0.8f), 0.209876f, 0.00001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsPowerOf")
  {
    EZ_TEST_BOOL(ezMath::IsPowerOf(4, 2) == true);
    EZ_TEST_BOOL(ezMath::IsPowerOf(5, 2) == false);
    EZ_TEST_BOOL(ezMath::IsPowerOf(0, 2) == false);
    EZ_TEST_BOOL(ezMath::IsPowerOf(1, 2) == true);

    EZ_TEST_BOOL(ezMath::IsPowerOf(4, 3) == false);
    EZ_TEST_BOOL(ezMath::IsPowerOf(3, 3) == true);
    EZ_TEST_BOOL(ezMath::IsPowerOf(1, 3) == true);
    EZ_TEST_BOOL(ezMath::IsPowerOf(27, 3) == true);
    EZ_TEST_BOOL(ezMath::IsPowerOf(28, 3) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsPowerOf2")
  {
    EZ_TEST_BOOL(ezMath::IsPowerOf2(4) == true);
    EZ_TEST_BOOL(ezMath::IsPowerOf2(5) == false);
    EZ_TEST_BOOL(ezMath::IsPowerOf2(0) == false);
    EZ_TEST_BOOL(ezMath::IsPowerOf2(1) == true);
    EZ_TEST_BOOL(ezMath::IsPowerOf2(0x7FFFFFFFu) == false);
    EZ_TEST_BOOL(ezMath::IsPowerOf2(0x80000000u) == true);
    EZ_TEST_BOOL(ezMath::IsPowerOf2(0x80000001u) == false);
    EZ_TEST_BOOL(ezMath::IsPowerOf2(0xFFFFFFFFu) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "PowerOf2_Floor")
  {
    EZ_TEST_INT(ezMath::PowerOfTwo_Floor(64u), 64);
    EZ_TEST_INT(ezMath::PowerOfTwo_Floor(33u), 32);
    EZ_TEST_INT(ezMath::PowerOfTwo_Floor(4u), 4);
    EZ_TEST_INT(ezMath::PowerOfTwo_Floor(5u), 4);
    EZ_TEST_INT(ezMath::PowerOfTwo_Floor(1u), 1);
    EZ_TEST_INT(ezMath::PowerOfTwo_Floor(0x80000000), 0x80000000);
    EZ_TEST_INT(ezMath::PowerOfTwo_Floor(0x80000001), 0x80000000);
    // strange case...
    EZ_TEST_INT(ezMath::PowerOfTwo_Floor(0u), 1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "PowerOf2_Ceil")
  {
    EZ_TEST_INT(ezMath::PowerOfTwo_Ceil(64u), 64);
    EZ_TEST_INT(ezMath::PowerOfTwo_Ceil(33u), 64);
    EZ_TEST_INT(ezMath::PowerOfTwo_Ceil(4u), 4);
    EZ_TEST_INT(ezMath::PowerOfTwo_Ceil(5u), 8);
    EZ_TEST_INT(ezMath::PowerOfTwo_Ceil(1u), 1);
    EZ_TEST_INT(ezMath::PowerOfTwo_Ceil(0u), 1);
    EZ_TEST_INT(ezMath::PowerOfTwo_Ceil(0x7FFFFFFFu), 0x80000000);
    EZ_TEST_INT(ezMath::PowerOfTwo_Ceil(0x80000000), 0x80000000);
    // anything above 0x80000000 is undefined behavior due to how left-shift works
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GreatestCommonDivisor")
  {
    EZ_TEST_INT(ezMath::GreatestCommonDivisor(13, 13), 13);
    EZ_TEST_INT(ezMath::GreatestCommonDivisor(13, 0), 13);
    EZ_TEST_INT(ezMath::GreatestCommonDivisor(0, 637), 637);
    EZ_TEST_INT(ezMath::GreatestCommonDivisor(37, 600), 1);
    EZ_TEST_INT(ezMath::GreatestCommonDivisor(20, 100), 20);
    EZ_TEST_INT(ezMath::GreatestCommonDivisor(624129, 2061517), 18913);
  }


  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsEqual")
  {
    EZ_TEST_BOOL(ezMath::IsEqual(1.0f, 0.999f, 0.01f) == true);
    EZ_TEST_BOOL(ezMath::IsEqual(1.0f, 1.001f, 0.01f) == true);
    EZ_TEST_BOOL(ezMath::IsEqual(1.0f, 0.999f, 0.0001f) == false);
    EZ_TEST_BOOL(ezMath::IsEqual(1.0f, 1.001f, 0.0001f) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "NaN_Infinity")
  {
    if (ezMath::SupportsNaN<ezMathTestType>())
    {
      EZ_TEST_BOOL(ezMath::IsNaN(ezMath::NaN<ezMathTestType>()) == true);

      EZ_TEST_BOOL(ezMath::Infinity<ezMathTestType>() == ezMath::Infinity<ezMathTestType>() - (ezMathTestType)1);
      EZ_TEST_BOOL(ezMath::Infinity<ezMathTestType>() == ezMath::Infinity<ezMathTestType>() + (ezMathTestType)1);

      EZ_TEST_BOOL(ezMath::IsNaN(ezMath::Infinity<ezMathTestType>() - ezMath::Infinity<ezMathTestType>()));

      EZ_TEST_BOOL(!ezMath::IsFinite(ezMath::Infinity<ezMathTestType>()));
      EZ_TEST_BOOL(!ezMath::IsFinite(-ezMath::Infinity<ezMathTestType>()));
      EZ_TEST_BOOL(!ezMath::IsFinite(ezMath::NaN<ezMathTestType>()));
      EZ_TEST_BOOL(!ezMath::IsNaN(ezMath::Infinity<ezMathTestType>()));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsInRange")
  {
    EZ_TEST_BOOL(ezMath::IsInRange(1.0f, 0.0f, 2.0f) == true);
    EZ_TEST_BOOL(ezMath::IsInRange(1.0f, 0.0f, 1.0f) == true);
    EZ_TEST_BOOL(ezMath::IsInRange(1.0f, 1.0f, 2.0f) == true);
    EZ_TEST_BOOL(ezMath::IsInRange(0.0f, 1.0f, 2.0f) == false);
    EZ_TEST_BOOL(ezMath::IsInRange(3.0f, 0.0f, 2.0f) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsZero")
  {
    EZ_TEST_BOOL(ezMath::IsZero(0.009f, 0.01f) == true);
    EZ_TEST_BOOL(ezMath::IsZero(0.001f, 0.01f) == true);
    EZ_TEST_BOOL(ezMath::IsZero(0.009f, 0.0001f) == false);
    EZ_TEST_BOOL(ezMath::IsZero(0.001f, 0.0001f) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ColorFloatToByte")
  {
    EZ_TEST_INT(ezMath::ColorFloatToByte(ezMath::NaN<float>()), 0);
    EZ_TEST_INT(ezMath::ColorFloatToByte(-1.0f), 0);
    EZ_TEST_INT(ezMath::ColorFloatToByte(0.0f), 0);
    EZ_TEST_INT(ezMath::ColorFloatToByte(0.4f), 102);
    EZ_TEST_INT(ezMath::ColorFloatToByte(1.0f), 255);
    EZ_TEST_INT(ezMath::ColorFloatToByte(1.5f), 255);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ColorFloatToShort")
  {
    EZ_TEST_INT(ezMath::ColorFloatToShort(ezMath::NaN<float>()), 0);
    EZ_TEST_INT(ezMath::ColorFloatToShort(-1.0f), 0);
    EZ_TEST_INT(ezMath::ColorFloatToShort(0.0f), 0);
    EZ_TEST_INT(ezMath::ColorFloatToShort(0.4f), 26214);
    EZ_TEST_INT(ezMath::ColorFloatToShort(1.0f), 65535);
    EZ_TEST_INT(ezMath::ColorFloatToShort(1.5f), 65535);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ColorFloatToSignedByte")
  {
    EZ_TEST_INT(ezMath::ColorFloatToSignedByte(ezMath::NaN<float>()), 0);
    EZ_TEST_INT(ezMath::ColorFloatToSignedByte(-1.0f), -127);
    EZ_TEST_INT(ezMath::ColorFloatToSignedByte(0.0f), 0);
    EZ_TEST_INT(ezMath::ColorFloatToSignedByte(0.4f), 51);
    EZ_TEST_INT(ezMath::ColorFloatToSignedByte(1.0f), 127);
    EZ_TEST_INT(ezMath::ColorFloatToSignedByte(1.5f), 127);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ColorFloatToSignedShort")
  {
    EZ_TEST_INT(ezMath::ColorFloatToSignedShort(ezMath::NaN<float>()), 0);
    EZ_TEST_INT(ezMath::ColorFloatToSignedShort(-1.0f), -32767);
    EZ_TEST_INT(ezMath::ColorFloatToSignedShort(0.0f), 0);
    EZ_TEST_INT(ezMath::ColorFloatToSignedShort(0.4f), 13107);
    EZ_TEST_INT(ezMath::ColorFloatToSignedShort(0.5f), 16384);
    EZ_TEST_INT(ezMath::ColorFloatToSignedShort(1.0f), 32767);
    EZ_TEST_INT(ezMath::ColorFloatToSignedShort(1.5f), 32767);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ColorByteToFloat")
  {
    EZ_TEST_FLOAT(ezMath::ColorByteToFloat(0), 0.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::ColorByteToFloat(128), 0.501960784f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::ColorByteToFloat(255), 1.0f, 0.000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ColorShortToFloat")
  {
    EZ_TEST_FLOAT(ezMath::ColorShortToFloat(0), 0.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::ColorShortToFloat(32768), 0.5000076f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::ColorShortToFloat(65535), 1.0f, 0.000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ColorSignedByteToFloat")
  {
    EZ_TEST_FLOAT(ezMath::ColorSignedByteToFloat(-128), -1.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::ColorSignedByteToFloat(-127), -1.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::ColorSignedByteToFloat(0), 0.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::ColorSignedByteToFloat(64), 0.50393700787f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::ColorSignedByteToFloat(127), 1.0f, 0.000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ColorSignedShortToFloat")
  {
    EZ_TEST_FLOAT(ezMath::ColorSignedShortToFloat(-32768), -1.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::ColorSignedShortToFloat(-32767), -1.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::ColorSignedShortToFloat(0), 0.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::ColorSignedShortToFloat(16384), 0.50001526f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::ColorSignedShortToFloat(32767), 1.0f, 0.000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "EvaluateBezierCurve")
  {
    // Determined through the scientific method of manually comparing the result of the function with an online Bezier curve generator:
    // https://www.desmos.com/calculator/cahqdxeshd
    const ezVec2 res[] = {ezVec2(1, 5), ezVec2(0.893f, 4.455f), ezVec2(1.112f, 4.008f), ezVec2(1.557f, 3.631f), ezVec2(2.136f, 3.304f), ezVec2(2.750f, 3.000f),
      ezVec2(3.303f, 2.695f), ezVec2(3.701f, 2.368f), ezVec2(3.847f, 1.991f), ezVec2(3.645f, 1.543f), ezVec2(3, 1)};

    const float step = 1.0f / (EZ_ARRAY_SIZE(res) - 1);
    for (int i = 0; i < EZ_ARRAY_SIZE(res); ++i)
    {
      const ezVec2 r = ezMath::EvaluateBezierCurve<ezVec2>(step * i, ezVec2(1, 5), ezVec2(0, 3), ezVec2(6, 3), ezVec2(3, 1));
      EZ_TEST_VEC2(r, res[i], 0.002f);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FirstBitLow")
  {
    EZ_TEST_INT(ezMath::FirstBitLow(ezUInt32(0b1111)), 0);
    EZ_TEST_INT(ezMath::FirstBitLow(ezUInt32(0b1110)), 1);
    EZ_TEST_INT(ezMath::FirstBitLow(ezUInt32(0b1100)), 2);
    EZ_TEST_INT(ezMath::FirstBitLow(ezUInt32(0b1000)), 3);
    EZ_TEST_INT(ezMath::FirstBitLow(ezUInt32(0xFFFFFFFF)), 0);

    EZ_TEST_INT(ezMath::FirstBitLow(ezUInt64(0xFF000000FF00000F)), 0);
    EZ_TEST_INT(ezMath::FirstBitLow(ezUInt64(0xFF000000FF00000E)), 1);
    EZ_TEST_INT(ezMath::FirstBitLow(ezUInt64(0xFF000000FF00000C)), 2);
    EZ_TEST_INT(ezMath::FirstBitLow(ezUInt64(0xFF000000FF000008)), 3);
    EZ_TEST_INT(ezMath::FirstBitLow(ezUInt64(0xFFFFFFFFFFFFFFFF)), 0);

    // Edge cases specifically for 32-bit systems where upper and lower 32-bit are handled individually.
    EZ_TEST_INT(ezMath::FirstBitLow(ezUInt64(0x00000000FFFFFFFF)), 0);
    EZ_TEST_INT(ezMath::FirstBitLow(ezUInt64(0xFFFFFFFF00000000)), 32);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FirstBitHigh")
  {
    EZ_TEST_INT(ezMath::FirstBitHigh(ezUInt32(0b1111)), 3);
    EZ_TEST_INT(ezMath::FirstBitHigh(ezUInt32(0b0111)), 2);
    EZ_TEST_INT(ezMath::FirstBitHigh(ezUInt32(0b0011)), 1);
    EZ_TEST_INT(ezMath::FirstBitHigh(ezUInt32(0b0001)), 0);
    EZ_TEST_INT(ezMath::FirstBitHigh(ezUInt32(0xFFFFFFFF)), 31);

    EZ_TEST_INT(ezMath::FirstBitHigh(ezUInt64(0x00FF000000FF000F)), 55);
    EZ_TEST_INT(ezMath::FirstBitHigh(ezUInt64(0x007F000000FF000F)), 54);
    EZ_TEST_INT(ezMath::FirstBitHigh(ezUInt64(0x003F000000FF000F)), 53);
    EZ_TEST_INT(ezMath::FirstBitHigh(ezUInt64(0x001F000000FF000F)), 52);
    EZ_TEST_INT(ezMath::FirstBitHigh(ezUInt64(0xFFFFFFFFFFFFFFFF)), 63);

    // Edge cases specifically for 32-bit systems where upper and lower 32-bit are handled individually.
    EZ_TEST_INT(ezMath::FirstBitHigh(ezUInt64(0x00000000FFFFFFFF)), 31);
    EZ_TEST_INT(ezMath::FirstBitHigh(ezUInt64(0xFFFFFFFF00000000)), 63);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CountTrailingZeros (32)")
  {
    EZ_TEST_INT(ezMath::CountTrailingZeros(0b1111u), 0);
    EZ_TEST_INT(ezMath::CountTrailingZeros(0b1110u), 1);
    EZ_TEST_INT(ezMath::CountTrailingZeros(0b1100u), 2);
    EZ_TEST_INT(ezMath::CountTrailingZeros(0b1000u), 3);
    EZ_TEST_INT(ezMath::CountTrailingZeros(0xFFFFFFFF), 0);
    EZ_TEST_INT(ezMath::CountTrailingZeros(0u), 32);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CountTrailingZeros (64)")
  {
    EZ_TEST_INT(ezMath::CountTrailingZeros(0b1111llu), 0);
    EZ_TEST_INT(ezMath::CountTrailingZeros(0b1110llu), 1);
    EZ_TEST_INT(ezMath::CountTrailingZeros(0b1100llu), 2);
    EZ_TEST_INT(ezMath::CountTrailingZeros(0b1000llu), 3);
    EZ_TEST_INT(ezMath::CountTrailingZeros(0xFFFFFFFF0llu), 4);
    EZ_TEST_INT(ezMath::CountTrailingZeros(0llu), 64);
    EZ_TEST_INT(ezMath::CountTrailingZeros(0xFFFFFFFF00llu), 8);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CountLeadingZeros")
  {
    EZ_TEST_INT(ezMath::CountLeadingZeros(0b1111), 28);
    EZ_TEST_INT(ezMath::CountLeadingZeros(0b0111), 29);
    EZ_TEST_INT(ezMath::CountLeadingZeros(0b0011), 30);
    EZ_TEST_INT(ezMath::CountLeadingZeros(0b0001), 31);
    EZ_TEST_INT(ezMath::CountLeadingZeros(0xFFFFFFFF), 0);
    EZ_TEST_INT(ezMath::CountLeadingZeros(0), 32);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Bitmask_LowN")
  {
    EZ_TEST_INT(ezMath::Bitmask_LowN<ezUInt32>(0), 0);
    EZ_TEST_INT(ezMath::Bitmask_LowN<ezUInt32>(1), 1);
    EZ_TEST_INT(ezMath::Bitmask_LowN<ezUInt32>(2), 3);
    EZ_TEST_INT(ezMath::Bitmask_LowN<ezUInt32>(3), 7);
    EZ_TEST_INT(ezMath::Bitmask_LowN<ezUInt32>(31), 0x7fffffff);
    EZ_TEST_INT(ezMath::Bitmask_LowN<ezUInt32>(32), 0xffffffffu);
    EZ_TEST_INT(ezMath::Bitmask_LowN<ezUInt32>(33), 0xffffffffu);
    EZ_TEST_INT(ezMath::Bitmask_LowN<ezUInt32>(50), 0xffffffffu);

    EZ_TEST_INT(ezMath::Bitmask_LowN<ezUInt64>(0), 0);
    EZ_TEST_INT(ezMath::Bitmask_LowN<ezUInt64>(1), 1);
    EZ_TEST_INT(ezMath::Bitmask_LowN<ezUInt64>(2), 3);
    EZ_TEST_INT(ezMath::Bitmask_LowN<ezUInt64>(3), 7);
    EZ_TEST_INT(ezMath::Bitmask_LowN<ezUInt64>(31), 0x7fffffff);
    EZ_TEST_INT(ezMath::Bitmask_LowN<ezUInt64>(32), 0xffffffffu);
    EZ_TEST_INT(ezMath::Bitmask_LowN<ezUInt64>(63), 0x7fffffffffffffffull);
    EZ_TEST_BOOL(ezMath::Bitmask_LowN<ezUInt64>(64) == 0xffffffffffffffffull);
    EZ_TEST_BOOL(ezMath::Bitmask_LowN<ezUInt64>(65) == 0xffffffffffffffffull);
    EZ_TEST_BOOL(ezMath::Bitmask_LowN<ezUInt64>(100) == 0xffffffffffffffffull);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Bitmask_HighN")
  {
    EZ_TEST_INT(ezMath::Bitmask_HighN<ezUInt32>(0), 0u);
    EZ_TEST_INT(ezMath::Bitmask_HighN<ezUInt32>(1), 0x80000000u);
    EZ_TEST_INT(ezMath::Bitmask_HighN<ezUInt32>(2), 0xC0000000u);
    EZ_TEST_INT(ezMath::Bitmask_HighN<ezUInt32>(3), 0xE0000000u);
    EZ_TEST_INT(ezMath::Bitmask_HighN<ezUInt32>(31), 0xfffffffeu);
    EZ_TEST_INT(ezMath::Bitmask_HighN<ezUInt32>(32), 0xffffffffu);
    EZ_TEST_INT(ezMath::Bitmask_HighN<ezUInt32>(33), 0xffffffffu);
    EZ_TEST_INT(ezMath::Bitmask_HighN<ezUInt32>(60), 0xffffffffu);

    EZ_TEST_BOOL(ezMath::Bitmask_HighN<ezUInt64>(0) == 0);
    EZ_TEST_BOOL(ezMath::Bitmask_HighN<ezUInt64>(1) == 0x8000000000000000llu);
    EZ_TEST_BOOL(ezMath::Bitmask_HighN<ezUInt64>(2) == 0xC000000000000000llu);
    EZ_TEST_BOOL(ezMath::Bitmask_HighN<ezUInt64>(3) == 0xE000000000000000llu);
    EZ_TEST_BOOL(ezMath::Bitmask_HighN<ezUInt64>(31) == 0xfffffffe00000000llu);
    EZ_TEST_BOOL(ezMath::Bitmask_HighN<ezUInt64>(32) == 0xffffffff00000000llu);
    EZ_TEST_BOOL(ezMath::Bitmask_HighN<ezUInt64>(63) == 0xfffffffffffffffellu);
    EZ_TEST_BOOL(ezMath::Bitmask_HighN<ezUInt64>(64) == 0xffffffffffffffffull);
    EZ_TEST_BOOL(ezMath::Bitmask_HighN<ezUInt64>(65) == 0xffffffffffffffffull);
    EZ_TEST_BOOL(ezMath::Bitmask_HighN<ezUInt64>(1000) == 0xffffffffffffffffull);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TryMultiply32")
  {
    ezUInt32 res;

    res = 0;
    EZ_TEST_BOOL(ezMath::TryMultiply32(res, 1, 1, 2, 3).Succeeded());
    EZ_TEST_INT(res, 6);

    res = 0;
    EZ_TEST_BOOL(ezMath::TryMultiply32(res, 1, 1, 1, 0xFFFFFFFF).Succeeded());
    EZ_TEST_BOOL(res == 0xFFFFFFFF);

    res = 0;
    EZ_TEST_BOOL(ezMath::TryMultiply32(res, 0xFFFF, 0x10001).Succeeded());
    EZ_TEST_BOOL(res == 0xFFFFFFFF);

    res = 0;
    EZ_TEST_BOOL(ezMath::TryMultiply32(res, 0x3FFFFFF, 2, 4, 8).Succeeded());
    EZ_TEST_BOOL(res == 0xFFFFFFC0);

    res = 1;
    EZ_TEST_BOOL(ezMath::TryMultiply32(res, 0xFFFFFFFF, 2).Failed());
    EZ_TEST_BOOL(res == 1);

    res = 0;
    EZ_TEST_BOOL(ezMath::TryMultiply32(res, 0x80000000, 2).Failed()); // slightly above 0xFFFFFFFF
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TryMultiply64")
  {
    ezUInt64 res;

    res = 0;
    EZ_TEST_BOOL(ezMath::TryMultiply64(res, 1, 1, 2, 3).Succeeded());
    EZ_TEST_INT(res, 6);

    res = 0;
    EZ_TEST_BOOL(ezMath::TryMultiply64(res, 1, 1, 1, 0xFFFFFFFF).Succeeded());
    EZ_TEST_BOOL(res == 0xFFFFFFFF);

    res = 0;
    EZ_TEST_BOOL(ezMath::TryMultiply64(res, 0xFFFF, 0x10001).Succeeded());
    EZ_TEST_BOOL(res == 0xFFFFFFFF);

    res = 0;
    EZ_TEST_BOOL(ezMath::TryMultiply64(res, 0x3FFFFFF, 2, 4, 8).Succeeded());
    EZ_TEST_BOOL(res == 0xFFFFFFC0);

    res = 0;
    EZ_TEST_BOOL(ezMath::TryMultiply64(res, 0xFFFFFFFF, 2).Succeeded());
    EZ_TEST_BOOL(res == 0x1FFFFFFFE);

    res = 0;
    EZ_TEST_BOOL(ezMath::TryMultiply64(res, 0x80000000, 2).Succeeded());
    EZ_TEST_BOOL(res == 0x100000000);

    res = 0;
    EZ_TEST_BOOL(ezMath::TryMultiply64(res, 0xFFFFFFFF, 0xFFFFFFFF).Succeeded());
    EZ_TEST_BOOL(res == 0xFFFFFFFE00000001);

    res = 0;
    EZ_TEST_BOOL(ezMath::TryMultiply64(res, 0xFFFFFFFFFFFFFFFF, 2).Failed());

    res = 0;
    EZ_TEST_BOOL(ezMath::TryMultiply64(res, 0xFFFFFFFF, 0xFFFFFFFF, 2).Failed());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TryConvertToSizeT")
  {
    ezUInt64 x = ezMath::MaxValue<ezUInt32>();
    ezUInt64 y = x + 1;

    size_t res = 0;

    EZ_TEST_BOOL(ezMath::TryConvertToSizeT(res, x).Succeeded());
    EZ_TEST_BOOL(res == x);

    res = 0;
#if EZ_ENABLED(EZ_PLATFORM_32BIT)
    EZ_TEST_BOOL(ezMath::TryConvertToSizeT(res, y).Failed());
#else
    EZ_TEST_BOOL(ezMath::TryConvertToSizeT(res, y).Succeeded());
    EZ_TEST_BOOL(res == y);
#endif
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReplaceNaN")
  {
    EZ_TEST_FLOAT(ezMath::ReplaceNaN(0.0f, 42.0f), 0.0f, 0);
    EZ_TEST_FLOAT(ezMath::ReplaceNaN(ezMath::HighValue<float>(), 2.0f), ezMath::HighValue<float>(), 0);
    EZ_TEST_FLOAT(ezMath::ReplaceNaN(-ezMath::HighValue<float>(), 2.0f), -ezMath::HighValue<float>(), 0);

    EZ_TEST_FLOAT(ezMath::ReplaceNaN(ezMath::NaN<float>(), 2.0f), 2.0f, 0);
    EZ_TEST_FLOAT(ezMath::ReplaceNaN(ezMath::NaN<double>(), 3.0), 3.0, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ComparisonOperator")
  {
    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::Equal, 1.0, 1.0));
    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::Equal, 1.0, 2.0) == false);

    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::NotEqual, 1.0, 2.0));
    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::NotEqual, 1.0, 1.0) == false);

    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::Less, 1.0, 1.0) == false);
    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::Less, 1.0, 2.0));
    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::Less, -2.0, -1.0));
    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::Less, 3.0, 2.0) == false);

    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::LessEqual, 1.0, 1.0));
    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::LessEqual, 1.0, 2.0));
    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::LessEqual, -2.0, -1.0));
    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::LessEqual, 3.0, 2.0) == false);

    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::Greater, 1.0, 1.0) == false);
    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::Greater, 3.0, 2.0));
    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::Greater, -1.0, -2.0));
    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::Greater, 2.0, 3.0) == false);

    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::GreaterEqual, 1.0, 1.0));
    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::GreaterEqual, 3.0, 2.0));
    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::GreaterEqual, -1.0, -2.0));
    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::GreaterEqual, 2.0, 3.0) == false);

    ezStringView a = "a";
    ezStringView b = "b";
    ezStringView c = "c";
    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::Equal, a, a));
    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::Equal, a, b) == false);

    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::NotEqual, a, c));
    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::NotEqual, a, a) == false);

    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::Less, a, a) == false);
    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::Less, a, b));
    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::Less, c, b) == false);

    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::LessEqual, a, a));
    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::LessEqual, a, b));
    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::LessEqual, c, b) == false);

    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::Greater, a, a) == false);
    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::Greater, c, b));
    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::Greater, a, b) == false);

    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::GreaterEqual, a, a));
    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::GreaterEqual, c, b));
    EZ_TEST_BOOL(ezComparisonOperator::Compare(ezComparisonOperator::GreaterEqual, a, b) == false);
  }
}
