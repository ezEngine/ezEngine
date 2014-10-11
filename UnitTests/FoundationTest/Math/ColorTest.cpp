#include <PCH.h>
#include <Foundation/Math/Color.h>

EZ_CREATE_SIMPLE_TEST(Math, Color)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor empty")
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    if (ezMath::BasicType<ezMathTestType>::SupportsNaN())
    {
      // In debug the default constructor initializes everything with NaN.
      ezColor defCtor;
      EZ_TEST_BOOL(ezMath::IsNaN(defCtor.r) && ezMath::IsNaN(defCtor.g) && ezMath::IsNaN(defCtor.b) && ezMath::IsNaN(defCtor.a));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    float testBlock[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
    ezColor* pDefCtor = ::new ((void*)&testBlock[0]) ezColor;
    EZ_TEST_BOOL(pDefCtor->r == 1.0f && 
      pDefCtor->g == 2.0f && 
      pDefCtor->b == 3.0f && 
      pDefCtor->a == 4.0f);
#endif

    // Make sure the class didn't accidentally change in size
    EZ_TEST_BOOL(sizeof(ezColor) == sizeof(float)*4);

  }
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor components")
  {
    ezColor init3F(0.5f, 0.6f, 0.7f);
    EZ_TEST_BOOL(init3F.r == 0.5f && init3F.g == 0.6f && init3F.b == 0.7f && init3F.a == 1.0f);

    ezColor init4F(0.5f, 0.6f, 0.7f, 0.8);
    EZ_TEST_BOOL(init4F.r == 0.5f && init4F.g == 0.6f && init4F.b == 0.7f && init4F.a == 0.8f);
  }
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor copy")
  {
    ezColor init4F(0.5f, 0.6f, 0.7f, 0.8);
    ezColor copy(init4F);
    EZ_TEST_BOOL(copy.r == 0.5f && copy.g == 0.6f && copy.b == 0.7f && copy.a == 0.8f);
  }
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor from vector")
  {
    ezColor initV(ezVec4(0.5f, 0.6f, 0.7f, 0.8));

    EZ_TEST_FLOAT(initV.r, 0.5f, 0.000001f);
    EZ_TEST_FLOAT(initV.g, 0.6f, 0.000001f);
    EZ_TEST_FLOAT(initV.b, 0.7f, 0.000001f);
    EZ_TEST_FLOAT(initV.a, 0.8f, 0.000001f);

    ezColor initVd(ezVec4d(0.5, 0.6, 0.7, 0.8));

    EZ_TEST_FLOAT(initVd.r, 0.5f, 0.000001f);
    EZ_TEST_FLOAT(initVd.g, 0.6f, 0.000001f);
    EZ_TEST_FLOAT(initVd.b, 0.7f, 0.000001f);
    EZ_TEST_FLOAT(initVd.a, 0.8f, 0.000001f);
  }

  {
    ezColor cornflowerBlue(ezColor::GetCornflowerBlue());

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "Conversion vec4")
    {
      ezVec4d vec4d = static_cast<ezVec4d>(cornflowerBlue);
      EZ_TEST_BOOL(ezMath::IsEqual<double>(vec4d.x, cornflowerBlue.r, ezMath::BasicType<double>::SmallEpsilon()) &&
        ezMath::IsEqual<double>(vec4d.y, cornflowerBlue.g, ezMath::BasicType<double>::SmallEpsilon()) &&
        ezMath::IsEqual<double>(vec4d.z, cornflowerBlue.b, ezMath::BasicType<double>::SmallEpsilon()) &&
        ezMath::IsEqual<double>(vec4d.w, cornflowerBlue.a, ezMath::BasicType<double>::SmallEpsilon()));
      ezVec4 vec4f = static_cast<ezVec4>(cornflowerBlue);
      EZ_TEST_BOOL(vec4f.x == cornflowerBlue.r && vec4f.y == cornflowerBlue.g && vec4f.z == cornflowerBlue.b && vec4f.w == cornflowerBlue.a);  // exact!
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "Conversion float")
    {
      float* pFloats = static_cast<float*>(cornflowerBlue);
      EZ_TEST_BOOL(pFloats[0] == cornflowerBlue.r && pFloats[1] == cornflowerBlue.g && pFloats[2] == cornflowerBlue.b && pFloats[3] == cornflowerBlue.a);

      const float* pConstFloats = static_cast<const float*>(cornflowerBlue);
      EZ_TEST_BOOL(pConstFloats[0] == cornflowerBlue.r && pConstFloats[1] == cornflowerBlue.g && pConstFloats[2] == cornflowerBlue.b && pConstFloats[3] == cornflowerBlue.a);
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "Get/Set RGB/BGR")
    {
      ezVec3d vec3RGBd = cornflowerBlue.GetRGB<double>();
      EZ_TEST_DOUBLE(vec3RGBd.x, cornflowerBlue.r, ezMath::BasicType<double>::SmallEpsilon());
      EZ_TEST_DOUBLE(vec3RGBd.y, cornflowerBlue.g, ezMath::BasicType<double>::SmallEpsilon());
      EZ_TEST_DOUBLE(vec3RGBd.z, cornflowerBlue.b, ezMath::BasicType<double>::SmallEpsilon());

      ezVec3 vec3RGBf = cornflowerBlue.GetRGB<float>();
      EZ_TEST_BOOL(vec3RGBf.x == cornflowerBlue.r && vec3RGBf.y == cornflowerBlue.g && vec3RGBf.z == cornflowerBlue.b);

      ezVec3d vec3BGRd = cornflowerBlue.GetBGR<double>();
      EZ_TEST_DOUBLE(vec3BGRd.x, cornflowerBlue.b, ezMath::BasicType<double>::SmallEpsilon());
      EZ_TEST_DOUBLE(vec3BGRd.y, cornflowerBlue.g, ezMath::BasicType<double>::SmallEpsilon());
      EZ_TEST_DOUBLE(vec3BGRd.z, cornflowerBlue.r, ezMath::BasicType<double>::SmallEpsilon());

      ezVec3 vec3BGRf = cornflowerBlue.GetBGR<float>();
      EZ_TEST_BOOL(vec3BGRf.x == cornflowerBlue.b && vec3BGRf.y == cornflowerBlue.g && vec3BGRf.z == cornflowerBlue.r);

      ezColor black0(0,0,0,0);
      black0.SetRGB(vec3RGBd);
      EZ_TEST_DOUBLE(black0.r, cornflowerBlue.r, ezMath::BasicType<double>::SmallEpsilon());
      EZ_TEST_DOUBLE(black0.g, cornflowerBlue.g, ezMath::BasicType<double>::SmallEpsilon());
      EZ_TEST_DOUBLE(black0.b, cornflowerBlue.b, ezMath::BasicType<double>::SmallEpsilon());
      EZ_TEST_DOUBLE(black0.a, 1.0f, ezMath::BasicType<double>::SmallEpsilon());

      black0.SetRGB(vec3RGBf);
      EZ_TEST_BOOL(black0.r == cornflowerBlue.r && black0.g == cornflowerBlue.g && black0.b == cornflowerBlue.b);

      ezColor black1(0,0,0,0);
      black1.SetBGR(vec3BGRd);
      EZ_TEST_DOUBLE(black1.r, cornflowerBlue.r, ezMath::BasicType<double>::SmallEpsilon());
      EZ_TEST_DOUBLE(black1.g, cornflowerBlue.g, ezMath::BasicType<double>::SmallEpsilon());
      EZ_TEST_DOUBLE(black1.b, cornflowerBlue.b, ezMath::BasicType<double>::SmallEpsilon());
      EZ_TEST_DOUBLE(black1.a, 1.0f, ezMath::BasicType<double>::SmallEpsilon());

      black1.SetBGR(vec3BGRf);
      EZ_TEST_BOOL(black1.r == cornflowerBlue.r && black1.g == cornflowerBlue.g && black1.b == cornflowerBlue.b);
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "Get/Set RGB/BGR")
    {
      ezColor c1(1, 2, 3, 4);
      c1.SetRGB(ezVec3(5, 6, 7));

      ezVec4 v4 = c1.GetRGBA<float>();
      EZ_TEST_VEC4(v4, ezVec4(5, 6, 7, 1), 0.0f);

      v4 = c1.GetBGRA<float>();
      EZ_TEST_VEC4(v4, ezVec4(7, 6, 5, 1), 0.0f);

      c1.SetRGBA(ezVec4(2, 4, 6, 8));

      v4 = c1.GetRGBA<float>();
      EZ_TEST_VEC4(v4, ezVec4(2, 4, 6, 8), 0.0f);

      c1.SetBGRA(ezVec4(3, 5, 7, 9));

      v4 = c1.GetBGRA<float>();
      EZ_TEST_VEC4(v4, ezVec4(3, 5, 7, 9), 0.0f);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "HSV conversion")
  {
    ezColor normalizedColor(0.0f, 1.0f, 0.999f, 0.0001f);
    EZ_TEST_BOOL(normalizedColor.IsNormalized());
    ezColor notNormalizedColor0(-0.01f, 1.0f, 0.999f, 0.0001f);
    EZ_TEST_BOOL(!notNormalizedColor0.IsNormalized());
    ezColor notNormalizedColor1(0.5f, 1.1f, 0.9f, 0.1f);
    EZ_TEST_BOOL(!notNormalizedColor1.IsNormalized());
    ezColor notNormalizedColor2(0.1f, 1.0f, 1.999f, 0.1f);
    EZ_TEST_BOOL(!notNormalizedColor2.IsNormalized());
    ezColor notNormalizedColor3(0.1f, 1.0f, 1.0f, -0.1f);
    EZ_TEST_BOOL(!notNormalizedColor3.IsNormalized());


    // hsv test - took some samples from http://www.javascripter.net/faq/rgb2hsv.htm
    ezVec3 rgb[] = { ezVec3(1,1,1), ezVec3(0,0,0), ezVec3(123, 12, 1) / 255.0f, ezVec3(31, 112, 153) / 255.0f };
    ezVec3 hsv[] = { ezVec3(0,0,1), ezVec3(0,0,0), ezVec3(5.4f, 0.991f, 0.48f), ezVec3(200.2f, 0.797f, 0.600f)};
    for (int i=0; i<4; ++i)
    {
      ezColor color;
      color.SetRGB(rgb[i]);
      ezVec3 hsvConversionResult(color.ConvertToHSV<float>());
      // needs bigger 
      EZ_TEST_BOOL(hsvConversionResult.IsEqual(hsv[i], 0.1f));

      ezColor fromHSV = ezColor::FromHSV(hsv[i]);
      EZ_TEST_BOOL(fromHSV.GetRGB<float>().IsEqual(rgb[i], 0.01f));
    }
  }

  /// \todo Missing ezColor Tests: Problem: Find good reference values
  //float GetSaturation() const;
  //float GetLuminance() const;
  //ezColor GetInvertedColor() const;
  //ezColor ConvertLinearToSRGB() const;
  //ezColor ConvertSRGBToLinear() const;

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


      EZ_TEST_BLOCK(true, "IsNaN") 
      {
        for (int i = 0; i < 4; ++i)
        {
          EZ_TEST_BOOL(nanArray[i].IsNaN());
          EZ_TEST_BOOL(!compArray[i].IsNaN());
        }
      }

      EZ_TEST_BLOCK(true, "IsValid")
      {
        for (int i = 0; i < 4; ++i)
        {
          EZ_TEST_BOOL(!nanArray[i].IsValid());
          EZ_TEST_BOOL(compArray[i].IsValid());

          EZ_TEST_BOOL(!(compArray[i] * ezMath::BasicType<float>::GetInfinity()).IsValid());
          EZ_TEST_BOOL(!(compArray[i] * -ezMath::BasicType<float>::GetInfinity()).IsValid());
        }
      }
    }
  }

  {
    const ezColor op1(-4.0, 0.2f, -7.0f, -0.0f);
    const ezColor op2( 2.0, 0.3f,  0.0f,  1.0f);
    const ezColor compArray[4] = { ezColor(1.0f, 0.0f, 0.0f, 0.0f),
      ezColor(0.0f, 1.0f, 0.0f, 0.0f),
      ezColor(0.0f, 0.0f, 1.0f, 0.0f),
      ezColor(0.0f, 0.0f, 0.0f, 1.0f) };

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsIdentical")
    {
      EZ_TEST_BOOL(op1.IsIdentical(op1));
      for (int i = 0; i < 4; ++i)
      {
        EZ_TEST_BOOL(!op1.IsIdentical(op1 + ezMath::BasicType<float>::SmallEpsilon() * compArray[i]));
        EZ_TEST_BOOL(!op1.IsIdentical(op1 - ezMath::BasicType<float>::SmallEpsilon() * compArray[i]));
      }
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsEqual")
    {
      EZ_TEST_BOOL(op1.IsEqual(op1, 0.0f));
      for (int i = 0; i < 4; ++i)
      {
        EZ_TEST_BOOL(op1.IsEqual(op1 + ezMath::BasicType<float>::SmallEpsilon()   * compArray[i], 2 * ezMath::BasicType<float>::SmallEpsilon()));
        EZ_TEST_BOOL(op1.IsEqual(op1 - ezMath::BasicType<float>::SmallEpsilon()   * compArray[i], 2 * ezMath::BasicType<float>::SmallEpsilon()));
        EZ_TEST_BOOL(op1.IsEqual(op1 + ezMath::BasicType<float>::DefaultEpsilon() * compArray[i], 2 * ezMath::BasicType<float>::DefaultEpsilon()));
        EZ_TEST_BOOL(op1.IsEqual(op1 - ezMath::BasicType<float>::DefaultEpsilon() * compArray[i], 2 * ezMath::BasicType<float>::DefaultEpsilon()));
      }
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator+= (ezColor)")
    {
      ezColor plusAssign = op1;
      plusAssign += op2;
      EZ_TEST_BOOL(plusAssign.IsEqual(ezColor(-2.0f, 0.5f, -7.0f, 1.0f), ezMath::BasicType<float>::SmallEpsilon()));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator-= (ezColor)")
    {
      ezColor minusAssign = op1;
      minusAssign -= op2;
      EZ_TEST_BOOL(minusAssign.IsEqual(ezColor(-6.0f, -0.1f, -7.0f, -1.0f), ezMath::BasicType<float>::SmallEpsilon()));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "ooperator*= (float)")
    {
      ezColor mulFloat = op1;
      mulFloat *= 2.0f;
      EZ_TEST_BOOL(mulFloat.IsEqual(ezColor(-8.0f, 0.4f, -14.0f, -0.0f), ezMath::BasicType<float>::SmallEpsilon()));
      mulFloat *= 0.0f;
      EZ_TEST_BOOL(mulFloat.IsEqual(ezColor(0.0f, 0.0f, 0.0f, 0.0f), ezMath::BasicType<float>::SmallEpsilon()));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator/= (float)")
    {
      ezColor vDivFloat = op1;
      vDivFloat /= 2.0f;
      EZ_TEST_BOOL(vDivFloat.IsEqual(ezColor(-2.0f, 0.1f, -3.5f, -0.0f), ezMath::BasicType<float>::SmallEpsilon()));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator+ (ezColor, ezColor)")
    {
      ezColor plus = (op1 + op2);
      EZ_TEST_BOOL(plus.IsEqual(ezColor(-2.0f, 0.5f, -7.0f, 1.0f), ezMath::BasicType<float>::SmallEpsilon()));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator- (ezColor, ezColor)")
    {
      ezColor minus = (op1 - op2);
      EZ_TEST_BOOL(minus.IsEqual(ezColor(-6.0f, -0.1f, -7.0f, -1.0f), ezMath::BasicType<float>::SmallEpsilon()));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator* (float, ezColor)")
    {
      ezColor mulFloatVec4 = 2 * op1;
      EZ_TEST_BOOL(mulFloatVec4.IsEqual(ezColor(-8.0f, 0.4f, -14.0f, -0.0f), ezMath::BasicType<float>::SmallEpsilon()));
      mulFloatVec4 = ((float) 0 * op1);
      EZ_TEST_BOOL(mulFloatVec4.IsEqual(ezColor(0.0f, 0.0f, 0.0f, 0.0f), ezMath::BasicType<float>::SmallEpsilon()));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator* (ezColor, float)")
    {
      ezColor mulVec4Float = op1 * 2;
      EZ_TEST_BOOL(mulVec4Float.IsEqual(ezColor(-8.0f, 0.4f, -14.0f, -0.0f), ezMath::BasicType<float>::SmallEpsilon()));
      mulVec4Float = (op1 * (float) 0);
      EZ_TEST_BOOL(mulVec4Float.IsEqual(ezColor(0.0f, 0.0f, 0.0f, 0.0f), ezMath::BasicType<float>::SmallEpsilon()));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator/ (ezColor, float)")
    {
      ezColor vDivVec4Float = op1 / 2;
      EZ_TEST_BOOL(vDivVec4Float.IsEqual(ezColor(-2.0f, 0.1f, -3.5f, -0.0f), ezMath::BasicType<float>::SmallEpsilon()));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator== (ezColor, ezColor)")
    {
      EZ_TEST_BOOL(op1 == op1);
      for (int i = 0; i < 4; ++i)
      {
        EZ_TEST_BOOL( !(op1 == (op1 + ezMath::BasicType<float>::SmallEpsilon() * compArray[i])) );
        EZ_TEST_BOOL( !(op1 == (op1 - ezMath::BasicType<float>::SmallEpsilon() * compArray[i])) );
      }
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator!= (ezColor, ezColor)")
    {
      EZ_TEST_BOOL(!(op1 != op1));
      for (int i = 0; i < 4; ++i)
      {
        EZ_TEST_BOOL(op1 != (op1 + ezMath::BasicType<float>::SmallEpsilon() * compArray[i]));
        EZ_TEST_BOOL(op1 != (op1 - ezMath::BasicType<float>::SmallEpsilon() * compArray[i]));
      }
    }
  }
}