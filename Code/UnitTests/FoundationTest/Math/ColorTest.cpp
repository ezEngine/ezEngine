#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/Mat4.h>

EZ_CREATE_SIMPLE_TEST(Math, Color)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor empty")
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    if (ezMath::SupportsNaN<ezMathTestType>())
    {
      // In debug the default constructor initializes everything with NaN.
      ezColor defCtor;
      EZ_TEST_BOOL(ezMath::IsNaN(defCtor.r) && ezMath::IsNaN(defCtor.g) && ezMath::IsNaN(defCtor.b) && ezMath::IsNaN(defCtor.a));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    float testBlock[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    ezColor* pDefCtor = ::new ((void*)&testBlock[0]) ezColor;
    EZ_TEST_BOOL(pDefCtor->r == 1.0f && pDefCtor->g == 2.0f && pDefCtor->b == 3.0f && pDefCtor->a == 4.0f);
#endif

    // Make sure the class didn't accidentally change in size
    EZ_TEST_BOOL(sizeof(ezColor) == sizeof(float) * 4);
  }
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor components")
  {
    ezColor init3F(0.5f, 0.6f, 0.7f);
    EZ_TEST_BOOL(init3F.r == 0.5f && init3F.g == 0.6f && init3F.b == 0.7f && init3F.a == 1.0f);

    ezColor init4F(0.5f, 0.6f, 0.7f, 0.8f);
    EZ_TEST_BOOL(init4F.r == 0.5f && init4F.g == 0.6f && init4F.b == 0.7f && init4F.a == 0.8f);
  }
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor copy")
  {
    ezColor init4F(0.5f, 0.6f, 0.7f, 0.8f);
    ezColor copy(init4F);
    EZ_TEST_BOOL(copy.r == 0.5f && copy.g == 0.6f && copy.b == 0.7f && copy.a == 0.8f);
  }

  {
    ezColor cornflowerBlue(ezColor(0.39f, 0.58f, 0.93f));

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "Conversion float")
    {
      float* pFloats = cornflowerBlue.GetData();
      EZ_TEST_BOOL(
        pFloats[0] == cornflowerBlue.r && pFloats[1] == cornflowerBlue.g && pFloats[2] == cornflowerBlue.b && pFloats[3] == cornflowerBlue.a);

      const float* pConstFloats = cornflowerBlue.GetData();
      EZ_TEST_BOOL(pConstFloats[0] == cornflowerBlue.r && pConstFloats[1] == cornflowerBlue.g && pConstFloats[2] == cornflowerBlue.b &&
                   pConstFloats[3] == cornflowerBlue.a);
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
    const ezColorGammaUB rgb[] = {ezColorGammaUB(255, 255, 255), ezColorGammaUB(0, 0, 0), ezColorGammaUB(123, 12, 1), ezColorGammaUB(31, 112, 153)};
    const ezVec3 hsv[] = {ezVec3(0, 0, 1), ezVec3(0, 0, 0), ezVec3(5.4f, 0.991f, 0.48f), ezVec3(200.2f, 0.797f, 0.600f)};

    for (int i = 0; i < 4; ++i)
    {
      const ezColor color = rgb[i];
      float hue, sat, val;
      color.GetHSV(hue, sat, val);

      EZ_TEST_FLOAT(hue, hsv[i].x, 0.1f);
      EZ_TEST_FLOAT(sat, hsv[i].y, 0.1f);
      EZ_TEST_FLOAT(val, hsv[i].z, 0.1f);

      ezColor fromHSV = ezColor::MakeHSV(hsv[i].x, hsv[i].y, hsv[i].z);
      EZ_TEST_FLOAT(fromHSV.r, color.r, 0.01f);
      EZ_TEST_FLOAT(fromHSV.g, color.g, 0.01f);
      EZ_TEST_FLOAT(fromHSV.b, color.b, 0.01f);
    }
  }

  {
    if (ezMath::SupportsNaN<ezMathTestType>())
    {
      float fNaN = ezMath::NaN<float>();
      const ezColor nanArray[4] = {
        ezColor(fNaN, 0.0f, 0.0f, 0.0f), ezColor(0.0f, fNaN, 0.0f, 0.0f), ezColor(0.0f, 0.0f, fNaN, 0.0f), ezColor(0.0f, 0.0f, 0.0f, fNaN)};
      const ezColor compArray[4] = {
        ezColor(1.0f, 0.0f, 0.0f, 0.0f), ezColor(0.0f, 1.0f, 0.0f, 0.0f), ezColor(0.0f, 0.0f, 1.0f, 0.0f), ezColor(0.0f, 0.0f, 0.0f, 1.0f)};


      EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsNaN")
      {
        for (int i = 0; i < 4; ++i)
        {
          EZ_TEST_BOOL(nanArray[i].IsNaN());
          EZ_TEST_BOOL(!compArray[i].IsNaN());
        }
      }

      EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsValid")
      {
        for (int i = 0; i < 4; ++i)
        {
          EZ_TEST_BOOL(!nanArray[i].IsValid());
          EZ_TEST_BOOL(compArray[i].IsValid());

          EZ_TEST_BOOL(!(compArray[i] * ezMath::Infinity<float>()).IsValid());
          EZ_TEST_BOOL(!(compArray[i] * -ezMath::Infinity<float>()).IsValid());
        }
      }
    }
  }

  {
    const ezColor op1(-4.0, 0.2f, -7.0f, -0.0f);
    const ezColor op2(2.0, 0.3f, 0.0f, 1.0f);
    const ezColor compArray[4] = {
      ezColor(1.0f, 0.0f, 0.0f, 0.0f), ezColor(0.0f, 1.0f, 0.0f, 0.0f), ezColor(0.0f, 0.0f, 1.0f, 0.0f), ezColor(0.0f, 0.0f, 0.0f, 1.0f)};

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
        EZ_TEST_BOOL(!op1.IsIdenticalRGBA(op1 + ezMath::SmallEpsilon<float>() * compArray[i]));
        EZ_TEST_BOOL(!op1.IsIdenticalRGBA(op1 - ezMath::SmallEpsilon<float>() * compArray[i]));
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
        EZ_TEST_BOOL(op1.IsEqualRGBA(op1 + ezMath::SmallEpsilon<float>() * compArray[i], 2 * ezMath::SmallEpsilon<float>()));
        EZ_TEST_BOOL(op1.IsEqualRGBA(op1 - ezMath::SmallEpsilon<float>() * compArray[i], 2 * ezMath::SmallEpsilon<float>()));
        EZ_TEST_BOOL(op1.IsEqualRGBA(op1 + ezMath::DefaultEpsilon<float>() * compArray[i], 2 * ezMath::DefaultEpsilon<float>()));
        EZ_TEST_BOOL(op1.IsEqualRGBA(op1 - ezMath::DefaultEpsilon<float>() * compArray[i], 2 * ezMath::DefaultEpsilon<float>()));
      }
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator+= (ezColor)")
    {
      ezColor plusAssign = op1;
      plusAssign += op2;
      EZ_TEST_BOOL(plusAssign.IsEqualRGBA(ezColor(-2.0f, 0.5f, -7.0f, 1.0f), ezMath::SmallEpsilon<float>()));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator-= (ezColor)")
    {
      ezColor minusAssign = op1;
      minusAssign -= op2;
      EZ_TEST_BOOL(minusAssign.IsEqualRGBA(ezColor(-6.0f, -0.1f, -7.0f, -1.0f), ezMath::SmallEpsilon<float>()));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "ooperator*= (float)")
    {
      ezColor mulFloat = op1;
      mulFloat *= 2.0f;
      EZ_TEST_BOOL(mulFloat.IsEqualRGBA(ezColor(-8.0f, 0.4f, -14.0f, -0.0f), ezMath::SmallEpsilon<float>()));
      mulFloat *= 0.0f;
      EZ_TEST_BOOL(mulFloat.IsEqualRGBA(ezColor(0.0f, 0.0f, 0.0f, 0.0f), ezMath::SmallEpsilon<float>()));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator/= (float)")
    {
      ezColor vDivFloat = op1;
      vDivFloat /= 2.0f;
      EZ_TEST_BOOL(vDivFloat.IsEqualRGBA(ezColor(-2.0f, 0.1f, -3.5f, -0.0f), ezMath::SmallEpsilon<float>()));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator+ (ezColor, ezColor)")
    {
      ezColor plus = (op1 + op2);
      EZ_TEST_BOOL(plus.IsEqualRGBA(ezColor(-2.0f, 0.5f, -7.0f, 1.0f), ezMath::SmallEpsilon<float>()));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator- (ezColor, ezColor)")
    {
      ezColor minus = (op1 - op2);
      EZ_TEST_BOOL(minus.IsEqualRGBA(ezColor(-6.0f, -0.1f, -7.0f, -1.0f), ezMath::SmallEpsilon<float>()));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator* (float, ezColor)")
    {
      ezColor mulFloatVec4 = 2 * op1;
      EZ_TEST_BOOL(mulFloatVec4.IsEqualRGBA(ezColor(-8.0f, 0.4f, -14.0f, -0.0f), ezMath::SmallEpsilon<float>()));
      mulFloatVec4 = ((float)0 * op1);
      EZ_TEST_BOOL(mulFloatVec4.IsEqualRGBA(ezColor(0.0f, 0.0f, 0.0f, 0.0f), ezMath::SmallEpsilon<float>()));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator* (ezColor, float)")
    {
      ezColor mulVec4Float = op1 * 2;
      EZ_TEST_BOOL(mulVec4Float.IsEqualRGBA(ezColor(-8.0f, 0.4f, -14.0f, -0.0f), ezMath::SmallEpsilon<float>()));
      mulVec4Float = (op1 * (float)0);
      EZ_TEST_BOOL(mulVec4Float.IsEqualRGBA(ezColor(0.0f, 0.0f, 0.0f, 0.0f), ezMath::SmallEpsilon<float>()));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator/ (ezColor, float)")
    {
      ezColor vDivVec4Float = op1 / 2;
      EZ_TEST_BOOL(vDivVec4Float.IsEqualRGBA(ezColor(-2.0f, 0.1f, -3.5f, -0.0f), ezMath::SmallEpsilon<float>()));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator== (ezColor, ezColor)")
    {
      EZ_TEST_BOOL(op1 == op1);
      for (int i = 0; i < 4; ++i)
      {
        EZ_TEST_BOOL(!(op1 == (op1 + ezMath::SmallEpsilon<float>() * compArray[i])));
        EZ_TEST_BOOL(!(op1 == (op1 - ezMath::SmallEpsilon<float>() * compArray[i])));
      }
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator< (ezColor, ezColor)")
    {
      for (int i = 0; i < 4; ++i)
      {
        for (int j = 0; j < 4; ++j)
        {
          if (i == j)
          {
            EZ_TEST_BOOL(!(compArray[i] < compArray[j]));
            EZ_TEST_BOOL(!(compArray[j] < compArray[i]));
          }
          else if (i < j)
          {
            EZ_TEST_BOOL(!(compArray[i] < compArray[j]));
            EZ_TEST_BOOL(compArray[j] < compArray[i]);
          }
          else
          {
            EZ_TEST_BOOL(!(compArray[j] < compArray[i]));
            EZ_TEST_BOOL(compArray[i] < compArray[j]);
          }
        }
      }
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator!= (ezColor, ezColor)")
    {
      EZ_TEST_BOOL(!(op1 != op1));
      for (int i = 0; i < 4; ++i)
      {
        EZ_TEST_BOOL(op1 != (op1 + ezMath::SmallEpsilon<float>() * compArray[i]));
        EZ_TEST_BOOL(op1 != (op1 - ezMath::SmallEpsilon<float>() * compArray[i]));
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

      EZ_TEST_FLOAT(c.r, 0.031f, 0.001f);
      EZ_TEST_FLOAT(c.g, 0.127f, 0.001f);
      EZ_TEST_FLOAT(c.b, 0.304f, 0.001f);
      EZ_TEST_FLOAT(c.a, 1.0f, 0.001f);

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
      EZ_TEST_FLOAT(ezColor::Lime.GetSaturation(), 1.0f, 0.001f);
      ;
      EZ_TEST_FLOAT(ezColor::Blue.GetSaturation(), 1.0f, 0.001f);
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator * / *= (ezMat4)")
    {
      ezMat4 m;
      m.SetIdentity();
      m = ezMat4::MakeScaling(ezVec3(0.5f, 0.75f, 0.25f));
      m.SetTranslationVector(ezVec3(0.1f, 0.2f, 0.3f));

      ezColor c1 = m * ezColor::White;

      EZ_TEST_BOOL(c1.IsEqualRGBA(ezColor(0.6f, 0.95f, 0.55f, 1.0f), 0.01f));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "CalcAverageRGB")
    {
      ezColor c1(0.6f, 0.3f, 0.9f, 0.5f);
      EZ_TEST_FLOAT(c1.CalcAverageRGB(), (0.6f + 0.3f + 0.9f) / 3.0f, 0.001f);

      ezColor c2(1.0f, 1.0f, 1.0f, 0.0f);
      EZ_TEST_FLOAT(c2.CalcAverageRGB(), 1.0f, 0.001f);

      ezColor c3(0.0f, 0.0f, 0.0f, 1.0f);
      EZ_TEST_FLOAT(c3.CalcAverageRGB(), 0.0f, 0.001f);
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "ScaleRGB")
    {
      ezColor c1(0.5f, 0.6f, 0.7f, 0.8f);
      c1.ScaleRGB(2.0f);
      EZ_TEST_BOOL(c1.IsEqualRGBA(ezColor(1.0f, 1.2f, 1.4f, 0.8f), 0.001f));

      ezColor c2(0.4f, 0.3f, 0.2f, 0.1f);
      c2.ScaleRGB(0.5f);
      EZ_TEST_BOOL(c2.IsEqualRGBA(ezColor(0.2f, 0.15f, 0.1f, 0.1f), 0.001f));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "ScaleRGBA")
    {
      ezColor c1(0.5f, 0.6f, 0.7f, 0.8f);
      c1.ScaleRGBA(2.0f);
      EZ_TEST_BOOL(c1.IsEqualRGBA(ezColor(1.0f, 1.2f, 1.4f, 1.6f), 0.001f));

      ezColor c2(0.4f, 0.3f, 0.2f, 0.1f);
      c2.ScaleRGBA(0.5f);
      EZ_TEST_BOOL(c2.IsEqualRGBA(ezColor(0.2f, 0.15f, 0.1f, 0.05f), 0.001f));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "ComputeHdrMultiplier")
    {
      // LDR colors should return 1.0
      ezColor ldr1(0.5f, 0.3f, 0.7f, 1.0f);
      EZ_TEST_FLOAT(ldr1.ComputeHdrMultiplier(), 1.0f, 0.001f);

      ezColor ldr2(1.0f, 0.9f, 0.8f, 0.5f);
      EZ_TEST_FLOAT(ldr2.ComputeHdrMultiplier(), 1.0f, 0.001f);

      // HDR colors should return the largest component
      ezColor hdr1(2.0f, 1.5f, 1.0f, 0.5f);
      EZ_TEST_FLOAT(hdr1.ComputeHdrMultiplier(), 2.0f, 0.001f);

      ezColor hdr2(1.0f, 3.5f, 2.2f, 1.0f);
      EZ_TEST_FLOAT(hdr2.ComputeHdrMultiplier(), 3.5f, 0.001f);
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "ComputeHdrExposureValue")
    {
      // LDR colors should return 0
      ezColor ldr(0.5f, 0.3f, 0.7f, 1.0f);
      EZ_TEST_FLOAT(ldr.ComputeHdrExposureValue(), 0.0f, 0.001f);

      // HDR colors should return log2 of the multiplier
      ezColor hdr1(2.0f, 1.0f, 1.0f, 0.5f);
      EZ_TEST_FLOAT(hdr1.ComputeHdrExposureValue(), 1.0f, 0.001f); // log2(2) = 1

      ezColor hdr2(4.0f, 2.0f, 1.0f, 0.5f);
      EZ_TEST_FLOAT(hdr2.ComputeHdrExposureValue(), 2.0f, 0.001f); // log2(4) = 2
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "ApplyHdrExposureValue")
    {
      ezColor c1(0.5f, 0.25f, 0.125f, 1.0f);
      c1.ApplyHdrExposureValue(2.0f); // 2^2 = 4
      EZ_TEST_BOOL(c1.IsEqualRGBA(ezColor(2.0f, 1.0f, 0.5f, 1.0f), 0.001f));

      ezColor c2(1.0f, 0.5f, 0.25f, 0.8f);
      c2.ApplyHdrExposureValue(-1.0f); // 2^-1 = 0.5
      EZ_TEST_BOOL(c2.IsEqualRGBA(ezColor(0.5f, 0.25f, 0.125f, 0.8f), 0.001f));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "NormalizeToLdrRange")
    {
      // HDR color should be normalized
      ezColor hdr(4.0f, 2.0f, 1.0f, 0.5f);
      hdr.NormalizeToLdrRange();
      EZ_TEST_BOOL(hdr.IsEqualRGBA(ezColor(1.0f, 0.5f, 0.25f, 0.5f), 0.001f));

      // LDR color should remain unchanged
      ezColor ldr(0.8f, 0.6f, 0.4f, 1.0f);
      ldr.NormalizeToLdrRange();
      EZ_TEST_BOOL(ldr.IsEqualRGBA(ezColor(0.8f, 0.6f, 0.4f, 1.0f), 0.001f));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetDarker")
    {
      ezColor bright(0.8f, 0.6f, 0.4f, 1.0f);
      ezColor darker = bright.GetDarker(2.0f);

      // Should be darker (lower values) but same alpha
      EZ_TEST_BOOL(darker.r < bright.r && darker.g < bright.g && darker.b < bright.b);
      EZ_TEST_FLOAT(darker.a, bright.a, 0.001f);

      // Test default factor
      ezColor darker2 = bright.GetDarker();
      EZ_TEST_BOOL(darker2.r < bright.r && darker2.g < bright.g && darker2.b < bright.b);
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "WithAlpha")
    {
      ezColor c1(0.5f, 0.6f, 0.7f, 0.8f);
      ezColor c2 = c1.WithAlpha(0.3f);

      EZ_TEST_BOOL(c2.IsEqualRGBA(ezColor(0.5f, 0.6f, 0.7f, 0.3f), 0.001f));
      // Original should be unchanged
      EZ_TEST_BOOL(c1.IsEqualRGBA(ezColor(0.5f, 0.6f, 0.7f, 0.8f), 0.001f));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "ToRGBA8")
    {
      ezColor c1(1.0f, 0.5f, 0.25f, 0.0f);
      ezUInt32 rgba = c1.ToRGBA8();

      // R=255, G=128, B=64, A=0 -> 0xFF804000 (R in MSB, A in LSB)
      EZ_TEST_INT((rgba >> 24) & 0xFF, 255); // R
      EZ_TEST_INT((rgba >> 16) & 0xFF, 128); // G
      EZ_TEST_INT((rgba >> 8) & 0xFF, 64);   // B
      EZ_TEST_INT(rgba & 0xFF, 0);           // A
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "ToABGR8")
    {
      ezColor c1(1.0f, 0.5f, 0.25f, 0.0f);
      ezUInt32 abgr = c1.ToABGR8();

      // A=0, B=64, G=128, R=255 -> 0x004080FF (A in MSB, R in LSB)
      EZ_TEST_INT((abgr >> 24) & 0xFF, 0);  // A
      EZ_TEST_INT((abgr >> 16) & 0xFF, 64); // B
      EZ_TEST_INT((abgr >> 8) & 0xFF, 128); // G
      EZ_TEST_INT(abgr & 0xFF, 255);        // R
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "Static factory functions")
    {
      // MakeNaN
      if (ezMath::SupportsNaN<float>())
      {
        ezColor nanColor = ezColor::MakeNaN();
        EZ_TEST_BOOL(nanColor.IsNaN());
      }

      // MakeZero
      ezColor zeroColor = ezColor::MakeZero();
      EZ_TEST_BOOL(zeroColor.IsEqualRGBA(ezColor(0.0f, 0.0f, 0.0f, 0.0f), 0.001f));

      // MakeRGBA
      ezColor rgbaColor = ezColor::MakeRGBA(0.1f, 0.2f, 0.3f, 0.4f);
      EZ_TEST_BOOL(rgbaColor.IsEqualRGBA(ezColor(0.1f, 0.2f, 0.3f, 0.4f), 0.001f));

      ezColor rgbColor = ezColor::MakeRGBA(0.5f, 0.6f, 0.7f);
      EZ_TEST_BOOL(rgbColor.IsEqualRGBA(ezColor(0.5f, 0.6f, 0.7f, 1.0f), 0.001f));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetAsVec4")
    {
      ezColor c1(0.1f, 0.2f, 0.3f, 0.4f);
      ezVec4 v1 = c1.GetAsVec4();
      EZ_TEST_BOOL(v1.IsEqual(ezVec4(0.1f, 0.2f, 0.3f, 0.4f), 0.001f));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "Gamma/Linear conversion functions")
    {
      // Test single float conversions
      float gamma = 0.5f;
      float linear = ezColor::GammaToLinear(gamma);
      float backToGamma = ezColor::LinearToGamma(linear);
      EZ_TEST_FLOAT(backToGamma, gamma, 0.001f);

      // Test Vec3 conversions
      ezVec3 gammaVec(0.2f, 0.5f, 0.8f);
      ezVec3 linearVec = ezColor::GammaToLinear(gammaVec);
      ezVec3 backToGammaVec = ezColor::LinearToGamma(linearVec);
      EZ_TEST_BOOL(backToGammaVec.IsEqual(gammaVec, 0.001f));

      // Test edge cases
      EZ_TEST_FLOAT(ezColor::GammaToLinear(0.0f), 0.0f, 0.001f);
      EZ_TEST_FLOAT(ezColor::GammaToLinear(1.0f), 1.0f, 0.001f);
      EZ_TEST_FLOAT(ezColor::LinearToGamma(0.0f), 0.0f, 0.001f);
      EZ_TEST_FLOAT(ezColor::LinearToGamma(1.0f), 1.0f, 0.001f);
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*= (ezColor)")
    {
      ezColor c1(0.5f, 0.6f, 0.8f, 1.0f);
      ezColor c2(2.0f, 0.5f, 0.25f, 0.8f);
      c1 *= c2;
      EZ_TEST_BOOL(c1.IsEqualRGBA(ezColor(1.0f, 0.3f, 0.2f, 0.8f), 0.001f));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator* (ezColor, ezColor)")
    {
      ezColor c1(0.5f, 0.6f, 0.8f, 1.0f);
      ezColor c2(2.0f, 0.5f, 0.25f, 0.8f);
      ezColor result = c1 * c2;
      EZ_TEST_BOOL(result.IsEqualRGBA(ezColor(1.0f, 0.3f, 0.2f, 0.8f), 0.001f));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeFromKelvin")
    {
      // Test some known temperature points
      ezColor warm = ezColor::MakeFromKelvin(2700);     // Warm white (incandescent)
      ezColor daylight = ezColor::MakeFromKelvin(6500); // Daylight
      ezColor cool = ezColor::MakeFromKelvin(9000);     // Cool daylight

      // Warm should be more red/orange
      EZ_TEST_BOOL(warm.r > warm.b);

      // Cool should be more blue
      EZ_TEST_BOOL(cool.b > cool.r);

      // Alpha should always be 1
      EZ_TEST_FLOAT(warm.a, 1.0f, 0.001f);
      EZ_TEST_FLOAT(daylight.a, 1.0f, 0.001f);
      EZ_TEST_FLOAT(cool.a, 1.0f, 0.001f);

      // Test reasonable temperature values - all should produce valid colors
      EZ_TEST_BOOL(warm.IsValid());
      EZ_TEST_BOOL(daylight.IsValid());
      EZ_TEST_BOOL(cool.IsValid());
    }
  }
}
