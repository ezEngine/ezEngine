#include <PCH.h>
#include <Foundation/Math/Math.h>

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

#define EZ_8BIT__(iBits) \
   (((iBits & 000000001) ?   1 : 0) \
  + ((iBits & 000000010) ?   2 : 0) \
  + ((iBits & 000000100) ?   4 : 0) \
  + ((iBits & 000001000) ?   8 : 0) \
  + ((iBits & 000010000) ?  16 : 0) \
  + ((iBits & 000100000) ?  32 : 0) \
  + ((iBits & 001000000) ?  64 : 0) \
  + ((iBits & 010000000) ? 128 : 0)) \

#define EZ_8BIT(B) \
  ((ezUInt8)EZ_8BIT__(OCT__(B)))

#define EZ_16BIT(B2, B1) \
  (((ezUInt8)EZ_8BIT(B2) << 8) + EZ_8BIT(B1))

#define EZ_32BIT(B4, B3, B2, B1) \
    ((unsigned long)EZ_8BIT(B4) << 24) \
  + ((unsigned long)EZ_8BIT(B3) << 16) \
  + ((unsigned long)EZ_8BIT(B2) <<  8) \
  + ((unsigned long)EZ_8BIT(B1))

namespace 
{
    struct UniqueInt
    {
        int i, id;
        UniqueInt(int i, int id) : i(i), id(id) {}

        bool operator < (const UniqueInt& rh) 
        {
            return this->i < rh.i;
        }

        bool operator > (const UniqueInt& rh)
        {
            return this->i > rh.i;
        }
    };
};


EZ_CREATE_SIMPLE_TEST_GROUP(Math);

EZ_CREATE_SIMPLE_TEST(Math, General)
{
  //EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constants")
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
    EZ_TEST_FLOAT(ezMath::Sin(ezAngle::Degree(0.0f)),   0.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Sin(ezAngle::Degree(90.0f)),  1.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Sin(ezAngle::Degree(180.0f)), 0.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Sin(ezAngle::Degree(270.0f)),-1.0f, 0.000001f);
    
    EZ_TEST_FLOAT(ezMath::Sin(ezAngle::Degree(45.0f)),  0.7071067f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Sin(ezAngle::Degree(135.0f)), 0.7071067f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Sin(ezAngle::Degree(225.0f)),-0.7071067f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Sin(ezAngle::Degree(315.0f)),-0.7071067f, 0.000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Cos")
  {
    EZ_TEST_FLOAT(ezMath::Cos(ezAngle::Degree(0.0f)),   1.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Cos(ezAngle::Degree(90.0f)),  0.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Cos(ezAngle::Degree(180.0f)),-1.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Cos(ezAngle::Degree(270.0f)), 0.0f, 0.000001f);
    
    EZ_TEST_FLOAT(ezMath::Cos(ezAngle::Degree(45.0f)),  0.7071067f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Cos(ezAngle::Degree(135.0f)),-0.7071067f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Cos(ezAngle::Degree(225.0f)),-0.7071067f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Cos(ezAngle::Degree(315.0f)), 0.7071067f, 0.000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Tan")
  {
    EZ_TEST_FLOAT(ezMath::Tan(ezAngle::Degree(0.0f)), 0.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Tan(ezAngle::Degree(45.0f)), 1.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Tan(ezAngle::Degree(-45.0f)), -1.0f, 0.000001f);
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

      EZ_TEST_FLOAT(fTan - fTanPrev, 0.0f, 0.001f);
      EZ_TEST_FLOAT(fTan - fTanNext, 0.0f, 0.001f);
      EZ_TEST_FLOAT(fTan - (fSin / fCos), 0.0f, 0.0001f);
      angle += ezAngle::Degree(1.234f);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ASin")
  {
    EZ_TEST_FLOAT(ezMath::ASin(0.0f).GetDegree(),   0.0f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::ASin(1.0f).GetDegree(),  90.0f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::ASin(-1.0f).GetDegree(),-90.0f, 0.00001f);
    
    EZ_TEST_FLOAT(ezMath::ASin(0.7071067f).GetDegree(),  45.0f, 0.0001f);
    EZ_TEST_FLOAT(ezMath::ASin(-0.7071067f).GetDegree(),-45.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ACos")
  {
    EZ_TEST_FLOAT(ezMath::ACos(0.0f).GetDegree(),  90.0f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::ACos(1.0f).GetDegree(),   0.0f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::ACos(-1.0f).GetDegree(),180.0f, 0.0001f);
    
    EZ_TEST_FLOAT(ezMath::ACos( 0.7071067f).GetDegree(), 45.0f, 0.0001f);
    EZ_TEST_FLOAT(ezMath::ACos(-0.7071067f).GetDegree(),135.0f, 0.0001f);
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
    EZ_TEST_FLOAT(1.0f, ezMath::Exp (0.0f), 0.000001f);
    EZ_TEST_FLOAT(2.7182818284f, ezMath::Exp (1.0f), 0.000001f);
    EZ_TEST_FLOAT(7.3890560989f, ezMath::Exp (2.0f), 0.000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Ln")
  {
    EZ_TEST_FLOAT(0.0f, ezMath::Ln (1.0f), 0.000001f);
    EZ_TEST_FLOAT(1.0f, ezMath::Ln (2.7182818284f), 0.000001f);
    EZ_TEST_FLOAT(2.0f, ezMath::Ln (7.3890560989f), 0.000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Log2")
  {
    EZ_TEST_FLOAT(0.0f, ezMath::Log2 (1.0f), 0.000001f);
    EZ_TEST_FLOAT(1.0f, ezMath::Log2 (2.0f), 0.000001f);
    EZ_TEST_FLOAT(2.0f, ezMath::Log2 (4.0f), 0.000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Log2i")
  {
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
    EZ_TEST_FLOAT(0.0f, ezMath::Log10 (1.0f), 0.000001f);
    EZ_TEST_FLOAT(1.0f, ezMath::Log10 (10.0f), 0.000001f);
    EZ_TEST_FLOAT(2.0f, ezMath::Log10 (100.0f), 0.000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Log")
  {
    EZ_TEST_FLOAT(0.0f, ezMath::Log (2.7182818284f, 1.0f), 0.000001f);
    EZ_TEST_FLOAT(1.0f, ezMath::Log (2.7182818284f, 2.7182818284f), 0.000001f);
    EZ_TEST_FLOAT(2.0f, ezMath::Log (2.7182818284f, 7.3890560989f), 0.000001f);

    EZ_TEST_FLOAT(0.0f, ezMath::Log (2.0f, 1.0f), 0.000001f);
    EZ_TEST_FLOAT(1.0f, ezMath::Log (2.0f, 2.0f), 0.000001f);
    EZ_TEST_FLOAT(2.0f, ezMath::Log (2.0f, 4.0f), 0.000001f);

    EZ_TEST_FLOAT(0.0f, ezMath::Log (10.0f, 1.0f), 0.000001f);
    EZ_TEST_FLOAT(1.0f, ezMath::Log (10.0f, 10.0f), 0.000001f);
    EZ_TEST_FLOAT(2.0f, ezMath::Log (10.0f, 100.0f), 0.000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Pow2")
  {
    EZ_TEST_FLOAT(1.0f, ezMath::Pow2 (0.0f), 0.000001f);
    EZ_TEST_FLOAT(2.0f, ezMath::Pow2 (1.0f), 0.000001f);
    EZ_TEST_FLOAT(4.0f, ezMath::Pow2 (2.0f), 0.000001f);

    EZ_TEST_BOOL(ezMath::Pow2 (0) == 1);
    EZ_TEST_BOOL(ezMath::Pow2 (1) == 2);
    EZ_TEST_BOOL(ezMath::Pow2 (2) == 4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Pow")
  {
    EZ_TEST_FLOAT(1.0f, ezMath::Pow (3.0f, 0.0f), 0.000001f);
    EZ_TEST_FLOAT(3.0f, ezMath::Pow (3.0f, 1.0f), 0.000001f);
    EZ_TEST_FLOAT(9.0f, ezMath::Pow (3.0f, 2.0f), 0.000001f);

    EZ_TEST_BOOL(ezMath::Pow (3, 0) == 1);
    EZ_TEST_BOOL(ezMath::Pow (3, 1) == 3);
    EZ_TEST_BOOL(ezMath::Pow (3, 2) == 9);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Square")
  {
    EZ_TEST_FLOAT(0.0f, ezMath::Square (0.0f), 0.000001f);
    EZ_TEST_FLOAT(1.0f, ezMath::Square (1.0f), 0.000001f);
    EZ_TEST_FLOAT(4.0f, ezMath::Square (2.0f), 0.000001f);
    EZ_TEST_FLOAT(4.0f, ezMath::Square (-2.0f), 0.000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Sqrt")
  {
    EZ_TEST_FLOAT(0.0f, ezMath::Sqrt (0.0f), 0.000001f);
    EZ_TEST_FLOAT(1.0f, ezMath::Sqrt (1.0f), 0.000001f);
    EZ_TEST_FLOAT(2.0f, ezMath::Sqrt (4.0f), 0.000001f);
    EZ_TEST_FLOAT(4.0f, ezMath::Sqrt (16.0f), 0.000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Root")
  {
    EZ_TEST_FLOAT(3.0f, ezMath::Root (27.0f, 3.0f), 0.000001f);
    EZ_TEST_FLOAT(3.0f, ezMath::Root (81.0f, 4.0f), 0.000001f);
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
    EZ_TEST_FLOAT(-23.0f, ezMath::Min(0.0f,-23.0f), 0.00000001f);

    EZ_TEST_BOOL(ezMath::Min(1, 2, 3) == 1);
    EZ_TEST_BOOL(ezMath::Min(4, 2, 3) == 2);
    EZ_TEST_BOOL(ezMath::Min(4, 5, 3) == 3);

    EZ_TEST_BOOL(ezMath::Min(1, 2, 3, 4) == 1);
    EZ_TEST_BOOL(ezMath::Min(5, 2, 3, 4) == 2);
    EZ_TEST_BOOL(ezMath::Min(5, 6, 3, 4) == 3);
    EZ_TEST_BOOL(ezMath::Min(5, 6, 7, 4) == 4);

    EZ_TEST_BOOL(ezMath::Min(UniqueInt(1, 0), UniqueInt(1, 1)).id == 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Max")
  {
    EZ_TEST_FLOAT(23.0f, ezMath::Max(0.0f, 23.0f), 0.00000001f);
    EZ_TEST_FLOAT(0.0f, ezMath::Max(0.0f,-23.0f), 0.00000001f);

    EZ_TEST_BOOL(ezMath::Max(1, 2, 3) == 3);
    EZ_TEST_BOOL(ezMath::Max(1, 2, 0) == 2);
    EZ_TEST_BOOL(ezMath::Max(1, 0, 0) == 1);
                   
    EZ_TEST_BOOL(ezMath::Max(1, 2, 3, 4) == 4);
    EZ_TEST_BOOL(ezMath::Max(1, 2, 3, 0) == 3);
    EZ_TEST_BOOL(ezMath::Max(1, 2, 0, 0) == 2);
    EZ_TEST_BOOL(ezMath::Max(1, 0, 0, 0) == 1);

    EZ_TEST_BOOL(ezMath::Max(UniqueInt(1, 0), UniqueInt(1, 1)).id == 1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Clamp")
  {
    EZ_TEST_FLOAT(15.0f, ezMath::Clamp(23.0f, 12.0f, 15.0f), 0.00000001f);
    EZ_TEST_FLOAT(12.0f, ezMath::Clamp(3.0f, 12.0f, 15.0f), 0.00000001f);
    EZ_TEST_FLOAT(14.0f, ezMath::Clamp(14.0f, 12.0f, 15.0f), 0.00000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Floor")
  {
    EZ_TEST_BOOL( 12 == ezMath::Floor(12.34f));
    EZ_TEST_BOOL(-13 == ezMath::Floor(-12.34f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Ceil")
  {
    EZ_TEST_BOOL( 13 == ezMath::Ceil(12.34f));
    EZ_TEST_BOOL(-12 == ezMath::Ceil(-12.34f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FloorM")
  {
    EZ_TEST_FLOAT( 10.0f, ezMath::Floor(12.34f, 5.0f), 0.0000001f);
    EZ_TEST_FLOAT(-15.0f, ezMath::Floor(-12.34f, 5.0f), 0.0000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CeilM")
  {
    EZ_TEST_FLOAT( 15.0f, ezMath::Ceil(12.34f, 5.0f), 0.0000001f);
    EZ_TEST_FLOAT(-10.0f, ezMath::Ceil(-12.34f, 5.0f), 0.0000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FloorIM")
  {
    EZ_TEST_BOOL(10 == ezMath::Floor(11, 5));
    EZ_TEST_BOOL(10 == ezMath::Floor(10, 5));
    EZ_TEST_BOOL(5  == ezMath::Floor(9, 5));

    EZ_TEST_BOOL(-10== ezMath::Floor(-6, 5));
    EZ_TEST_BOOL(-10== ezMath::Floor(-10, 5));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CeilIM")
  {
    EZ_TEST_BOOL(15 == ezMath::Ceil(11, 5));
    EZ_TEST_BOOL(10 == ezMath::Ceil(10, 5));
    EZ_TEST_BOOL(10  == ezMath::Ceil(9, 5));

    EZ_TEST_BOOL(-5== ezMath::Ceil(-6, 5));
    EZ_TEST_BOOL(-10== ezMath::Ceil(-10, 5));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Trunc")
  {
    EZ_TEST_BOOL(ezMath::Trunc (12.34f) == 12);
    EZ_TEST_BOOL(ezMath::Trunc (-12.34f) == -12);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Round")
  {
    EZ_TEST_BOOL(ezMath::Round(12.34f) == 12);
    EZ_TEST_BOOL(ezMath::Round(-12.34f) == -12);

    EZ_TEST_BOOL(ezMath::Round(12.54f) == 13);
    EZ_TEST_BOOL(ezMath::Round(-12.54f) == -13);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Round_Multiple")
  {
    EZ_TEST_FLOAT(ezMath::Round(12.34f, 7.0f), 14.0f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::Round(-12.34f, 7.0f), -14.0f, 0.00001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Fraction")
  {
    EZ_TEST_FLOAT(ezMath::Fraction(12.34f), 0.34f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::Fraction(-12.34f), -0.34f, 0.00001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Mod")
  {
    EZ_TEST_FLOAT( 2.34f, ezMath::Mod( 12.34f,  2.5f), 0.000001f);
    EZ_TEST_FLOAT(-2.34f, ezMath::Mod(-12.34f,  2.5f), 0.000001f);

    EZ_TEST_FLOAT( 2.34f, ezMath::Mod( 12.34f, -2.5f), 0.000001f);
    EZ_TEST_FLOAT(-2.34f, ezMath::Mod(-12.34f, -2.5f), 0.000001f);
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
    EZ_TEST_FLOAT(ezMath::Lerp(-5.0f, 5.0f, 1.0f),  5.0f, 0.000001);
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
      EZ_TEST_FLOAT(ezMath::SmoothStep(0.0f  * iScale, 0.1f * iScale, 0.4f * iScale), 0.0f, 0.000001);
      EZ_TEST_FLOAT(ezMath::SmoothStep(0.1f  * iScale, 0.1f * iScale, 0.4f * iScale), 0.0f, 0.000001);
      EZ_TEST_FLOAT(ezMath::SmoothStep(0.4f  * iScale, 0.1f * iScale, 0.4f * iScale), 1.0f, 0.000001);
      EZ_TEST_FLOAT(ezMath::SmoothStep(0.25f * iScale, 0.1f * iScale, 0.4f * iScale), 0.5f, 0.000001);
      EZ_TEST_FLOAT(ezMath::SmoothStep(0.5f  * iScale, 0.1f * iScale, 0.4f * iScale), 1.0f, 0.000001);

      EZ_TEST_FLOAT(ezMath::SmoothStep(0.5f  * iScale, 0.4f * iScale, 0.1f * iScale), 0.0f, 0.000001);
      EZ_TEST_FLOAT(ezMath::SmoothStep(0.4f  * iScale, 0.4f * iScale, 0.1f * iScale), 0.0f, 0.000001);
      EZ_TEST_FLOAT(ezMath::SmoothStep(0.1f  * iScale, 0.4f * iScale, 0.1f * iScale), 1.0f, 0.000001);
      EZ_TEST_FLOAT(ezMath::SmoothStep(0.25f * iScale, 0.1f * iScale, 0.4f * iScale), 0.5f, 0.000001);
      EZ_TEST_FLOAT(ezMath::SmoothStep(0.0f  * iScale, 0.4f * iScale, 0.1f * iScale), 1.0f, 0.000001);

      // For edge1 == edge2 SmoothStep should behave like Step
      EZ_TEST_FLOAT(ezMath::SmoothStep(0.0f* iScale, 0.1f* iScale, 0.1f* iScale), iScale > 0 ? 0.0f : 1.0f, 0.000001);
      EZ_TEST_FLOAT(ezMath::SmoothStep(0.2f* iScale, 0.1f* iScale, 0.1f* iScale), iScale < 0 ? 0.0f : 1.0f, 0.000001);
    }
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
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "PowerOf2_Floor")
  {
    EZ_TEST_BOOL(ezMath::PowerOfTwo_Floor(64) == 64);
    EZ_TEST_BOOL(ezMath::PowerOfTwo_Floor(33) == 32);
    EZ_TEST_BOOL(ezMath::PowerOfTwo_Floor(4) == 4);
    EZ_TEST_BOOL(ezMath::PowerOfTwo_Floor(5) == 4);
    EZ_TEST_BOOL(ezMath::PowerOfTwo_Floor(1) == 1);
    // strange case...
    EZ_TEST_BOOL(ezMath::PowerOfTwo_Floor(0) == 1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "PowerOf2_Ceil")
  {
    EZ_TEST_BOOL(ezMath::PowerOfTwo_Ceil(64) == 64);
    EZ_TEST_BOOL(ezMath::PowerOfTwo_Ceil(33) == 64);
    EZ_TEST_BOOL(ezMath::PowerOfTwo_Ceil(4) == 4);
    EZ_TEST_BOOL(ezMath::PowerOfTwo_Ceil(5) == 8);
    EZ_TEST_BOOL(ezMath::PowerOfTwo_Ceil(1) == 1);
    EZ_TEST_BOOL(ezMath::PowerOfTwo_Ceil(0) == 1);
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
    if (ezMath::BasicType<ezMathTestType>::SupportsNaN())
    {
      EZ_TEST_BOOL(ezMath::IsNaN(ezMath::BasicType<ezMathTestType>::GetNaN()) == true);

      EZ_TEST_BOOL(ezMath::BasicType<ezMathTestType>::GetInfinity() == ezMath::BasicType<ezMathTestType>::GetInfinity() - (ezMathTestType) 1);
      EZ_TEST_BOOL(ezMath::BasicType<ezMathTestType>::GetInfinity() == ezMath::BasicType<ezMathTestType>::GetInfinity() + (ezMathTestType) 1);

      EZ_TEST_BOOL(ezMath::IsNaN(ezMath::BasicType<ezMathTestType>::GetInfinity() - ezMath::BasicType<ezMathTestType>::GetInfinity()));

      EZ_TEST_BOOL(!ezMath::IsFinite( ezMath::BasicType<ezMathTestType>::GetInfinity()));
      EZ_TEST_BOOL(!ezMath::IsFinite(-ezMath::BasicType<ezMathTestType>::GetInfinity()));
      EZ_TEST_BOOL(!ezMath::IsFinite(ezMath::BasicType<ezMathTestType>::GetNaN()));
      EZ_TEST_BOOL(!ezMath::IsNaN(ezMath::BasicType<ezMathTestType>::GetInfinity()));
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
}
