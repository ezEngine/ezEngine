#include <PCH.h>
#include <Foundation/Math/Angle.h>

EZ_CREATE_SIMPLE_TEST(Math, Angle)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "DegToRad")
  {
    EZ_TEST_FLOAT(ezAngle::DegToRad(0.0f), 0.0f, 0.00001f);
    EZ_TEST_FLOAT(ezAngle::DegToRad(45.0f), 0.785398163f, 0.00001f);
    EZ_TEST_FLOAT(ezAngle::DegToRad(90.0f), 1.570796327f, 0.00001f);
    EZ_TEST_FLOAT(ezAngle::DegToRad(120.0f), 2.094395102f, 0.00001f);
    EZ_TEST_FLOAT(ezAngle::DegToRad(170.0f), 2.967059728f, 0.00001f);
    EZ_TEST_FLOAT(ezAngle::DegToRad(180.0f), 3.141592654f, 0.00001f);
    EZ_TEST_FLOAT(ezAngle::DegToRad(250.0f), 4.36332313f, 0.00001f);
    EZ_TEST_FLOAT(ezAngle::DegToRad(320.0f), 5.585053606f, 0.00001f);
    EZ_TEST_FLOAT(ezAngle::DegToRad(360.0f), 6.283185307f, 0.00001f);
    EZ_TEST_FLOAT(ezAngle::DegToRad(700.0f), 12.217304764f, 0.00001f);
    EZ_TEST_FLOAT(ezAngle::DegToRad(-123.0f), -2.14675498f, 0.00001f);
    EZ_TEST_FLOAT(ezAngle::DegToRad(-1234.0f), -21.53736297f, 0.00001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RadToDeg")
  {
    EZ_TEST_FLOAT(ezAngle::RadToDeg(         0.0f), 0.0f    , 0.00001f);
    EZ_TEST_FLOAT(ezAngle::RadToDeg(0.785398163f ), 45.0f   , 0.00001f);
    EZ_TEST_FLOAT(ezAngle::RadToDeg(1.570796327f ), 90.0f   , 0.00001f);
    EZ_TEST_FLOAT(ezAngle::RadToDeg(2.094395102f ), 120.0f  , 0.00001f);
    EZ_TEST_FLOAT(ezAngle::RadToDeg(2.967059728f ), 170.0f  , 0.0001f);
    EZ_TEST_FLOAT(ezAngle::RadToDeg(3.141592654f ), 180.0f  , 0.00001f);
    EZ_TEST_FLOAT(ezAngle::RadToDeg(4.36332313f  ), 250.0f  , 0.0001f);
    EZ_TEST_FLOAT(ezAngle::RadToDeg(5.585053606f ), 320.0f  , 0.0001f);
    EZ_TEST_FLOAT(ezAngle::RadToDeg(6.283185307f ), 360.0f  , 0.00001f);
    EZ_TEST_FLOAT(ezAngle::RadToDeg(12.217304764f), 700.0f  , 0.00001f);
    EZ_TEST_FLOAT(ezAngle::RadToDeg(-2.14675498f ), -123.0f , 0.00001f);
    EZ_TEST_FLOAT(ezAngle::RadToDeg(-21.53736297f), -1234.0f, 0.001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Init")
  {
    ezAngle a0;
    EZ_TEST_FLOAT(a0.GetRadian(), 0.0f, 0.0f);
    EZ_TEST_FLOAT(a0.GetDegree(), 0.0f, 0.0f);

    ezAngle a1 = ezAngle::Radian(1.570796327f);
    EZ_TEST_FLOAT(a1.GetRadian(), 1.570796327f, 0.00001f);
    EZ_TEST_FLOAT(a1.GetDegree(), 90.0f, 0.00001f);

    ezAngle a2 = ezAngle::Degree(90);
    EZ_TEST_FLOAT(a2.GetRadian(), 1.570796327f, 0.00001f);
    EZ_TEST_FLOAT(a2.GetDegree(), 90.0f, 0.00001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "NormalizeRange / IsEqual ")
  {
    ezAngle a;

    for (ezInt32 i = 1; i < 359; i++)
    {
      a = ezAngle::Degree((float) i);             a.NormalizeRange(); EZ_TEST_FLOAT(a.GetDegree(), (float) i, 0.01f);
      a = ezAngle::Degree((float) i);             a.NormalizeRange(); EZ_TEST_FLOAT(a.GetDegree(), (float) i, 0.01f);
      a = ezAngle::Degree((float) i + 360.0f);    a.NormalizeRange(); EZ_TEST_FLOAT(a.GetDegree(), (float) i, 0.01f);
      a = ezAngle::Degree((float) i - 360.0f);    a.NormalizeRange(); EZ_TEST_FLOAT(a.GetDegree(), (float) i, 0.01f);
      a = ezAngle::Degree((float) i + 3600.0f);   a.NormalizeRange(); EZ_TEST_FLOAT(a.GetDegree(), (float) i, 0.01f);
      a = ezAngle::Degree((float) i - 3600.0f);   a.NormalizeRange(); EZ_TEST_FLOAT(a.GetDegree(), (float) i, 0.01f);
      a = ezAngle::Degree((float) i + 36000.0f);  a.NormalizeRange(); EZ_TEST_FLOAT(a.GetDegree(), (float) i, 0.01f);
      a = ezAngle::Degree((float) i - 36000.0f);  a.NormalizeRange(); EZ_TEST_FLOAT(a.GetDegree(), (float) i, 0.01f);
    }

    for (ezInt32 i = 0; i < 360; i++)
    {
      a = ezAngle::Degree((float) i);             EZ_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(ezAngle::Degree((float) i), ezAngle::Degree(0.01f)));
      a = ezAngle::Degree((float) i);             EZ_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(ezAngle::Degree((float) i), ezAngle::Degree(0.01f)));
      a = ezAngle::Degree((float) i + 360.0f);    EZ_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(ezAngle::Degree((float) i), ezAngle::Degree(0.01f)));
      a = ezAngle::Degree((float) i - 360.0f);    EZ_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(ezAngle::Degree((float) i), ezAngle::Degree(0.01f)));
      a = ezAngle::Degree((float) i + 3600.0f);   EZ_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(ezAngle::Degree((float) i), ezAngle::Degree(0.01f)));
      a = ezAngle::Degree((float) i - 3600.0f);   EZ_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(ezAngle::Degree((float) i), ezAngle::Degree(0.01f)));
      a = ezAngle::Degree((float) i + 36000.0f);  EZ_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(ezAngle::Degree((float) i), ezAngle::Degree(0.01f)));
      a = ezAngle::Degree((float) i - 36000.0f);  EZ_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(ezAngle::Degree((float) i), ezAngle::Degree(0.01f)));
    }

    for (ezInt32 i = 0; i < 360; i++)
    {
      a = ezAngle::Degree((float) i);             EZ_TEST_BOOL(a.IsEqualNormalized(ezAngle::Degree((float) i), ezAngle::Degree(0.01f)));
      a = ezAngle::Degree((float) i);             EZ_TEST_BOOL(a.IsEqualNormalized(ezAngle::Degree((float) i), ezAngle::Degree(0.01f)));
      a = ezAngle::Degree((float) i + 360.0f);    EZ_TEST_BOOL(a.IsEqualNormalized(ezAngle::Degree((float) i), ezAngle::Degree(0.01f)));
      a = ezAngle::Degree((float) i - 360.0f);    EZ_TEST_BOOL(a.IsEqualNormalized(ezAngle::Degree((float) i), ezAngle::Degree(0.01f)));
      a = ezAngle::Degree((float) i + 3600.0f);   EZ_TEST_BOOL(a.IsEqualNormalized(ezAngle::Degree((float) i), ezAngle::Degree(0.01f)));
      a = ezAngle::Degree((float) i - 3600.0f);   EZ_TEST_BOOL(a.IsEqualNormalized(ezAngle::Degree((float) i), ezAngle::Degree(0.01f)));
      a = ezAngle::Degree((float) i + 36000.0f);  EZ_TEST_BOOL(a.IsEqualNormalized(ezAngle::Degree((float) i), ezAngle::Degree(0.01f)));
      a = ezAngle::Degree((float) i - 36000.0f);  EZ_TEST_BOOL(a.IsEqualNormalized(ezAngle::Degree((float) i), ezAngle::Degree(0.01f)));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AngleBetween")
  {
    EZ_TEST_FLOAT(ezAngle::AngleBetween(ezAngle::Degree(0),   ezAngle::Degree(  0)).GetDegree(), 0.0f, 0.0001f);
    EZ_TEST_FLOAT(ezAngle::AngleBetween(ezAngle::Degree(0),   ezAngle::Degree(360)).GetDegree(), 0.0f, 0.0001f);
    EZ_TEST_FLOAT(ezAngle::AngleBetween(ezAngle::Degree(360), ezAngle::Degree(360)).GetDegree(), 0.0f, 0.0001f);
    EZ_TEST_FLOAT(ezAngle::AngleBetween(ezAngle::Degree(360), ezAngle::Degree(  0)).GetDegree(), 0.0f, 0.0001f);

    EZ_TEST_FLOAT(ezAngle::AngleBetween(ezAngle::Degree(5),   ezAngle::Degree(186)).GetDegree(), 179.0f, 0.0001f);
    EZ_TEST_FLOAT(ezAngle::AngleBetween(ezAngle::Degree(-5),   ezAngle::Degree(-186)).GetDegree(), 179.0f, 0.0001f);

    EZ_TEST_FLOAT(ezAngle::AngleBetween(ezAngle::Degree(360.0f +  5),  ezAngle::Degree( 360.0f + 186)).GetDegree(), 179.0f, 0.0001f);
    EZ_TEST_FLOAT(ezAngle::AngleBetween(ezAngle::Degree(360.0f + -5),  ezAngle::Degree( 360.0f - 186)).GetDegree(), 179.0f, 0.0001f);

    for (ezInt32 i = 0; i <= 179; ++i)
      EZ_TEST_FLOAT(ezAngle::AngleBetween(ezAngle::Degree((float) i), ezAngle::Degree((float) (i + i))).GetDegree(), (float) i, 0.0001f);

    for (ezInt32 i = -179; i <= 0; ++i)
      EZ_TEST_FLOAT(ezAngle::AngleBetween(ezAngle::Degree((float) i), ezAngle::Degree((float) (i + i))).GetDegree(), (float) -i, 0.0001f);
  }
}
