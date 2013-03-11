#include <TestFramework/Framework/TestFramework.h>
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


EZ_CREATE_SIMPLE_TEST_GROUP(Math);

EZ_CREATE_SIMPLE_TEST(Math, General)
{
  EZ_TEST_BLOCK(true, "Constants")
  {
    // Macro test
    EZ_TEST(EZ_8BIT(01010101) == 85);
    EZ_TEST(EZ_16BIT(10101010, 01010101) == 43605);
    EZ_TEST(EZ_32BIT(10000000, 11111111, 10101010, 01010101) == 2164238933);

    // Infinity test
    //                           Sign:_
    //                       Exponent: _______  _
    //                       Fraction:           _______  ________  ________
    ezIntFloatUnion uInf = { EZ_32BIT(01111111, 10000000, 00000000, 00000000) };
    EZ_TEST(uInf.f == ezMath::Infinity());

    // FloatMax_Pos test
    ezIntFloatUnion uMax = { EZ_32BIT(01111111, 01111111, 11111111, 11111111) };
    EZ_TEST(uMax.f == ezMath::FloatMax_Pos());

    // FloatMax_Neg test
    ezIntFloatUnion uMin = { EZ_32BIT(11111111, 01111111, 11111111, 11111111) };
    EZ_TEST(uMin.f == ezMath::FloatMax_Neg());
  }

  EZ_TEST_BLOCK(true, "Sin")
  {
    EZ_TEST_FLOAT(ezMath::SinDeg(0.0f),   0.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::SinDeg(90.0f),  1.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::SinDeg(180.0f), 0.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::SinDeg(270.0f),-1.0f, 0.000001f);
    
    EZ_TEST_FLOAT(ezMath::SinDeg(45.0f),  0.7071067f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::SinDeg(135.0f), 0.7071067f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::SinDeg(225.0f),-0.7071067f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::SinDeg(315.0f),-0.7071067f, 0.000001f);

    EZ_TEST_FLOAT(ezMath::SinRad(ezMath::DegToRad(0.0f)),   0.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::SinRad(ezMath::DegToRad(90.0f)),  1.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::SinRad(ezMath::DegToRad(180.0f)), 0.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::SinRad(ezMath::DegToRad(270.0f)),-1.0f, 0.000001f);
    
    EZ_TEST_FLOAT(ezMath::SinRad(ezMath::DegToRad(45.0f )), 0.7071067f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::SinRad(ezMath::DegToRad(135.0f)), 0.7071067f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::SinRad(ezMath::DegToRad(225.0f)),-0.7071067f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::SinRad(ezMath::DegToRad(315.0f)),-0.7071067f, 0.000001f);
  }

  EZ_TEST_BLOCK(true, "Cos")
  {
    EZ_TEST_FLOAT(ezMath::CosDeg(0.0f),   1.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::CosDeg(90.0f),  0.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::CosDeg(180.0f),-1.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::CosDeg(270.0f), 0.0f, 0.000001f);
    
    EZ_TEST_FLOAT(ezMath::CosDeg(45.0f),  0.7071067f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::CosDeg(135.0f),-0.7071067f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::CosDeg(225.0f),-0.7071067f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::CosDeg(315.0f), 0.7071067f, 0.000001f);

    EZ_TEST_FLOAT(ezMath::CosRad(ezMath::DegToRad(0.0f  )), 1.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::CosRad(ezMath::DegToRad(90.0f )), 0.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::CosRad(ezMath::DegToRad(180.0f)),-1.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::CosRad(ezMath::DegToRad(270.0f)), 0.0f, 0.000001f);

    EZ_TEST_FLOAT(ezMath::CosRad(ezMath::DegToRad(45.0f )), 0.7071067f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::CosRad(ezMath::DegToRad(135.0f)),-0.7071067f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::CosRad(ezMath::DegToRad(225.0f)),-0.7071067f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::CosRad(ezMath::DegToRad(315.0f)), 0.7071067f, 0.000001f);
  }

  EZ_TEST_BLOCK(true, "Tan")
  {
    EZ_TEST_FLOAT(ezMath::TanDeg(0.0f), 0.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::TanDeg(45.0f), 1.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::TanDeg(-45.0f), -1.0f, 0.000001f);
    EZ_TEST(ezMath::TanDeg(90.00001f) < 1000000.0f);
    EZ_TEST(ezMath::TanDeg(89.99999f) > 1000000.0f);

    EZ_TEST_FLOAT(ezMath::TanRad(ezMath::DegToRad(0.0f)), 0.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::TanRad(ezMath::DegToRad(45.0f)), 1.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::TanRad(ezMath::DegToRad(-45.0f)), -1.0f, 0.000001f);
    EZ_TEST(ezMath::TanRad(ezMath::DegToRad(90.00001f)) < 1000000.0f);
    EZ_TEST(ezMath::TanRad(ezMath::DegToRad(89.99999f)) > 1000000.0f);

    // Testing the period of tan(x) centered at 0 and the adjacent ones
    float fAngle = -89.0f;
    while (fAngle < 89.0f)
    {
      float fTan = ezMath::TanDeg(fAngle);
      float fTanPrev = ezMath::TanDeg(fAngle - 180.0f);
      float fTanNext = ezMath::TanDeg(fAngle + 180.0f);
      float fSin = ezMath::SinDeg(fAngle);
      float fCos = ezMath::CosDeg(fAngle);

      EZ_TEST_FLOAT(fTan - fTanPrev, 0.0f, 0.001f);
      EZ_TEST_FLOAT(fTan - fTanNext, 0.0f, 0.001f);
      EZ_TEST_FLOAT(fTan - (fSin / fCos), 0.0f, 0.0001f);
      fAngle += 1.234f;
    }
  }

  EZ_TEST_BLOCK(true, "ASin")
  {
    EZ_TEST_FLOAT(ezMath::ASinDeg(0.0f),   0.0f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::ASinDeg(1.0f),  90.0f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::ASinDeg(-1.0f),-90.0f, 0.00001f);
    
    EZ_TEST_FLOAT(ezMath::ASinDeg(0.7071067f),  45.0f, 0.0001f);
    EZ_TEST_FLOAT(ezMath::ASinDeg(-0.7071067f),-45.0f, 0.0001f);

    EZ_TEST_FLOAT(ezMath::RadToDeg(ezMath::ASinRad( 0.0f)),  0.0f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::RadToDeg(ezMath::ASinRad( 1.0f)), 90.0f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::RadToDeg(ezMath::ASinRad(-1.0f)),-90.0f, 0.00001f);
    
    EZ_TEST_FLOAT(ezMath::RadToDeg(ezMath::ASinRad( 0.7071067f)),  45.0f, 0.0001f);
    EZ_TEST_FLOAT(ezMath::RadToDeg(ezMath::ASinRad(-0.7071067f)),-45.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(true, "ACos")
  {
    EZ_TEST_FLOAT(ezMath::ACosDeg(0.0f),  90.0f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::ACosDeg(1.0f),   0.0f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::ACosDeg(-1.0f),180.0f, 0.0001f);
    
    EZ_TEST_FLOAT(ezMath::ACosDeg( 0.7071067f), 45.0f, 0.0001f);
    EZ_TEST_FLOAT(ezMath::ACosDeg(-0.7071067f),135.0f, 0.0001f);

    EZ_TEST_FLOAT(ezMath::RadToDeg(ezMath::ACosRad ( 0.0f)),  90.0f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::RadToDeg(ezMath::ACosRad ( 1.0f)),   0.0f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::RadToDeg(ezMath::ACosRad (-1.0f)), 180.0f, 0.0001f);
    
    EZ_TEST_FLOAT(ezMath::RadToDeg(ezMath::ACosRad( 0.7071067f)),  45.0f, 0.0001f);
    EZ_TEST_FLOAT(ezMath::RadToDeg(ezMath::ACosRad(-0.7071067f)), 135.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(true, "ATan")
  {
    EZ_TEST_FLOAT(ezMath::ATanDeg(0.0f), 0.0f, 0.0000001f);
    EZ_TEST_FLOAT(ezMath::ATanDeg(1.0f), 45.0f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::ATanDeg(-1.0f), -45.0f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::ATanDeg(10000000.0f), 90.0f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::ATanDeg(-10000000.0f), -90.0f, 0.00001f);

    EZ_TEST_FLOAT(ezMath::RadToDeg(ezMath::ATanRad(0.0f)), 0.0f, 0.0000001f);
    EZ_TEST_FLOAT(ezMath::RadToDeg(ezMath::ATanRad(1.0f)), 45.0f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::RadToDeg(ezMath::ATanRad(-1.0f)), -45.0f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::RadToDeg(ezMath::ATanRad(10000000.0f)), 90.0f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::RadToDeg(ezMath::ATanRad(-10000000.0f)), -90.0f, 0.00001f);
  }

  EZ_TEST_BLOCK(true, "ATan2")
  {
    for (float fScale = 0.125f; fScale < 1000000.0f; fScale *= 2.0f)
    {
      EZ_TEST_FLOAT(ezMath::ATan2Deg(0.0f, fScale), 0.0f, 0.0000001f);
      EZ_TEST_FLOAT(ezMath::ATan2Deg(fScale, fScale), 45.0f, 0.00001f);
      EZ_TEST_FLOAT(ezMath::ATan2Deg(fScale, 0.0f), 90.0f, 0.00001f);
      EZ_TEST_FLOAT(ezMath::ATan2Deg(-fScale, fScale), -45.0f, 0.00001f);
      EZ_TEST_FLOAT(ezMath::ATan2Deg(-fScale, 0.0f), -90.0f, 0.00001f);
      EZ_TEST_FLOAT(ezMath::ATan2Deg(0.0f, -fScale), 180.0f, 0.0001f);

      EZ_TEST_FLOAT(ezMath::RadToDeg(ezMath::ATan2Rad(0.0f, fScale)), 0.0f, 0.0000001f);
      EZ_TEST_FLOAT(ezMath::RadToDeg(ezMath::ATan2Rad(fScale, fScale)), 45.0f, 0.00001f);
      EZ_TEST_FLOAT(ezMath::RadToDeg(ezMath::ATan2Rad(fScale, 0.0f)), 90.0f, 0.00001f);
      EZ_TEST_FLOAT(ezMath::RadToDeg(ezMath::ATan2Rad(-fScale, fScale)), -45.0f, 0.00001f);
      EZ_TEST_FLOAT(ezMath::RadToDeg(ezMath::ATan2Rad(-fScale, 0.0f)), -90.0f, 0.00001f);
      EZ_TEST_FLOAT(ezMath::RadToDeg(ezMath::ATan2Rad(0.0f, -fScale)), 180.0f, 0.0001f);
    }
  }

  EZ_TEST_BLOCK(true, "Exp")
  {
    EZ_TEST_FLOAT(1.0f, ezMath::Exp (0.0f), 0.000001f);
    EZ_TEST_FLOAT(2.7182818284f, ezMath::Exp (1.0f), 0.000001f);
    EZ_TEST_FLOAT(7.3890560989f, ezMath::Exp (2.0f), 0.000001f);
  }

  EZ_TEST_BLOCK(true, "Ln")
  {
    EZ_TEST_FLOAT(0.0f, ezMath::Ln (1.0f), 0.000001f);
    EZ_TEST_FLOAT(1.0f, ezMath::Ln (2.7182818284f), 0.000001f);
    EZ_TEST_FLOAT(2.0f, ezMath::Ln (7.3890560989f), 0.000001f);
  }

  EZ_TEST_BLOCK(true, "Log2")
  {
    EZ_TEST_FLOAT(0.0f, ezMath::Log2 (1.0f), 0.000001f);
    EZ_TEST_FLOAT(1.0f, ezMath::Log2 (2.0f), 0.000001f);
    EZ_TEST_FLOAT(2.0f, ezMath::Log2 (4.0f), 0.000001f);
  }

  EZ_TEST_BLOCK(true, "Log2i")
  {
    EZ_TEST(ezMath::Log2i(1) == 0);
    EZ_TEST(ezMath::Log2i(2) == 1);
    EZ_TEST(ezMath::Log2i(3) == 1);
    EZ_TEST(ezMath::Log2i(4) == 2);
    EZ_TEST(ezMath::Log2i(6) == 2);
    EZ_TEST(ezMath::Log2i(7) == 2);
    EZ_TEST(ezMath::Log2i(8) == 3);
  }

  EZ_TEST_BLOCK(true, "Log10")
  {
    EZ_TEST_FLOAT(0.0f, ezMath::Log10 (1.0f), 0.000001f);
    EZ_TEST_FLOAT(1.0f, ezMath::Log10 (10.0f), 0.000001f);
    EZ_TEST_FLOAT(2.0f, ezMath::Log10 (100.0f), 0.000001f);
  }

  EZ_TEST_BLOCK(true, "Log")
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

  EZ_TEST_BLOCK(true, "Pow2")
  {
    EZ_TEST_FLOAT(1.0f, ezMath::Pow2 (0.0f), 0.000001f);
    EZ_TEST_FLOAT(2.0f, ezMath::Pow2 (1.0f), 0.000001f);
    EZ_TEST_FLOAT(4.0f, ezMath::Pow2 (2.0f), 0.000001f);

    EZ_TEST(ezMath::Pow2 (0) == 1);
    EZ_TEST(ezMath::Pow2 (1) == 2);
    EZ_TEST(ezMath::Pow2 (2) == 4);
  }

  EZ_TEST_BLOCK(true, "Pow")
  {
    EZ_TEST_FLOAT(1.0f, ezMath::Pow (3.0f, 0.0f), 0.000001f);
    EZ_TEST_FLOAT(3.0f, ezMath::Pow (3.0f, 1.0f), 0.000001f);
    EZ_TEST_FLOAT(9.0f, ezMath::Pow (3.0f, 2.0f), 0.000001f);

    EZ_TEST(ezMath::Pow (3, 0) == 1);
    EZ_TEST(ezMath::Pow (3, 1) == 3);
    EZ_TEST(ezMath::Pow (3, 2) == 9);
  }

  EZ_TEST_BLOCK(true, "Square")
  {
    EZ_TEST_FLOAT(0.0f, ezMath::Square (0.0f), 0.000001f);
    EZ_TEST_FLOAT(1.0f, ezMath::Square (1.0f), 0.000001f);
    EZ_TEST_FLOAT(4.0f, ezMath::Square (2.0f), 0.000001f);
    EZ_TEST_FLOAT(4.0f, ezMath::Square (-2.0f), 0.000001f);
  }

  EZ_TEST_BLOCK(true, "Sqrt")
  {
    EZ_TEST_FLOAT(0.0f, ezMath::Sqrt (0.0f), 0.000001f);
    EZ_TEST_FLOAT(1.0f, ezMath::Sqrt (1.0f), 0.000001f);
    EZ_TEST_FLOAT(2.0f, ezMath::Sqrt (4.0f), 0.000001f);
    EZ_TEST_FLOAT(4.0f, ezMath::Sqrt (16.0f), 0.000001f);
  }

  EZ_TEST_BLOCK(true, "Root")
  {
    EZ_TEST_FLOAT(3.0f, ezMath::Root (27.0f, 3.0f), 0.000001f);
    EZ_TEST_FLOAT(3.0f, ezMath::Root (81.0f, 4.0f), 0.000001f);
  }

  EZ_TEST_BLOCK(true, "Sign")
  {
    EZ_TEST_FLOAT(0.0f, ezMath::Sign(0.0f), 0.00000001f);
    EZ_TEST_FLOAT(1.0f, ezMath::Sign(0.01f), 0.00000001f);
    EZ_TEST_FLOAT(-1.0f, ezMath::Sign(-0.01f), 0.00000001f);
  }

  EZ_TEST_BLOCK(true, "Abs")
  {
    EZ_TEST_FLOAT(0.0f, ezMath::Abs(0.0f), 0.00000001f);
    EZ_TEST_FLOAT(20.0f, ezMath::Abs(20.0f), 0.00000001f);
    EZ_TEST_FLOAT(20.0f, ezMath::Abs(-20.0f), 0.00000001f);
  }

  EZ_TEST_BLOCK(true, "Min")
  {
    EZ_TEST_FLOAT(0.0f, ezMath::Min(0.0f, 23.0f), 0.00000001f);
    EZ_TEST_FLOAT(-23.0f, ezMath::Min(0.0f,-23.0f), 0.00000001f);

    EZ_TEST(ezMath::Min(1, 2, 3) == 1);
    EZ_TEST(ezMath::Min(4, 2, 3) == 2);
    EZ_TEST(ezMath::Min(4, 5, 3) == 3);

    EZ_TEST(ezMath::Min(1, 2, 3, 4) == 1);
    EZ_TEST(ezMath::Min(5, 2, 3, 4) == 2);
    EZ_TEST(ezMath::Min(5, 6, 3, 4) == 3);
    EZ_TEST(ezMath::Min(5, 6, 7, 4) == 4);
  }

  EZ_TEST_BLOCK(true, "Max")
  {
    EZ_TEST_FLOAT(23.0f, ezMath::Max(0.0f, 23.0f), 0.00000001f);
    EZ_TEST_FLOAT(0.0f, ezMath::Max(0.0f,-23.0f), 0.00000001f);

    EZ_TEST(ezMath::Max(1, 2, 3) == 3);
    EZ_TEST(ezMath::Max(1, 2, 0) == 2);
    EZ_TEST(ezMath::Max(1, 0, 0) == 1);
                   
    EZ_TEST(ezMath::Max(1, 2, 3, 4) == 4);
    EZ_TEST(ezMath::Max(1, 2, 3, 0) == 3);
    EZ_TEST(ezMath::Max(1, 2, 0, 0) == 2);
    EZ_TEST(ezMath::Max(1, 0, 0, 0) == 1);
  }

  EZ_TEST_BLOCK(true, "Clamp")
  {
    EZ_TEST_FLOAT(15.0f, ezMath::Clamp(23.0f, 12.0f, 15.0f), 0.00000001f);
    EZ_TEST_FLOAT(12.0f, ezMath::Clamp(3.0f, 12.0f, 15.0f), 0.00000001f);
    EZ_TEST_FLOAT(14.0f, ezMath::Clamp(14.0f, 12.0f, 15.0f), 0.00000001f);
  }

  EZ_TEST_BLOCK(true, "Floor")
  {
    EZ_TEST( 12 == ezMath::Floor(12.34f));
    EZ_TEST(-13 == ezMath::Floor(-12.34f));
  }

  EZ_TEST_BLOCK(true, "Ceil")
  {
    EZ_TEST( 13 == ezMath::Ceil(12.34f));
    EZ_TEST(-12 == ezMath::Ceil(-12.34f));
  }

  EZ_TEST_BLOCK(true, "FloorM")
  {
    EZ_TEST_FLOAT( 10.0f, ezMath::Floor(12.34f, 5.0f), 0.0000001f);
    EZ_TEST_FLOAT(-15.0f, ezMath::Floor(-12.34f, 5.0f), 0.0000001f);
  }

  EZ_TEST_BLOCK(true, "CeilM")
  {
    EZ_TEST_FLOAT( 15.0f, ezMath::Ceil(12.34f, 5.0f), 0.0000001f);
    EZ_TEST_FLOAT(-10.0f, ezMath::Ceil(-12.34f, 5.0f), 0.0000001f);
  }

  EZ_TEST_BLOCK(true, "FloorIM")
  {
    EZ_TEST(10 == ezMath::Floor(11, 5));
    EZ_TEST(10 == ezMath::Floor(10, 5));
    EZ_TEST(5  == ezMath::Floor(9, 5));

    EZ_TEST(-10== ezMath::Floor(-6, 5));
    EZ_TEST(-10== ezMath::Floor(-10, 5));
  }

  EZ_TEST_BLOCK(true, "CeilIM")
  {
    EZ_TEST(15 == ezMath::Ceil(11, 5));
    EZ_TEST(10 == ezMath::Ceil(10, 5));
    EZ_TEST(10  == ezMath::Ceil(9, 5));

    EZ_TEST(-5== ezMath::Ceil(-6, 5));
    EZ_TEST(-10== ezMath::Ceil(-10, 5));
  }

  EZ_TEST_BLOCK(true, "Trunc")
  {
    EZ_TEST(ezMath::Trunc (12.34f) == 12);
    EZ_TEST(ezMath::Trunc (-12.34f) == -12);
  }

  EZ_TEST_BLOCK(true, "Round")
  {
    EZ_TEST(ezMath::Round(12.34f) == 12);
    EZ_TEST(ezMath::Round(-12.34f) == -12);

    EZ_TEST(ezMath::Round(12.54f) == 13);
    EZ_TEST(ezMath::Round(-12.54f) == -13);
  }

  EZ_TEST_BLOCK(true, "Round_Multiple")
  {
    EZ_TEST_FLOAT(ezMath::Round(12.34f, 7.0f), 14.0f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::Round(-12.34f, 7.0f), -14.0f, 0.00001f);
  }

  EZ_TEST_BLOCK(true, "Fraction")
  {
    EZ_TEST_FLOAT(ezMath::Fraction(12.34f), 0.34f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::Fraction(-12.34f), -0.34f, 0.00001f);
  }

  EZ_TEST_BLOCK(true, "FloatToInt")
  {
    EZ_TEST(ezMath::FloatToInt(12.34f) == 12);
    EZ_TEST(ezMath::FloatToInt(-12.34f) == -12);
  }

  EZ_TEST_BLOCK(true, "Mod")
  {
    EZ_TEST_FLOAT(2.34f, ezMath::Mod(12.34f, 2.5f), 0.000001f);
    EZ_TEST_FLOAT(-2.34f, ezMath::Mod(-12.34f, 2.5f), 0.000001f);
  }

  EZ_TEST_BLOCK(true, "Invert")
  {
    EZ_TEST_FLOAT(ezMath::Invert(1.0f), 1.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Invert(2.0f), 0.5f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Invert(4.0f), 0.25f, 0.000001f);

    EZ_TEST_FLOAT(ezMath::Invert(-1.0f), -1.0f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Invert(-2.0f), -0.5f, 0.000001f);
    EZ_TEST_FLOAT(ezMath::Invert(-4.0f), -0.25f, 0.000001f);
  }

  EZ_TEST_BLOCK(true, "Odd")
  {
    EZ_TEST(ezMath::IsOdd(0) == false);
    EZ_TEST(ezMath::IsOdd(1) == true);
    EZ_TEST(ezMath::IsOdd(2) == false);
    EZ_TEST(ezMath::IsOdd(-1) == true);
    EZ_TEST(ezMath::IsOdd(-2) == false);
  }

  EZ_TEST_BLOCK(true, "Even")
  {
    EZ_TEST(ezMath::IsEven(0) == true);
    EZ_TEST(ezMath::IsEven(1) == false);
    EZ_TEST(ezMath::IsEven(2) == true);
    EZ_TEST(ezMath::IsEven(-1) == false);
    EZ_TEST(ezMath::IsEven(-2) == true);
  }

  EZ_TEST_BLOCK(true, "Swap")
  {
    ezInt32 a = 1;
    ezInt32 b = 2;
    ezMath::Swap(a, b);
    EZ_TEST((a == 2) && (b == 1));
  }

  EZ_TEST_BLOCK(true, "Lerp")
  {
    EZ_TEST_FLOAT(ezMath::Lerp(-5.0f, 5.0f, 0.5f), 0.0f, 0.000001);
    EZ_TEST_FLOAT(ezMath::Lerp(0.0f, 5.0f, 0.5f), 2.5f, 0.000001);
    EZ_TEST_FLOAT(ezMath::Lerp(-5.0f, 5.0f, 0.0f), -5.0f, 0.000001);
    EZ_TEST_FLOAT(ezMath::Lerp(-5.0f, 5.0f, 1.0f),  5.0f, 0.000001);
  }

  EZ_TEST_BLOCK(true, "Step")
  {
    EZ_TEST_FLOAT(ezMath::Step(0.5f, 0.4f), 1.0f, 0.00001f);
    EZ_TEST_FLOAT(ezMath::Step(0.3f, 0.4f), 0.0f, 0.00001f);
  }

  EZ_TEST_BLOCK(true, "SmoothStep")
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

  EZ_TEST_BLOCK(true, "IsPowerOf")
  {
    EZ_TEST(ezMath::IsPowerOf(4, 2) == true);
    EZ_TEST(ezMath::IsPowerOf(5, 2) == false);
    EZ_TEST(ezMath::IsPowerOf(0, 2) == false);
    EZ_TEST(ezMath::IsPowerOf(1, 2) == true);

    EZ_TEST(ezMath::IsPowerOf(4, 3) == false);
    EZ_TEST(ezMath::IsPowerOf(3, 3) == true);
    EZ_TEST(ezMath::IsPowerOf(1, 3) == true);
    EZ_TEST(ezMath::IsPowerOf(27, 3) == true);
    EZ_TEST(ezMath::IsPowerOf(28, 3) == false);
  }

  EZ_TEST_BLOCK(true, "IsPowerOf2")
  {
    EZ_TEST(ezMath::IsPowerOf2(4) == true);
    EZ_TEST(ezMath::IsPowerOf2(5) == false);
    EZ_TEST(ezMath::IsPowerOf2(0) == false);
    EZ_TEST(ezMath::IsPowerOf2(1) == true);
  }

  EZ_TEST_BLOCK(true, "PowerOf2_Floor")
  {
    EZ_TEST(ezMath::PowerOfTwo_Floor(64) == 64);
    EZ_TEST(ezMath::PowerOfTwo_Floor(33) == 32);
    EZ_TEST(ezMath::PowerOfTwo_Floor(4) == 4);
    EZ_TEST(ezMath::PowerOfTwo_Floor(5) == 4);
    EZ_TEST(ezMath::PowerOfTwo_Floor(1) == 1);
    // strange case...
    EZ_TEST(ezMath::PowerOfTwo_Floor(0) == 1);
  }

  EZ_TEST_BLOCK(true, "PowerOf2_Ceil")
  {
    EZ_TEST(ezMath::PowerOfTwo_Ceil(64) == 64);
    EZ_TEST(ezMath::PowerOfTwo_Ceil(33) == 64);
    EZ_TEST(ezMath::PowerOfTwo_Ceil(4) == 4);
    EZ_TEST(ezMath::PowerOfTwo_Ceil(5) == 8);
    EZ_TEST(ezMath::PowerOfTwo_Ceil(1) == 1);
    EZ_TEST(ezMath::PowerOfTwo_Ceil(0) == 1);
  }

  EZ_TEST_BLOCK(true, "IsFloatEqual")
  {
    EZ_TEST(ezMath::IsFloatEqual(1.0f, 0.999f, 0.01f) == true);
    EZ_TEST(ezMath::IsFloatEqual(1.0f, 1.001f, 0.01f) == true);
    EZ_TEST(ezMath::IsFloatEqual(1.0f, 0.999f, 0.0001f) == false);
    EZ_TEST(ezMath::IsFloatEqual(1.0f, 1.001f, 0.0001f) == false);
  }

  EZ_TEST_BLOCK(true, "NaN_Infinity")
  {
    EZ_TEST(ezMath::IsNaN(ezMath::NaN()) == true);

    EZ_TEST(ezMath::Infinity() == ezMath::Infinity() - 1);
    EZ_TEST(ezMath::Infinity() == ezMath::Infinity() + 1);

    EZ_TEST(ezMath::IsNaN(ezMath::Infinity() - ezMath::Infinity()));

    EZ_TEST(!ezMath::IsFinite( ezMath::Infinity()));
    EZ_TEST(!ezMath::IsFinite(-ezMath::Infinity()));
    EZ_TEST(!ezMath::IsFinite(ezMath::NaN()));
    EZ_TEST(!ezMath::IsNaN(ezMath::Infinity()));
  }

  EZ_TEST_BLOCK(true, "IsInRange")
  {
    EZ_TEST(ezMath::IsInRange(1.0f, 0.0f, 2.0f) == true);
    EZ_TEST(ezMath::IsInRange(1.0f, 0.0f, 1.0f) == true);
    EZ_TEST(ezMath::IsInRange(1.0f, 1.0f, 2.0f) == true);
    EZ_TEST(ezMath::IsInRange(0.0f, 1.0f, 2.0f) == false);
    EZ_TEST(ezMath::IsInRange(3.0f, 0.0f, 2.0f) == false);
  }

  EZ_TEST_BLOCK(true, "IsZero")
  {
    EZ_TEST(ezMath::IsZero(0.009f, 0.01f) == true);
    EZ_TEST(ezMath::IsZero(0.001f, 0.01f) == true);
    EZ_TEST(ezMath::IsZero(0.009f, 0.0001f) == false);
    EZ_TEST(ezMath::IsZero(0.001f, 0.0001f) == false);
  }
}
