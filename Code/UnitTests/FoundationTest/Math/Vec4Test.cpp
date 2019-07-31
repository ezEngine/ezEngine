#include <FoundationTestPCH.h>

#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec4.h>


EZ_CREATE_SIMPLE_TEST(Math, Vec4)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    if (ezMath::SupportsNaN<ezMathTestType>())
    {
      // In debug the default constructor initializes everything with NaN.
      ezVec4T vDefCtor;
      EZ_TEST_BOOL(ezMath::IsNaN(vDefCtor.x) && ezMath::IsNaN(vDefCtor.y)/* && ezMath::IsNaN(vDefCtor.z) && ezMath::IsNaN(vDefCtor.w)*/);
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    ezVec4T::ComponentType testBlock[4] = {(ezVec4T::ComponentType)1, (ezVec4T::ComponentType)2, (ezVec4T::ComponentType)3,
                                           (ezVec4T::ComponentType)4};
    ezVec4T* pDefCtor = ::new ((void*)&testBlock[0]) ezVec4T;
    EZ_TEST_BOOL(pDefCtor->x == (ezVec4T::ComponentType)1 && pDefCtor->y == (ezVec4T::ComponentType)2 &&
                 pDefCtor->z == (ezVec4T::ComponentType)3 && pDefCtor->w == (ezVec4T::ComponentType)4);
#endif

    // Make sure the class didn't accidentally change in size.
    EZ_TEST_BOOL(sizeof(ezVec4) == 16);
    EZ_TEST_BOOL(sizeof(ezVec4d) == 32);

    ezVec4T vInit1F(2.0f);
    EZ_TEST_BOOL(vInit1F.x == 2.0f && vInit1F.y == 2.0f && vInit1F.z == 2.0f && vInit1F.w == 2.0f);

    ezVec4T vInit4F(1.0f, 2.0f, 3.0f, 4.0f);
    EZ_TEST_BOOL(vInit4F.x == 1.0f && vInit4F.y == 2.0f && vInit4F.z == 3.0f && vInit4F.w == 4.0f);

    ezVec4T vCopy(vInit4F);
    EZ_TEST_BOOL(vCopy.x == 1.0f && vCopy.y == 2.0f && vCopy.z == 3.0f && vCopy.w == 4.0f);

    ezVec4T vZero = ezVec4T::ZeroVector();
    EZ_TEST_BOOL(vZero.x == 0.0f && vZero.y == 0.0f && vZero.z == 0.0f && vZero.w == 0.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Conversion")
  {
    ezVec4T vData(1.0f, 2.0f, 3.0f, 4.0f);
    ezVec2T vToVec2 = vData.GetAsVec2();
    EZ_TEST_BOOL(vToVec2.x == vData.x && vToVec2.y == vData.y);

    ezVec3T vToVec3 = vData.GetAsVec3();
    EZ_TEST_BOOL(vToVec3.x == vData.x && vToVec3.y == vData.y && vToVec3.z == vData.z);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Setter")
  {
    ezVec4T vSet1F;
    vSet1F.Set(2.0f);
    EZ_TEST_BOOL(vSet1F.x == 2.0f && vSet1F.y == 2.0f && vSet1F.z == 2.0f && vSet1F.w == 2.0f);

    ezVec4T vSet4F;
    vSet4F.Set(1.0f, 2.0f, 3.0f, 4.0f);
    EZ_TEST_BOOL(vSet4F.x == 1.0f && vSet4F.y == 2.0f && vSet4F.z == 3.0f && vSet4F.w == 4.0f);

    ezVec4T vSetZero;
    vSetZero.SetZero();
    EZ_TEST_BOOL(vSetZero.x == 0.0f && vSetZero.y == 0.0f && vSetZero.z == 0.0f && vSetZero.w == 0.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Length")
  {
    const ezVec4T vOp1(-4.0, 4.0f, -2.0f, -0.0f);
    const ezVec4T compArray[4] = {ezVec4T(1.0f, 0.0f, 0.0f, 0.0f), ezVec4T(0.0f, 1.0f, 0.0f, 0.0f), ezVec4T(0.0f, 0.0f, 1.0f, 0.0f),
                                  ezVec4T(0.0f, 0.0f, 0.0f, 1.0f)};

    // GetLength
    EZ_TEST_FLOAT(vOp1.GetLength(), 6.0f, ezMath::SmallEpsilon<ezMathTestType>());

    // GetLengthSquared
    EZ_TEST_FLOAT(vOp1.GetLengthSquared(), 36.0f, ezMath::SmallEpsilon<ezMathTestType>());

    // GetLengthAndNormalize
    ezVec4T vLengthAndNorm = vOp1;
    ezMathTestType fLength = vLengthAndNorm.GetLengthAndNormalize();
    EZ_TEST_FLOAT(vLengthAndNorm.GetLength(), 1.0f, ezMath::SmallEpsilon<ezMathTestType>());
    EZ_TEST_FLOAT(fLength, 6.0f, ezMath::SmallEpsilon<ezMathTestType>());
    EZ_TEST_FLOAT(vLengthAndNorm.x * vLengthAndNorm.x + vLengthAndNorm.y * vLengthAndNorm.y + vLengthAndNorm.z * vLengthAndNorm.z +
                      vLengthAndNorm.w * vLengthAndNorm.w,
                  1.0f, ezMath::SmallEpsilon<ezMathTestType>());
    EZ_TEST_BOOL(vLengthAndNorm.IsNormalized(ezMath::SmallEpsilon<ezMathTestType>()));

    // GetNormalized
    ezVec4T vGetNorm = vOp1.GetNormalized();
    EZ_TEST_FLOAT(vGetNorm.x * vGetNorm.x + vGetNorm.y * vGetNorm.y + vGetNorm.z * vGetNorm.z + vGetNorm.w * vGetNorm.w, 1.0f,
                  ezMath::SmallEpsilon<ezMathTestType>());
    EZ_TEST_BOOL(vGetNorm.IsNormalized(ezMath::SmallEpsilon<ezMathTestType>()));

    // Normalize
    ezVec4T vNorm = vOp1;
    vNorm.Normalize();
    EZ_TEST_FLOAT(vNorm.x * vNorm.x + vNorm.y * vNorm.y + vNorm.z * vNorm.z + vNorm.w * vNorm.w, 1.0f,
                  ezMath::SmallEpsilon<ezMathTestType>());
    EZ_TEST_BOOL(vNorm.IsNormalized(ezMath::SmallEpsilon<ezMathTestType>()));

    // NormalizeIfNotZero
    ezVec4T vNormCond = vNorm * ezMath::DefaultEpsilon<ezMathTestType>();
    EZ_TEST_BOOL(vNormCond.NormalizeIfNotZero(vOp1, ezMath::LargeEpsilon<ezMathTestType>()) == EZ_FAILURE);
    EZ_TEST_BOOL(vNormCond == vOp1);
    vNormCond = vNorm * ezMath::DefaultEpsilon<ezMathTestType>();
    EZ_TEST_BOOL(vNormCond.NormalizeIfNotZero(vOp1, ezMath::SmallEpsilon<ezMathTestType>()) == EZ_SUCCESS);
    EZ_TEST_VEC4(vNormCond, vNorm, ezMath::DefaultEpsilon<ezVec4T::ComponentType>());

    // IsZero
    EZ_TEST_BOOL(ezVec4T::ZeroVector().IsZero());
    for (int i = 0; i < 4; ++i)
    {
      EZ_TEST_BOOL(!compArray[i].IsZero());
    }

    // IsZero(float)
    EZ_TEST_BOOL(ezVec4T::ZeroVector().IsZero(0.0f));
    for (int i = 0; i < 4; ++i)
    {
      EZ_TEST_BOOL(!compArray[i].IsZero(0.0f));
      EZ_TEST_BOOL(compArray[i].IsZero(1.0f));
      EZ_TEST_BOOL((-compArray[i]).IsZero(1.0f));
    }

    // IsNormalized (already tested above)
    for (int i = 0; i < 4; ++i)
    {
      EZ_TEST_BOOL(compArray[i].IsNormalized());
      EZ_TEST_BOOL((-compArray[i]).IsNormalized());
      EZ_TEST_BOOL((compArray[i] * (ezMathTestType)2).IsNormalized((ezMathTestType)4));
      EZ_TEST_BOOL((compArray[i] * (ezMathTestType)2).IsNormalized((ezMathTestType)4));
    }

    if (ezMath::SupportsNaN<ezMathTestType>())
    {
      ezMathTestType TypeNaN = ezMath::NaN<ezMathTestType>();
      const ezVec4T nanArray[4] = {ezVec4T(TypeNaN, 0.0f, 0.0f, 0.0f), ezVec4T(0.0f, TypeNaN, 0.0f, 0.0f),
                                   ezVec4T(0.0f, 0.0f, TypeNaN, 0.0f), ezVec4T(0.0f, 0.0f, 0.0f, TypeNaN)};

      // IsNaN
      for (int i = 0; i < 4; ++i)
      {
        EZ_TEST_BOOL(nanArray[i].IsNaN());
        EZ_TEST_BOOL(!compArray[i].IsNaN());
      }

      // IsValid
      for (int i = 0; i < 4; ++i)
      {
        EZ_TEST_BOOL(!nanArray[i].IsValid());
        EZ_TEST_BOOL(compArray[i].IsValid());

        EZ_TEST_BOOL(!(compArray[i] * ezMath::Infinity<ezMathTestType>()).IsValid());
        EZ_TEST_BOOL(!(compArray[i] * -ezMath::Infinity<ezMathTestType>()).IsValid());
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Operators")
  {
    const ezVec4T vOp1(-4.0, 0.2f, -7.0f, -0.0f);
    const ezVec4T vOp2(2.0, 0.3f, 0.0f, 1.0f);
    const ezVec4T compArray[4] = {ezVec4T(1.0f, 0.0f, 0.0f, 0.0f), ezVec4T(0.0f, 1.0f, 0.0f, 0.0f), ezVec4T(0.0f, 0.0f, 1.0f, 0.0f),
                                  ezVec4T(0.0f, 0.0f, 0.0f, 1.0f)};
    // IsIdentical
    EZ_TEST_BOOL(vOp1.IsIdentical(vOp1));
    for (int i = 0; i < 4; ++i)
    {
      EZ_TEST_BOOL(!vOp1.IsIdentical(vOp1 + (ezMathTestType)ezMath::SmallEpsilon<ezMathTestType>() * compArray[i]));
      EZ_TEST_BOOL(!vOp1.IsIdentical(vOp1 - (ezMathTestType)ezMath::SmallEpsilon<ezMathTestType>() * compArray[i]));
    }

    // IsEqual
    EZ_TEST_BOOL(vOp1.IsEqual(vOp1, 0.0f));
    for (int i = 0; i < 4; ++i)
    {
      EZ_TEST_BOOL(vOp1.IsEqual(vOp1 + ezMath::SmallEpsilon<ezMathTestType>() * compArray[i],
                                2 * ezMath::SmallEpsilon<ezMathTestType>()));
      EZ_TEST_BOOL(vOp1.IsEqual(vOp1 - ezMath::SmallEpsilon<ezMathTestType>() * compArray[i],
                                2 * ezMath::SmallEpsilon<ezMathTestType>()));
      EZ_TEST_BOOL(vOp1.IsEqual(vOp1 + ezMath::DefaultEpsilon<ezMathTestType>() * compArray[i],
                                2 * ezMath::DefaultEpsilon<ezMathTestType>()));
      EZ_TEST_BOOL(vOp1.IsEqual(vOp1 - ezMath::DefaultEpsilon<ezMathTestType>() * compArray[i],
                                2 * ezMath::DefaultEpsilon<ezMathTestType>()));
    }

    // operator-
    ezVec4T vNegated = -vOp1;
    EZ_TEST_BOOL(vOp1.x == -vNegated.x && vOp1.y == -vNegated.y && vOp1.z == -vNegated.z && vOp1.w == -vNegated.w);

    // operator+= (ezVec4T)
    ezVec4T vPlusAssign = vOp1;
    vPlusAssign += vOp2;
    EZ_TEST_BOOL(vPlusAssign.IsEqual(ezVec4T(-2.0f, 0.5f, -7.0f, 1.0f), ezMath::SmallEpsilon<ezMathTestType>()));

    // operator-= (ezVec4T)
    ezVec4T vMinusAssign = vOp1;
    vMinusAssign -= vOp2;
    EZ_TEST_BOOL(vMinusAssign.IsEqual(ezVec4T(-6.0f, -0.1f, -7.0f, -1.0f), ezMath::SmallEpsilon<ezMathTestType>()));

    // operator*= (float)
    ezVec4T vMulFloat = vOp1;
    vMulFloat *= 2.0f;
    EZ_TEST_BOOL(vMulFloat.IsEqual(ezVec4T(-8.0f, 0.4f, -14.0f, -0.0f), ezMath::SmallEpsilon<ezMathTestType>()));
    vMulFloat *= 0.0f;
    EZ_TEST_BOOL(vMulFloat.IsEqual(ezVec4T::ZeroVector(), ezMath::SmallEpsilon<ezMathTestType>()));

    // operator/= (float)
    ezVec4T vDivFloat = vOp1;
    vDivFloat /= 2.0f;
    EZ_TEST_BOOL(vDivFloat.IsEqual(ezVec4T(-2.0f, 0.1f, -3.5f, -0.0f), ezMath::SmallEpsilon<ezMathTestType>()));

    // operator+ (ezVec4T, ezVec4T)
    ezVec4T vPlus = (vOp1 + vOp2);
    EZ_TEST_BOOL(vPlus.IsEqual(ezVec4T(-2.0f, 0.5f, -7.0f, 1.0f), ezMath::SmallEpsilon<ezMathTestType>()));

    // operator- (ezVec4T, ezVec4T)
    ezVec4T vMinus = (vOp1 - vOp2);
    EZ_TEST_BOOL(vMinus.IsEqual(ezVec4T(-6.0f, -0.1f, -7.0f, -1.0f), ezMath::SmallEpsilon<ezMathTestType>()));

    // operator* (float, ezVec4T)
    ezVec4T vMulFloatVec4 = ((ezMathTestType)2 * vOp1);
    EZ_TEST_BOOL(vMulFloatVec4.IsEqual(ezVec4T(-8.0f, 0.4f, -14.0f, -0.0f), ezMath::SmallEpsilon<ezMathTestType>()));
    vMulFloatVec4 = ((ezMathTestType)0 * vOp1);
    EZ_TEST_BOOL(vMulFloatVec4.IsEqual(ezVec4T::ZeroVector(), ezMath::SmallEpsilon<ezMathTestType>()));

    // operator* (ezVec4T, float)
    ezVec4T vMulVec4Float = (vOp1 * (ezMathTestType)2);
    EZ_TEST_BOOL(vMulVec4Float.IsEqual(ezVec4T(-8.0f, 0.4f, -14.0f, -0.0f), ezMath::SmallEpsilon<ezMathTestType>()));
    vMulVec4Float = (vOp1 * (ezMathTestType)0);
    EZ_TEST_BOOL(vMulVec4Float.IsEqual(ezVec4T::ZeroVector(), ezMath::SmallEpsilon<ezMathTestType>()));

    // operator/ (ezVec4T, float)
    ezVec4T vDivVec4Float = (vOp1 / (ezMathTestType)2);
    EZ_TEST_BOOL(vDivVec4Float.IsEqual(ezVec4T(-2.0f, 0.1f, -3.5f, -0.0f), ezMath::SmallEpsilon<ezMathTestType>()));

    // operator== (ezVec4T, ezVec4T)
    EZ_TEST_BOOL(vOp1 == vOp1);
    for (int i = 0; i < 4; ++i)
    {
      EZ_TEST_BOOL(!(vOp1 == (vOp1 + (ezMathTestType)ezMath::SmallEpsilon<ezMathTestType>() * compArray[i])));
      EZ_TEST_BOOL(!(vOp1 == (vOp1 - (ezMathTestType)ezMath::SmallEpsilon<ezMathTestType>() * compArray[i])));
    }

    // operator!= (ezVec4T, ezVec4T)
    EZ_TEST_BOOL(!(vOp1 != vOp1));
    for (int i = 0; i < 4; ++i)
    {
      EZ_TEST_BOOL(vOp1 != (vOp1 + (ezMathTestType)ezMath::SmallEpsilon<ezMathTestType>() * compArray[i]));
      EZ_TEST_BOOL(vOp1 != (vOp1 - (ezMathTestType)ezMath::SmallEpsilon<ezMathTestType>() * compArray[i]));
    }

    // operator< (ezVec4T, ezVec4T)
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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Common")
  {
    const ezVec4T vOp1(-4.0, 0.2f, -7.0f, -0.0f);
    const ezVec4T vOp2(2.0, -0.3f, 0.5f, 1.0f);

    // Dot
    EZ_TEST_FLOAT(vOp1.Dot(vOp2), -11.56f, ezMath::SmallEpsilon<ezMathTestType>());
    EZ_TEST_FLOAT(vOp2.Dot(vOp1), -11.56f, ezMath::SmallEpsilon<ezMathTestType>());

    // CompMin
    EZ_TEST_BOOL(vOp1.CompMin(vOp2).IsEqual(ezVec4T(-4.0f, -0.3f, -7.0f, -0.0f), ezMath::SmallEpsilon<ezMathTestType>()));
    EZ_TEST_BOOL(vOp2.CompMin(vOp1).IsEqual(ezVec4T(-4.0f, -0.3f, -7.0f, -0.0f), ezMath::SmallEpsilon<ezMathTestType>()));

    // CompMax
    EZ_TEST_BOOL(vOp1.CompMax(vOp2).IsEqual(ezVec4T(2.0f, 0.2f, 0.5f, 1.0f), ezMath::SmallEpsilon<ezMathTestType>()));
    EZ_TEST_BOOL(vOp2.CompMax(vOp1).IsEqual(ezVec4T(2.0f, 0.2f, 0.5f, 1.0f), ezMath::SmallEpsilon<ezMathTestType>()));

    // CompClamp
    EZ_TEST_BOOL(vOp1.CompClamp(vOp1, vOp2).IsEqual(ezVec4T(-4.0f, -0.3f, -7.0f, -0.0f), ezMath::SmallEpsilon<ezMathTestType>()));
    EZ_TEST_BOOL(vOp2.CompClamp(vOp1, vOp2).IsEqual(ezVec4T(2.0f, 0.2f, 0.5f, 1.0f), ezMath::SmallEpsilon<ezMathTestType>()));

    // CompMul
    EZ_TEST_BOOL(vOp1.CompMul(vOp2).IsEqual(ezVec4T(-8.0f, -0.06f, -3.5f, 0.0f), ezMath::SmallEpsilon<ezMathTestType>()));
    EZ_TEST_BOOL(vOp2.CompMul(vOp1).IsEqual(ezVec4T(-8.0f, -0.06f, -3.5f, 0.0f), ezMath::SmallEpsilon<ezMathTestType>()));

    // CompDiv
    EZ_TEST_BOOL(vOp1.CompDiv(vOp2).IsEqual(ezVec4T(-2.0f, -0.66666666f, -14.0f, 0.0f), ezMath::SmallEpsilon<ezMathTestType>()));

    // Abs
    EZ_TEST_VEC4(vOp1.Abs(), ezVec4T(4.0, 0.2f, 7.0f, 0.0f), ezMath::SmallEpsilon<ezMathTestType>());
  }
}
