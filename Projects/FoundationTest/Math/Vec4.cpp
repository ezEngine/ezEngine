#include <TestFramework/Framework/TestFramework.h>
#include <Foundation/Math/Vec4.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec2.h>

EZ_CREATE_SIMPLE_TEST(Math, Vec4)
{
  EZ_TEST_BLOCK(true, "Constructor")
  {
#if EZ_COMPILE_FOR_DEBUG
    // In debug the default constructor initializes everything with NaN.
    ezVec4 vDefCtor;
    EZ_TEST(ezMath::IsNaN(vDefCtor.x) && ezMath::IsNaN(vDefCtor.y) && ezMath::IsNaN(vDefCtor.z) && ezMath::IsNaN(vDefCtor.w));
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    float testBlock[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    ezVec4* pDefCtor = ::new ((void*)&testBlock[0]) ezVec4;
    EZ_TEST(pDefCtor->x == 1.0f && pDefCtor->y == 2.0f && pDefCtor->z == 3.0f && pDefCtor->w == 4.0f);
#endif

    // Make sure the class didn't accidentally change in size.
    EZ_TEST(sizeof(ezVec4) == 16);

    ezVec4 vInit1F(2.0f);
    EZ_TEST(vInit1F.x == 2.0f && vInit1F.y == 2.0f && vInit1F.z == 2.0f && vInit1F.w == 2.0f);

    ezVec4 vInit4F(1.0f, 2.0f, 3.0f, 4.0f);
    EZ_TEST(vInit4F.x == 1.0f && vInit4F.y == 2.0f && vInit4F.z == 3.0f && vInit4F.w == 4.0f);

    ezVec4 vCopy(vInit4F);
    EZ_TEST(vCopy.x == 1.0f && vCopy.y == 2.0f && vCopy.z == 3.0f && vCopy.w == 4.0f);

    ezVec4 vZero = ezVec4::ZeroVector();
    EZ_TEST(vZero.x == 0.0f && vZero.y == 0.0f && vZero.z == 0.0f && vZero.w == 0.0f);
  }

  EZ_TEST_BLOCK(true, "Conversion")
  {
    ezVec4 vData(1.0f, 2.0f, 3.0f, 4.0f);
    ezVec2 vToVec2 = vData.GetAsVec2();
    EZ_TEST(vToVec2.x == vData.x && vToVec2.y == vData.y);

    ezVec3 vToVec3 = vData.GetAsVec3();
    EZ_TEST(vToVec3.x == vData.x && vToVec3.y == vData.y && vToVec3.z == vData.z);
  }

  EZ_TEST_BLOCK(true, "Setter")
  {
    ezVec4 vSet1F;
    vSet1F.Set(2.0f);
    EZ_TEST(vSet1F.x == 2.0f && vSet1F.y == 2.0f && vSet1F.z == 2.0f && vSet1F.w == 2.0f);

    ezVec4 vSet4F;
    vSet4F.Set(1.0f, 2.0f, 3.0f, 4.0f);
    EZ_TEST(vSet4F.x == 1.0f && vSet4F.y == 2.0f && vSet4F.z == 3.0f && vSet4F.w == 4.0f);

    ezVec4 vSetZero;
    vSetZero.SetZero();
    EZ_TEST(vSetZero.x == 0.0f && vSetZero.y == 0.0f && vSetZero.z == 0.0f && vSetZero.w == 0.0f);
  }

  EZ_TEST_BLOCK(true, "Length")
  {
    const ezVec4 vOp1(-4.0, 4.0f, -2.0f, -0.0f);
    const ezVec4 compArray[4] = { ezVec4(1.0f, 0.0f, 0.0f, 0.0f),
                                  ezVec4(0.0f, 1.0f, 0.0f, 0.0f),
                                  ezVec4(0.0f, 0.0f, 1.0f, 0.0f),
                                  ezVec4(0.0f, 0.0f, 0.0f, 1.0f) };

    // GetLength
    EZ_TEST_FLOAT(vOp1.GetLength(), 6.0f, ezMath_SmallEpsilon);

    // GetLengthSquared
    EZ_TEST_FLOAT(vOp1.GetLengthSquared(), 36.0f, ezMath_SmallEpsilon);

    // GetLengthAndNormalize
    ezVec4 vLengthAndNorm = vOp1;
    float fLength = vLengthAndNorm.GetLengthAndNormalize();
    EZ_TEST_FLOAT(vLengthAndNorm.GetLength(), 1.0f, ezMath_SmallEpsilon);
    EZ_TEST_FLOAT(fLength, 6.0f, ezMath_SmallEpsilon);
    EZ_TEST_FLOAT(vLengthAndNorm.x * vLengthAndNorm.x + vLengthAndNorm.y * vLengthAndNorm.y
                + vLengthAndNorm.z * vLengthAndNorm.z + vLengthAndNorm.w * vLengthAndNorm.w, 1.0f, ezMath_SmallEpsilon);
    EZ_TEST(vLengthAndNorm.IsNormalized(ezMath_SmallEpsilon));

    // GetNormalized
    ezVec4 vGetNorm = vOp1.GetNormalized();
    EZ_TEST_FLOAT(vGetNorm.x * vGetNorm.x + vGetNorm.y * vGetNorm.y + vGetNorm.z * vGetNorm.z + vGetNorm.w * vGetNorm.w, 1.0f, ezMath_SmallEpsilon);
    EZ_TEST(vGetNorm.IsNormalized(ezMath_SmallEpsilon));

    // Normalize
    ezVec4 vNorm = vOp1;
    vNorm.Normalize();
    EZ_TEST_FLOAT(vNorm.x * vNorm.x + vNorm.y * vNorm.y + vNorm.z * vNorm.z + vNorm.w * vNorm.w, 1.0f, ezMath_SmallEpsilon);
    EZ_TEST(vNorm.IsNormalized(ezMath_SmallEpsilon));

    // NormalizeIfNotZero
    ezVec4 vNormCond = vNorm * ezMath_DefaultEpsilon;
    EZ_TEST(vNormCond.NormalizeIfNotZero(vOp1, ezMath_LargeEpsilon) == EZ_FAILURE);
    EZ_TEST(vNormCond == vOp1);
    vNormCond = vNorm * ezMath_DefaultEpsilon;
    EZ_TEST(vNormCond.NormalizeIfNotZero(vOp1, ezMath_SmallEpsilon) == EZ_SUCCESS);
    EZ_TEST(vNormCond == vNorm);

    // IsZero
    EZ_TEST(ezVec4::ZeroVector().IsZero());
    for (int i = 0; i < 4; ++i)
    {
      EZ_TEST(!compArray[i].IsZero());
    }

    // IsZero(float)
    EZ_TEST(ezVec4::ZeroVector().IsZero(0.0f));
    for (int i = 0; i < 4; ++i)
    {
      EZ_TEST(!compArray[i].IsZero(0.0f));
      EZ_TEST(compArray[i].IsZero(1.0f));
      EZ_TEST((-compArray[i]).IsZero(1.0f));
    }

    // IsNormalized (already tested above)
    for (int i = 0; i < 4; ++i)
    {
      EZ_TEST(compArray[i].IsNormalized());
      EZ_TEST((-compArray[i]).IsNormalized());
      EZ_TEST((compArray[i] * 2.0f).IsNormalized(4.0f));
      EZ_TEST((compArray[i] * 2.0f).IsNormalized(4.0f));
    }

    float fNaN = ezMath::NaN();
    const ezVec4 nanArray[4] = {
      ezVec4(fNaN, 0.0f, 0.0f, 0.0f),
      ezVec4(0.0f, fNaN, 0.0f, 0.0f),
      ezVec4(0.0f, 0.0f, fNaN, 0.0f),
      ezVec4(0.0f, 0.0f, 0.0f, fNaN) };

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
      EZ_TEST(!(compArray[i] * ezMath::Infinity()).IsValid());
      EZ_TEST(!(compArray[i] * -ezMath::Infinity()).IsValid());
    }
  }

  EZ_TEST_BLOCK(true, "Operators")
  {
    const ezVec4 vOp1(-4.0, 0.2f, -7.0f, -0.0f);
    const ezVec4 vOp2( 2.0, 0.3f,  0.0f,  1.0f);
    const ezVec4 compArray[4] = { ezVec4(1.0f, 0.0f, 0.0f, 0.0f),
                                  ezVec4(0.0f, 1.0f, 0.0f, 0.0f),
                                  ezVec4(0.0f, 0.0f, 1.0f, 0.0f),
                                  ezVec4(0.0f, 0.0f, 0.0f, 1.0f) };
    // IsIdentical
    EZ_TEST(vOp1.IsIdentical(vOp1));
    for (int i = 0; i < 4; ++i)
    {
      EZ_TEST(!vOp1.IsIdentical(vOp1 + ezMath_SmallEpsilon * compArray[i]));
      EZ_TEST(!vOp1.IsIdentical(vOp1 - ezMath_SmallEpsilon * compArray[i]));
    }

    // IsEqual
    EZ_TEST(vOp1.IsEqual(vOp1, 0.0f));
    for (int i = 0; i < 4; ++i)
    {
      EZ_TEST(vOp1.IsEqual(vOp1 + ezMath_SmallEpsilon * compArray[i], ezMath_SmallEpsilon));
      EZ_TEST(vOp1.IsEqual(vOp1 - ezMath_SmallEpsilon * compArray[i], ezMath_SmallEpsilon));
      EZ_TEST(vOp1.IsEqual(vOp1 + ezMath_DefaultEpsilon * compArray[i], ezMath_DefaultEpsilon));
      EZ_TEST(vOp1.IsEqual(vOp1 - ezMath_DefaultEpsilon * compArray[i], ezMath_DefaultEpsilon));
    }

    // operator-
    ezVec4 vNegated = -vOp1;
    EZ_TEST(vOp1.x == -vNegated.x && vOp1.y == -vNegated.y && vOp1.z == -vNegated.z && vOp1.w == -vNegated.w);
    
    // operator+= (ezVec4)
    ezVec4 vPlusAssign = vOp1;
    vPlusAssign += vOp2;
    EZ_TEST(vPlusAssign.IsEqual(ezVec4(-2.0f, 0.5f, -7.0f, 1.0f), ezMath_SmallEpsilon));

    // operator-= (ezVec4)
    ezVec4 vMinusAssign = vOp1;
    vMinusAssign -= vOp2;
    EZ_TEST(vMinusAssign.IsEqual(ezVec4(-6.0f, -0.1f, -7.0f, -1.0f), ezMath_SmallEpsilon));

    // operator*= (float)
    ezVec4 vMulFloat = vOp1;
    vMulFloat *= 2.0f;
    EZ_TEST(vMulFloat.IsEqual(ezVec4(-8.0f, 0.4f, -14.0f, -0.0f), ezMath_SmallEpsilon));
    vMulFloat *= 0.0f;
    EZ_TEST(vMulFloat.IsEqual(ezVec4::ZeroVector(), ezMath_SmallEpsilon));

    // operator/= (float)
    ezVec4 vDivFloat = vOp1;
    vDivFloat /= 2.0f;
    EZ_TEST(vDivFloat.IsEqual(ezVec4(-2.0f, 0.1f, -3.5f, -0.0f), ezMath_SmallEpsilon));

    //operator+ (ezVec4, ezVec4)
    ezVec4 vPlus = (vOp1 + vOp2);
    EZ_TEST(vPlus.IsEqual(ezVec4(-2.0f, 0.5f, -7.0f, 1.0f), ezMath_SmallEpsilon));

    //operator- (ezVec4, ezVec4)
    ezVec4 vMinus = (vOp1 - vOp2);
    EZ_TEST(vMinus.IsEqual(ezVec4(-6.0f, -0.1f, -7.0f, -1.0f), ezMath_SmallEpsilon));

    // operator* (float, ezVec4)
    ezVec4 vMulFloatVec4 = (2.0f * vOp1);
    EZ_TEST(vMulFloatVec4.IsEqual(ezVec4(-8.0f, 0.4f, -14.0f, -0.0f), ezMath_SmallEpsilon));
    vMulFloatVec4 = (0.0f * vOp1);
    EZ_TEST(vMulFloatVec4.IsEqual(ezVec4::ZeroVector(), ezMath_SmallEpsilon));

    // operator* (ezVec4, float)
    ezVec4 vMulVec4Float = (vOp1 * 2.0f);
    EZ_TEST(vMulVec4Float.IsEqual(ezVec4(-8.0f, 0.4f, -14.0f, -0.0f), ezMath_SmallEpsilon));
    vMulVec4Float = (vOp1 * 0.0f);
    EZ_TEST(vMulVec4Float.IsEqual(ezVec4::ZeroVector(), ezMath_SmallEpsilon));

    // operator/ (ezVec4, float)
    ezVec4 vDivVec4Float = (vOp1 / 2.0f);
    EZ_TEST(vDivFloat.IsEqual(ezVec4(-2.0f, 0.1f, -3.5f, -0.0f), ezMath_SmallEpsilon));

    // operator== (ezVec4, ezVec4)
    EZ_TEST(vOp1 == vOp1);
    for (int i = 0; i < 4; ++i)
    {
      EZ_TEST( !(vOp1 == (vOp1 + ezMath_SmallEpsilon * compArray[i])) );
      EZ_TEST( !(vOp1 == (vOp1 - ezMath_SmallEpsilon * compArray[i])) );
    }

    // operator!= (ezVec4, ezVec4)
    EZ_TEST(!(vOp1 != vOp1));
    for (int i = 0; i < 4; ++i)
    {
      EZ_TEST(vOp1 != (vOp1 + ezMath_SmallEpsilon * compArray[i]));
      EZ_TEST(vOp1 != (vOp1 - ezMath_SmallEpsilon * compArray[i]));
    }

    // operator< (ezVec4, ezVec4)
    for (int i = 0; i < 4; ++i)
    {
      for (int j = 0; j < 4; ++j)
      {
        if (i == j)
        {
          EZ_TEST( !(compArray[i] < compArray[j]) );
          EZ_TEST( !(compArray[j] < compArray[i]) );
        }
        else if (i < j)
        {
          EZ_TEST( !(compArray[i] < compArray[j]) );
          EZ_TEST( compArray[j] < compArray[i] );
        }
        else
        {
          EZ_TEST( !(compArray[j] < compArray[i]) );
          EZ_TEST( compArray[i] < compArray[j] );
        }
      }
    }
  }

  EZ_TEST_BLOCK(true, "Common")
  {
    const ezVec4 vOp1(-4.0, 0.2f, -7.0f, -0.0f);
    const ezVec4 vOp2( 2.0, -0.3f, 0.5f,  1.0f);

    // Dot
    EZ_TEST_FLOAT(vOp1.Dot(vOp2), -11.56f, ezMath_SmallEpsilon);
    EZ_TEST_FLOAT(vOp2.Dot(vOp1), -11.56f, ezMath_SmallEpsilon);

    // CompMin
    EZ_TEST(vOp1.CompMin(vOp2).IsEqual(ezVec4(-4.0f, -0.3f, -7.0f, -0.0f), ezMath_SmallEpsilon));
    EZ_TEST(vOp2.CompMin(vOp1).IsEqual(ezVec4(-4.0f, -0.3f, -7.0f, -0.0f), ezMath_SmallEpsilon));

    // CompMax
    EZ_TEST(vOp1.CompMax(vOp2).IsEqual(ezVec4(2.0f, 0.2f, 0.5f, 1.0f), ezMath_SmallEpsilon));
    EZ_TEST(vOp2.CompMax(vOp1).IsEqual(ezVec4(2.0f, 0.2f, 0.5f, 1.0f), ezMath_SmallEpsilon));

    // CompMult
    EZ_TEST(vOp1.CompMult(vOp2).IsEqual(ezVec4(-8.0f, -0.06f, -3.5f, 0.0f), ezMath_SmallEpsilon));
    EZ_TEST(vOp2.CompMult(vOp1).IsEqual(ezVec4(-8.0f, -0.06f, -3.5f, 0.0f), ezMath_SmallEpsilon));

    // CompDiv
    EZ_TEST(vOp1.CompDiv(vOp2).IsEqual(ezVec4(-2.0f, -0.66666666f, -14.0f, 0.0f), ezMath_SmallEpsilon));
  }
}