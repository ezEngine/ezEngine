#include <PCH.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Color8UNorm.h>



EZ_CREATE_SIMPLE_TEST(Math, Color)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    if (ezMath::BasicType<ezMathTestType>::SupportsNaN())
    {
      // In debug the default constructor initializes everything with NaN.
      ezColor defCtor;
      EZ_TEST(ezMath::IsNaN(defCtor.r) && ezMath::IsNaN(defCtor.g) && ezMath::IsNaN(defCtor.b) && ezMath::IsNaN(defCtor.a));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    float testBlock[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
    ezColor* pDefCtor = ::new ((void*)&testBlock[0]) ezColor;
    EZ_TEST(pDefCtor->r == 1.0f && 
            pDefCtor->g == 2.0f && 
            pDefCtor->b == 3.0f && 
            pDefCtor->a == 4.0f);
#endif

    // Make sure the class didn't accidentally change in size
    EZ_TEST(sizeof(ezColor) == sizeof(float)*4);

    ezColor init3F(0.5f, 0.6f, 0.7f);
    EZ_TEST(init3F.r == 0.5f && init3F.g == 0.6f && init3F.b == 0.7f && init3F.a == 1.0f);

    ezColor init4F(0.5f, 0.6f, 0.7f, 0.8);
    EZ_TEST(init4F.r == 0.5f && init4F.g == 0.6f && init4F.b == 0.7f && init4F.a == 0.8f);

    ezColor copy(init4F);
    EZ_TEST(copy.r == 0.5f && copy.g == 0.6f && copy.b == 0.7f && copy.a == 0.8f);

    ezColor initV(ezVec4(0.5f, 0.6f, 0.7f, 0.8));
    EZ_TEST(initV.r == 0.5f && initV.g == 0.6f && initV.b == 0.7f && initV.a == 0.8f);

    ezColor initVd(ezVec4d(0.5, 0.6, 0.7, 0.8));
    EZ_TEST(initVd.r == 0.5f && initVd.g == 0.6f && initVd.b == 0.7f && initVd.a == 0.8f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Conversion")
  {
    ezColor cornflowerBlue(ezColor::GetCornflowerBlue());

    ezVec4d vec4d = static_cast<ezVec4d>(cornflowerBlue);
    EZ_TEST(ezMath::IsEqual<double>(vec4d.x, cornflowerBlue.r, ezMath::BasicType<double>::SmallEpsilon()) &&
            ezMath::IsEqual<double>(vec4d.y, cornflowerBlue.g, ezMath::BasicType<double>::SmallEpsilon()) &&
            ezMath::IsEqual<double>(vec4d.z, cornflowerBlue.b, ezMath::BasicType<double>::SmallEpsilon()) &&
            ezMath::IsEqual<double>(vec4d.w, cornflowerBlue.a, ezMath::BasicType<double>::SmallEpsilon()));
    ezVec4 vec4f = static_cast<ezVec4>(cornflowerBlue);
    EZ_TEST(vec4f.x == cornflowerBlue.r && vec4f.y == cornflowerBlue.g && vec4f.z == cornflowerBlue.b && vec4f.w == cornflowerBlue.a);  // exact!

    float* pFloats = static_cast<float*>(cornflowerBlue);
    EZ_TEST(pFloats[0] == cornflowerBlue.r && pFloats[1] == cornflowerBlue.g && pFloats[2] == cornflowerBlue.b && pFloats[3] == cornflowerBlue.a);

    const float* pConstFloats = static_cast<const float*>(cornflowerBlue);
    EZ_TEST(pConstFloats[0] == cornflowerBlue.r && pConstFloats[1] == cornflowerBlue.g && pConstFloats[2] == cornflowerBlue.b && pConstFloats[3] == cornflowerBlue.a);

    ezVec3d vec3RGBd = cornflowerBlue.GetRGB<double>();
    EZ_TEST(ezMath::IsEqual<double>(vec3RGBd.x, cornflowerBlue.r, ezMath::BasicType<double>::SmallEpsilon()) &&
            ezMath::IsEqual<double>(vec3RGBd.y, cornflowerBlue.g, ezMath::BasicType<double>::SmallEpsilon()) &&
            ezMath::IsEqual<double>(vec3RGBd.z, cornflowerBlue.b, ezMath::BasicType<double>::SmallEpsilon()));
    ezVec3 vec3RGBf = cornflowerBlue.GetRGB<float>();
    EZ_TEST(vec3RGBf.x == cornflowerBlue.r && vec3RGBf.y == cornflowerBlue.g && vec3RGBf.z == cornflowerBlue.b);

    ezVec3d vec3BGRd = cornflowerBlue.GetBGR<double>();
    EZ_TEST(ezMath::IsEqual<double>(vec3BGRd.x, cornflowerBlue.b, ezMath::BasicType<double>::SmallEpsilon()) &&
            ezMath::IsEqual<double>(vec3BGRd.y, cornflowerBlue.g, ezMath::BasicType<double>::SmallEpsilon()) &&
            ezMath::IsEqual<double>(vec3BGRd.z, cornflowerBlue.r, ezMath::BasicType<double>::SmallEpsilon()));
    ezVec3 vec3BGRf = cornflowerBlue.GetBGR<float>();
    EZ_TEST(vec3BGRf.x == cornflowerBlue.b && vec3BGRf.y == cornflowerBlue.g && vec3BGRf.z == cornflowerBlue.r);

    ezColor black0(0,0,0);
    black0.SetRGB(vec3RGBd);
    EZ_TEST(ezMath::IsEqual<double>(black0.r, cornflowerBlue.r, ezMath::BasicType<double>::SmallEpsilon()) &&
            ezMath::IsEqual<double>(black0.g, cornflowerBlue.g, ezMath::BasicType<double>::SmallEpsilon()) &&
            ezMath::IsEqual<double>(black0.b, cornflowerBlue.b, ezMath::BasicType<double>::SmallEpsilon()));
    black0.SetRGB(vec3RGBf);
    EZ_TEST(black0.r == cornflowerBlue.r && black0.g == cornflowerBlue.g && black0.b == cornflowerBlue.b);
    

    ezColor black1(0,0,0);
    black1.SetBGR(vec3BGRd);
    EZ_TEST(ezMath::IsEqual<double>(black1.r, cornflowerBlue.r, ezMath::BasicType<double>::SmallEpsilon()) &&
      ezMath::IsEqual<double>(black1.g, cornflowerBlue.g, ezMath::BasicType<double>::SmallEpsilon()) &&
      ezMath::IsEqual<double>(black1.b, cornflowerBlue.b, ezMath::BasicType<double>::SmallEpsilon()));
    black1.SetBGR(vec3BGRf);
    EZ_TEST(black1.r == cornflowerBlue.r && black1.g == cornflowerBlue.g && black1.b == cornflowerBlue.b);
  }

  
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Color conversions")
  {
    ezColor normalizedColor(0.0f, 1.0f, 0.999f, 0.0001f);
    EZ_TEST(normalizedColor.IsNormalized());
    ezColor notNormalizedColor0(-0.01f, 1.0f, 0.999f, 0.0001f);
    EZ_TEST(!notNormalizedColor0.IsNormalized());
    ezColor notNormalizedColor1(0.5f, 1.1f, 0.9f, 0.1f);
    EZ_TEST(!notNormalizedColor1.IsNormalized());
    ezColor notNormalizedColor2(0.1f, 1.0f, 1.999f, 0.1f);
    EZ_TEST(!notNormalizedColor2.IsNormalized());
    ezColor notNormalizedColor3(0.1f, 1.0f, 1.0f, -0.1f);
    EZ_TEST(!notNormalizedColor3.IsNormalized());


    // hsv test - took some samples from http://www.javascripter.net/faq/rgb2hsv.htm
    ezVec3 rgb[] = { ezVec3(1,1,1), ezVec3(0,0,0), ezVec3(123, 12, 1) / 255.0f, ezVec3(31, 112, 153) / 255.0f };
    ezVec3 hsv[] = { ezVec3(0,0,1), ezVec3(0,0,0), ezVec3(5.4f, 0.991f, 0.48f), ezVec3(200.2f, 0.797f, 0.600f)};
    for(int i=0; i<4; ++i)
    {
      ezColor color;
      color.SetRGB(rgb[i]);
      ezVec3 hsvConversionResult(color.ConvertToHSV<float>());
      // needs bigger 
      EZ_TEST(hsvConversionResult.IsEqual(hsv[i], 0.1f));

      ezColor fromHSV = ezColor::FromHSV(hsv[i]);
      EZ_TEST(fromHSV.GetRGB<float>().IsEqual(rgb[i], 0.01f));
    }


    // Still missing. Problem: Find good reference values
    //float GetSaturation() const;
    //float GetLuminance() const;
    //ezColor GetInvertedColor() const;
    //ezColor ConvertLinearToSRGB() const;
    //ezColor ConvertSRGBToLinear() const;
  }

  EZ_TEST_BLOCK(true, "Numeric properties")
  {
    if (ezMath::BasicType<ezMathTestType>::SupportsNaN())
    {
      float fNaN = ezMath::BasicType<float>::GetNaN();
      const ezColor nanArray[4] = {
              ezColor(fNaN, 0.0f, 0.0f, 0.0f),
              ezColor(0.0f, fNaN, 0.0f, 0.0f),
              ezColor(0.0f, 0.0f, fNaN, 0.0f),
              ezColor(0.0f, 0.0f, 0.0f, fNaN) };
      const ezColor compArray[4] = { 
              ezColor(1.0f, 0.0f, 0.0f, 0.0f),
              ezColor(0.0f, 1.0f, 0.0f, 0.0f),
              ezColor(0.0f, 0.0f, 1.0f, 0.0f),
              ezColor(0.0f, 0.0f, 0.0f, 1.0f) };


      // IsNaN
      for (int i = 0; i < 4; ++i)
      {
        EZ_TEST(nanArray[i].IsNaN());
        EZ_TEST(!compArray[i].IsNaN());
      }

      // IsValid
      for (int i = 0; i < 4; ++i)
      {
        EZ_TEST(!nanArray[i].IsValid());
        EZ_TEST(compArray[i].IsValid());

        EZ_TEST(!(compArray[i] * ezMath::BasicType<float>::GetInfinity()).IsValid());
        EZ_TEST(!(compArray[i] * -ezMath::BasicType<float>::GetInfinity()).IsValid());
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Operators")
  {
    const ezColor op1(-4.0, 0.2f, -7.0f, -0.0f);
    const ezColor op2( 2.0, 0.3f,  0.0f,  1.0f);
    const ezColor compArray[4] = { ezColor(1.0f, 0.0f, 0.0f, 0.0f),
      ezColor(0.0f, 1.0f, 0.0f, 0.0f),
      ezColor(0.0f, 0.0f, 1.0f, 0.0f),
      ezColor(0.0f, 0.0f, 0.0f, 1.0f) };
    // IsIdentical
    EZ_TEST(op1.IsIdentical(op1));
    for (int i = 0; i < 4; ++i)
    {
      EZ_TEST(!op1.IsIdentical(op1 + ezMath::BasicType<float>::SmallEpsilon() * compArray[i]));
      EZ_TEST(!op1.IsIdentical(op1 - ezMath::BasicType<float>::SmallEpsilon() * compArray[i]));
    }

    // IsEqual
    EZ_TEST(op1.IsEqual(op1, 0.0f));
    for (int i = 0; i < 4; ++i)
    {
      EZ_TEST(op1.IsEqual(op1 + ezMath::BasicType<float>::SmallEpsilon() * compArray[i], ezMath::BasicType<float>::SmallEpsilon()));
      EZ_TEST(op1.IsEqual(op1 - ezMath::BasicType<float>::SmallEpsilon() * compArray[i], ezMath::BasicType<float>::SmallEpsilon()));
      EZ_TEST(op1.IsEqual(op1 + ezMath::BasicType<float>::DefaultEpsilon() * compArray[i], ezMath::BasicType<float>::DefaultEpsilon()));
      EZ_TEST(op1.IsEqual(op1 - ezMath::BasicType<float>::DefaultEpsilon() * compArray[i], ezMath::BasicType<float>::DefaultEpsilon()));
    }

    // operator+= (ezColor)
    ezColor plusAssign = op1;
    plusAssign += op2;
    EZ_TEST(plusAssign.IsEqual(ezColor(-2.0f, 0.5f, -7.0f, 1.0f), ezMath::BasicType<float>::SmallEpsilon()));

    // operator-= (ezColor)
    ezColor minusAssign = op1;
    minusAssign -= op2;
    EZ_TEST(minusAssign.IsEqual(ezColor(-6.0f, -0.1f, -7.0f, -1.0f), ezMath::BasicType<float>::SmallEpsilon()));

    // operator*= (float)
    ezColor mulFloat = op1;
    mulFloat *= 2.0f;
    EZ_TEST(mulFloat.IsEqual(ezColor(-8.0f, 0.4f, -14.0f, -0.0f), ezMath::BasicType<float>::SmallEpsilon()));
    mulFloat *= 0.0f;
    EZ_TEST(mulFloat.IsEqual(ezColor(0.0f, 0.0f, 0.0f, 0.0f), ezMath::BasicType<float>::SmallEpsilon()));

    // operator/= (float)
    ezColor vDivFloat = op1;
    vDivFloat /= 2.0f;
    EZ_TEST(vDivFloat.IsEqual(ezColor(-2.0f, 0.1f, -3.5f, -0.0f), ezMath::BasicType<float>::SmallEpsilon()));

    //operator+ (ezColor, ezColor)
    ezColor plus = (op1 + op2);
    EZ_TEST(plus.IsEqual(ezColor(-2.0f, 0.5f, -7.0f, 1.0f), ezMath::BasicType<float>::SmallEpsilon()));

    //operator- (ezColor, ezColor)
    ezColor minus = (op1 - op2);
    EZ_TEST(minus.IsEqual(ezColor(-6.0f, -0.1f, -7.0f, -1.0f), ezMath::BasicType<float>::SmallEpsilon()));

    // operator* (float, ezColor)
    ezColor mulFloatVec4 = 2 * op1;
    EZ_TEST(mulFloatVec4.IsEqual(ezColor(-8.0f, 0.4f, -14.0f, -0.0f), ezMath::BasicType<float>::SmallEpsilon()));
    mulFloatVec4 = ((float) 0 * op1);
    EZ_TEST(mulFloatVec4.IsEqual(ezColor(0.0f, 0.0f, 0.0f, 0.0f), ezMath::BasicType<float>::SmallEpsilon()));

    // operator* (ezColor, float)
    ezColor mulVec4Float = op1 * 2;
    EZ_TEST(mulVec4Float.IsEqual(ezColor(-8.0f, 0.4f, -14.0f, -0.0f), ezMath::BasicType<float>::SmallEpsilon()));
    mulVec4Float = (op1 * (float) 0);
    EZ_TEST(mulVec4Float.IsEqual(ezColor(0.0f, 0.0f, 0.0f, 0.0f), ezMath::BasicType<float>::SmallEpsilon()));

    // operator/ (ezColor, float)
    ezColor vDivVec4Float = op1 / 2;
    EZ_TEST(vDivVec4Float.IsEqual(ezColor(-2.0f, 0.1f, -3.5f, -0.0f), ezMath::BasicType<float>::SmallEpsilon()));

    // operator== (ezColor, ezColor)
    EZ_TEST(op1 == op1);
    for (int i = 0; i < 4; ++i)
    {
      EZ_TEST( !(op1 == (op1 + ezMath::BasicType<float>::SmallEpsilon() * compArray[i])) );
      EZ_TEST( !(op1 == (op1 - ezMath::BasicType<float>::SmallEpsilon() * compArray[i])) );
    }

    // operator!= (ezColor, ezColor)
    EZ_TEST(!(op1 != op1));
    for (int i = 0; i < 4; ++i)
    {
      EZ_TEST(op1 != (op1 + ezMath::BasicType<float>::SmallEpsilon() * compArray[i]));
      EZ_TEST(op1 != (op1 - ezMath::BasicType<float>::SmallEpsilon() * compArray[i]));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezColor8UNorm")
  {
    // construction
    {
      // Placement new of the default constructor should not have any effect on the previous data.
      ezUInt8 testBlock[4] = { 0, 64, 128, 255 };
      ezColor8UNorm* pDefCtor = ::new ((void*)&testBlock[0]) ezColor8UNorm;
      EZ_TEST(pDefCtor->r == 0 && 
        pDefCtor->g == 64 && 
        pDefCtor->b == 128 && 
        pDefCtor->a == 255);

      // Make sure the class didn't accidentally change in size
      EZ_TEST(sizeof(ezColor8UNorm) == sizeof(ezUInt8)*4);

      ezColor8UNorm init3(100, 123, 255);
      EZ_TEST(init3.r == 100 && init3.g == 123 && init3.b == 255 && init3.a == 255);

      ezColor8UNorm init4(100, 123, 255, 42);
      EZ_TEST(init4.r == 100 && init4.g == 123 && init4.b == 255 && init4.a == 42);

      ezColor8UNorm copy(init4);
      EZ_TEST(copy.r == 100 && copy.g == 123 && copy.b == 255 && copy.a == 42);

      ezColor8UNorm fromColor32f(ezColor::GetCornflowerBlue());
      EZ_TEST(ezMath::IsEqual<ezUInt8>(fromColor32f.r, static_cast<ezUInt8>(ezColor::GetCornflowerBlue().r * 255), 2) &&
              ezMath::IsEqual<ezUInt8>(fromColor32f.g, static_cast<ezUInt8>(ezColor::GetCornflowerBlue().g * 255), 2) &&
              ezMath::IsEqual<ezUInt8>(fromColor32f.b, static_cast<ezUInt8>(ezColor::GetCornflowerBlue().b * 255), 2) &&
              ezMath::IsEqual<ezUInt8>(fromColor32f.a, static_cast<ezUInt8>(ezColor::GetCornflowerBlue().a * 255), 2));
    }

    // conversion
    {
      ezColor8UNorm cornflowerBlue(ezColor::GetCornflowerBlue());

      ezColor color32f = static_cast<ezColor>(cornflowerBlue);
      EZ_TEST(ezMath::IsEqual<float>(color32f.r, ezColor::GetCornflowerBlue().r, 2.0f / 255.0f) &&
              ezMath::IsEqual<float>(color32f.g, ezColor::GetCornflowerBlue().g, 2.0f / 255.0f) &&
              ezMath::IsEqual<float>(color32f.b, ezColor::GetCornflowerBlue().b, 2.0f / 255.0f) &&
              ezMath::IsEqual<float>(color32f.a, ezColor::GetCornflowerBlue().a, 2.0f / 255.0f));

      const ezUInt8* pUIntsConst = static_cast<const ezUInt8*>(cornflowerBlue);
      EZ_TEST(pUIntsConst[0] == cornflowerBlue.r && pUIntsConst[1] == cornflowerBlue.g && pUIntsConst[2] == cornflowerBlue.b && pUIntsConst[3] == cornflowerBlue.a);

      ezUInt8* pUInts = static_cast<ezUInt8*>(cornflowerBlue);
      EZ_TEST(pUInts[0] == cornflowerBlue.r && pUInts[1] == cornflowerBlue.g && pUInts[2] == cornflowerBlue.b && pUInts[3] == cornflowerBlue.a);
    }

    // comparision
    {
      ezColor8UNorm comp0(1,2,3);
      ezColor8UNorm comp1(1,2,3);
      ezColor8UNorm comp2(11,22,33);

      EZ_TEST(comp0.IsIdentical(comp1));
      EZ_TEST(!comp0.IsIdentical(comp2));

      // operator ==
      EZ_TEST(comp0 == comp1);
      EZ_TEST(!(comp0 == comp2));
      // operator !=
      EZ_TEST(comp0 != comp2);
      EZ_TEST(!(comp0 != comp1));
    }
  }
}