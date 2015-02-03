#include <PCH.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/Mat4.h>

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

  {
    ezColor cornflowerBlue(ezColor(0.39f, 0.58f, 0.93f));

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "Conversion float")
    {
      float* pFloats = cornflowerBlue.GetData();
      EZ_TEST_BOOL(pFloats[0] == cornflowerBlue.r && pFloats[1] == cornflowerBlue.g && pFloats[2] == cornflowerBlue.b && pFloats[3] == cornflowerBlue.a);

      const float* pConstFloats = cornflowerBlue.GetData();
      EZ_TEST_BOOL(pConstFloats[0] == cornflowerBlue.r && pConstFloats[1] == cornflowerBlue.g && pConstFloats[2] == cornflowerBlue.b && pConstFloats[3] == cornflowerBlue.a);
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
      ezColor color(rgb[i].x, rgb[i].y, rgb[i].z);
      float hue, sat, val;
      color.ToLinearHSV(hue, sat, val);

      EZ_TEST_FLOAT(hue, hsv[i].x, 0.1f);
      EZ_TEST_FLOAT(sat, hsv[i].y, 0.1f);
      EZ_TEST_FLOAT(val, hsv[i].z, 0.1f);

      ezColor fromHSV;
      fromHSV.FromLinearHSV(hsv[i].x, hsv[i].y, hsv[i].z);
      EZ_TEST_FLOAT(fromHSV.r, rgb[i].x, 0.01f);
      EZ_TEST_FLOAT(fromHSV.g, rgb[i].y, 0.01f);
      EZ_TEST_FLOAT(fromHSV.b, rgb[i].z, 0.01f);
    }
  }

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

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetRGB / SetRGBA")
    {
      ezColor c1(0, 0, 0, 0);

      c1.SetRGBA(1, 2, 3, 4);

      EZ_TEST_BOOL(c1 == ezColor(1, 2, 3, 4));

      c1.SetRGB(5, 6, 7);

      EZ_TEST_BOOL(c1 == ezColor(5, 6, 7, 4));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsIdenticalRGB")
    {
      ezColor c1(0, 0, 0, 0);
      ezColor c2(0, 0, 0, 1);

      EZ_TEST_BOOL(c1.IsIdenticalRGB(c2));
      EZ_TEST_BOOL(!c1.IsIdenticalRGBA(c2));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsIdenticalRGBA")
    {
      EZ_TEST_BOOL(op1.IsIdenticalRGBA(op1));
      for (int i = 0; i < 4; ++i)
      {
        EZ_TEST_BOOL(!op1.IsIdenticalRGBA(op1 + ezMath::BasicType<float>::SmallEpsilon() * compArray[i]));
        EZ_TEST_BOOL(!op1.IsIdenticalRGBA(op1 - ezMath::BasicType<float>::SmallEpsilon() * compArray[i]));
      }
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsEqualRGB")
    {
      ezColor c1(0, 0, 0, 0);
      ezColor c2(0, 0, 0.2f, 1);

      EZ_TEST_BOOL(!c1.IsEqualRGB(c2, 0.1f));
      EZ_TEST_BOOL(c1.IsEqualRGB(c2, 0.3f));
      EZ_TEST_BOOL(!c1.IsEqualRGBA(c2, 0.3f));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsEqualRGBA")
    {
      EZ_TEST_BOOL(op1.IsEqualRGBA(op1, 0.0f));
      for (int i = 0; i < 4; ++i)
      {
        EZ_TEST_BOOL(op1.IsEqualRGBA(op1 + ezMath::BasicType<float>::SmallEpsilon()   * compArray[i], 2 * ezMath::BasicType<float>::SmallEpsilon()));
        EZ_TEST_BOOL(op1.IsEqualRGBA(op1 - ezMath::BasicType<float>::SmallEpsilon()   * compArray[i], 2 * ezMath::BasicType<float>::SmallEpsilon()));
        EZ_TEST_BOOL(op1.IsEqualRGBA(op1 + ezMath::BasicType<float>::DefaultEpsilon() * compArray[i], 2 * ezMath::BasicType<float>::DefaultEpsilon()));
        EZ_TEST_BOOL(op1.IsEqualRGBA(op1 - ezMath::BasicType<float>::DefaultEpsilon() * compArray[i], 2 * ezMath::BasicType<float>::DefaultEpsilon()));
      }
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator+= (ezColor)")
    {
      ezColor plusAssign = op1;
      plusAssign += op2;
      EZ_TEST_BOOL(plusAssign.IsEqualRGBA(ezColor(-2.0f, 0.5f, -7.0f, 1.0f), ezMath::BasicType<float>::SmallEpsilon()));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator-= (ezColor)")
    {
      ezColor minusAssign = op1;
      minusAssign -= op2;
      EZ_TEST_BOOL(minusAssign.IsEqualRGBA(ezColor(-6.0f, -0.1f, -7.0f, -1.0f), ezMath::BasicType<float>::SmallEpsilon()));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "ooperator*= (float)")
    {
      ezColor mulFloat = op1;
      mulFloat *= 2.0f;
      EZ_TEST_BOOL(mulFloat.IsEqualRGBA(ezColor(-8.0f, 0.4f, -14.0f, -0.0f), ezMath::BasicType<float>::SmallEpsilon()));
      mulFloat *= 0.0f;
      EZ_TEST_BOOL(mulFloat.IsEqualRGBA(ezColor(0.0f, 0.0f, 0.0f, 0.0f), ezMath::BasicType<float>::SmallEpsilon()));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator/= (float)")
    {
      ezColor vDivFloat = op1;
      vDivFloat /= 2.0f;
      EZ_TEST_BOOL(vDivFloat.IsEqualRGBA(ezColor(-2.0f, 0.1f, -3.5f, -0.0f), ezMath::BasicType<float>::SmallEpsilon()));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator+ (ezColor, ezColor)")
    {
      ezColor plus = (op1 + op2);
      EZ_TEST_BOOL(plus.IsEqualRGBA(ezColor(-2.0f, 0.5f, -7.0f, 1.0f), ezMath::BasicType<float>::SmallEpsilon()));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator- (ezColor, ezColor)")
    {
      ezColor minus = (op1 - op2);
      EZ_TEST_BOOL(minus.IsEqualRGBA(ezColor(-6.0f, -0.1f, -7.0f, -1.0f), ezMath::BasicType<float>::SmallEpsilon()));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator* (float, ezColor)")
    {
      ezColor mulFloatVec4 = 2 * op1;
      EZ_TEST_BOOL(mulFloatVec4.IsEqualRGBA(ezColor(-8.0f, 0.4f, -14.0f, -0.0f), ezMath::BasicType<float>::SmallEpsilon()));
      mulFloatVec4 = ((float) 0 * op1);
      EZ_TEST_BOOL(mulFloatVec4.IsEqualRGBA(ezColor(0.0f, 0.0f, 0.0f, 0.0f), ezMath::BasicType<float>::SmallEpsilon()));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator* (ezColor, float)")
    {
      ezColor mulVec4Float = op1 * 2;
      EZ_TEST_BOOL(mulVec4Float.IsEqualRGBA(ezColor(-8.0f, 0.4f, -14.0f, -0.0f), ezMath::BasicType<float>::SmallEpsilon()));
      mulVec4Float = (op1 * (float) 0);
      EZ_TEST_BOOL(mulVec4Float.IsEqualRGBA(ezColor(0.0f, 0.0f, 0.0f, 0.0f), ezMath::BasicType<float>::SmallEpsilon()));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator/ (ezColor, float)")
    {
      ezColor vDivVec4Float = op1 / 2;
      EZ_TEST_BOOL(vDivVec4Float.IsEqualRGBA(ezColor(-2.0f, 0.1f, -3.5f, -0.0f), ezMath::BasicType<float>::SmallEpsilon()));
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

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator= (ezColorLinearUB)")
    {
      ezColor c;
      ezColorLinearUB lin(50, 100, 150, 255);
 
      c = lin;

      EZ_TEST_FLOAT(c.r, 50 / 255.0f, 0.001f);
      EZ_TEST_FLOAT(c.g, 100 / 255.0f, 0.001f);
      EZ_TEST_FLOAT(c.b, 150 / 255.0f, 0.001f);
      EZ_TEST_FLOAT(c.a, 1.0f, 0.001f);
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator= (ezColorGammaUB) / constructor(ezColorGammaUB)")
    {
      ezColor c;
      ezColorGammaUB gamma(50, 100, 150, 255);
 
      c = gamma;
      ezColor c3 = gamma;

      EZ_TEST_BOOL(c == c3);

      EZ_TEST_BOOL(c.r != 50 / 255.0f);
      EZ_TEST_BOOL(c.g != 100 / 255.0f);
      EZ_TEST_BOOL(c.b != 150 / 255.0f);
      EZ_TEST_BOOL(c.a == 1.0f);

      ezColorGammaUB c2 = c;

      EZ_TEST_INT(c2.r, 50);
      EZ_TEST_INT(c2.g, 100);
      EZ_TEST_INT(c2.b, 150);
      EZ_TEST_INT(c2.a, 255);
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetInvertedColor")
    {
      const ezColor c1(0.1f, 0.3f, 0.7f, 0.9f);

      ezColor c2 = c1.GetInvertedColor();

      EZ_TEST_BOOL(c2.IsEqualRGBA(ezColor(0.9f, 0.7f, 0.3f, 0.1f), 0.01f));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetLuminance")
    {
      EZ_TEST_FLOAT(ezColor::Black.GetLuminance(), 0.0f, 0.001f);
      EZ_TEST_FLOAT(ezColor::White.GetLuminance(), 1.0f, 0.001f);

      EZ_TEST_FLOAT(ezColor(0.5f, 0.5f, 0.5f).GetLuminance(), 0.2126f * 0.5f + 0.7152f * 0.5f + 0.0722f * 0.5f, 0.001f);
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetComplementaryColor")
    {
      // black and white have no complementary colors, or rather, they are their own complementary colors, apparently
      EZ_TEST_BOOL(ezColor::Black.GetComplementaryColor().IsEqualRGBA(ezColor::Black, 0.001f));
      EZ_TEST_BOOL(ezColor::White.GetComplementaryColor().IsEqualRGBA(ezColor::White, 0.001f));

      EZ_TEST_BOOL(ezColor::Red.GetComplementaryColor().IsEqualRGBA(ezColor::Cyan, 0.001f));
      EZ_TEST_BOOL(ezColor::Lime.GetComplementaryColor().IsEqualRGBA(ezColor::Magenta, 0.001f));
      EZ_TEST_BOOL(ezColor::Blue.GetComplementaryColor().IsEqualRGBA(ezColor::Yellow, 0.001f));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetSaturation")
    {
      EZ_TEST_FLOAT(ezColor::Black.GetSaturation(), 0.0f, 0.001f);
      EZ_TEST_FLOAT(ezColor::White.GetSaturation(), 0.0f, 0.001f);
      EZ_TEST_FLOAT(ezColor::Red.GetSaturation(), 1.0f, 0.001f);
      EZ_TEST_FLOAT(ezColor::Lime.GetSaturation(), 1.0f, 0.001f);;
      EZ_TEST_FLOAT(ezColor::Blue.GetSaturation(), 1.0f, 0.001f);
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "FromGammaHSV / ToGammaHSV")
    {
      ezColor c1;
      c1.FromGammaHSV(22, 0.54f, 1.0f);

      ezColorGammaUB c2 = c1;
      EZ_TEST_BOOL(c2 == ezColorGammaUB(255, 168, 117));

      float h, s, v;
      c1.ToGammaHSV(h, s, v);

      EZ_TEST_FLOAT(h, 22.0f, 0.01f);
      EZ_TEST_FLOAT(s, 0.54f, 0.01f);
      EZ_TEST_FLOAT(v, 1.0f, 0.01f);
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator * / *= (ezMat4)")
    {
      ezMat4 m;
      m.SetIdentity();
      m.SetScalingMatrix(ezVec3(0.5f, 0.75f, 0.25f));
      m.SetTranslationVector(ezVec3(0.1f, 0.2f, 0.3f));

      ezColor c1 = m * ezColor::White;

      EZ_TEST_BOOL(c1.IsEqualRGBA(ezColor(0.6f, 0.95f, 0.55f, 1.0f), 0.01f));
    }
  }
}