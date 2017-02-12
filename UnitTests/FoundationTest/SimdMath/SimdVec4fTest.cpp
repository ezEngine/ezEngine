#include <PCH.h>
#include <Foundation/SimdMath/SimdVec4f.h>

EZ_CREATE_SIMPLE_TEST(SimdMath, SimdVec4f)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    // In debug the default constructor initializes everything with NaN.
    ezSimdVec4f vDefCtor;
    EZ_TEST_BOOL(vDefCtor.IsNaN<4>());
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    float EZ_ALIGN_16(testBlock[4]) = { 1, 2, 3, 4 };
    ezSimdVec4f* pDefCtor = ::new ((void*)&testBlock[0]) ezSimdVec4f;
    EZ_TEST_BOOL(pDefCtor->x() == 1.0f && pDefCtor->y() == 2.0f &&
      pDefCtor->z() == 3.0f && pDefCtor->w() == 4.0f);
#endif

    // Make sure the class didn't accidentally change in size.
#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
    EZ_CHECK_AT_COMPILETIME(sizeof(ezSimdVec4f) == 16);
    EZ_CHECK_AT_COMPILETIME(EZ_ALIGNMENT_OF(ezSimdVec4f) == 16);
#endif

    ezSimdVec4f vInit1F(2.0f);
    EZ_TEST_BOOL(vInit1F.x() == 2.0f && vInit1F.y() == 2.0f &&
      vInit1F.z() == 2.0f && vInit1F.w() == 2.0f);

    ezSimdFloat a(3.0f);
    ezSimdVec4f vInit1SF(a);
    EZ_TEST_BOOL(vInit1SF.x() == 3.0f && vInit1SF.y() == 3.0f &&
      vInit1SF.z() == 3.0f && vInit1SF.w() == 3.0f);

    ezSimdVec4f vInit4F(1.0f, 2.0f, 3.0f, 4.0f);
    EZ_TEST_BOOL(vInit4F.x() == 1.0f && vInit4F.y() == 2.0f &&
      vInit4F.z() == 3.0f && vInit4F.w() == 4.0f);

    // Make sure all components have the correct values
#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
    EZ_TEST_BOOL(vInit4F.m_v.m128_f32[0] == 1.0f && vInit4F.m_v.m128_f32[1] == 2.0f &&
      vInit4F.m_v.m128_f32[2] == 3.0f && vInit4F.m_v.m128_f32[3] == 4.0f);
#endif

    ezSimdVec4f vCopy(vInit4F);
    EZ_TEST_BOOL(vCopy.x() == 1.0f && vCopy.y() == 2.0f &&
      vCopy.z() == 3.0f && vCopy.w() == 4.0f);

    ezSimdVec4f vZero = ezSimdVec4f::ZeroVector();
    EZ_TEST_BOOL(vZero.x() == 0.0f && vZero.y() == 0.0f &&
      vZero.z() == 0.0f && vZero.w() == 0.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Setter")
  {
    ezSimdVec4f vSet1F;
    vSet1F.Set(2.0f);
    EZ_TEST_BOOL(vSet1F.x() == 2.0f && vSet1F.y() == 2.0f &&
      vSet1F.z() == 2.0f && vSet1F.w() == 2.0f);

    ezSimdVec4f vSet4F;
    vSet4F.Set(1.0f, 2.0f, 3.0f, 4.0f);
    EZ_TEST_BOOL(vSet4F.x() == 1.0f && vSet4F.y() == 2.0f &&
      vSet4F.z() == 3.0f && vSet4F.w() == 4.0f);

    ezSimdVec4f vSetZero;
    vSetZero.SetZero();
    EZ_TEST_BOOL(vSetZero.x() == 0.0f && vSetZero.y() == 0.0f &&
      vSetZero.z() == 0.0f && vSetZero.w() == 0.0f);

    {
      float testBlock[4] = { 1, 2, 3, 4 };
      ezSimdVec4f x; x.Load<1>(testBlock);
      EZ_TEST_BOOL(x.x() == 1.0f && x.y() == 0.0f &&
        x.z() == 0.0f && x.w() == 0.0f);

      ezSimdVec4f xy; xy.Load<2>(testBlock);
      EZ_TEST_BOOL(xy.x() == 1.0f && xy.y() == 2.0f &&
        xy.z() == 0.0f && xy.w() == 0.0f);

      ezSimdVec4f xyz; xyz.Load<3>(testBlock);
      EZ_TEST_BOOL(xyz.x() == 1.0f && xyz.y() == 2.0f &&
        xyz.z() == 3.0f && xyz.w() == 0.0f);

      ezSimdVec4f xyzw; xyzw.Load<4>(testBlock);
      EZ_TEST_BOOL(xyzw.x() == 1.0f && xyzw.y() == 2.0f &&
        xyzw.z() == 3.0f && xyzw.w() == 4.0f);

      // Make sure all components have the correct values
#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
      EZ_TEST_BOOL(xyzw.m_v.m128_f32[0] == 1.0f && xyzw.m_v.m128_f32[1] == 2.0f &&
        xyzw.m_v.m128_f32[2] == 3.0f && xyzw.m_v.m128_f32[3] == 4.0f);
#endif
    }

    {
      float testBlock[4] = {7, 7, 7, 7};
      float mem[4] = {};

      ezSimdVec4f a(1, 2, 3, 4);

      memcpy(mem, testBlock, 16);
      a.Store<1>(mem);
      EZ_TEST_BOOL(mem[0] == 1.0f && mem[1] == 7.0f && mem[2] == 7.0f && mem[3] == 7.0f);

      memcpy(mem, testBlock, 16);
      a.Store<2>(mem);
      EZ_TEST_BOOL(mem[0] == 1.0f && mem[1] == 2.0f && mem[2] == 7.0f && mem[3] == 7.0f);

      memcpy(mem, testBlock, 16);
      a.Store<3>(mem);
      EZ_TEST_BOOL(mem[0] == 1.0f && mem[1] == 2.0f && mem[2] == 3.0f && mem[3] == 7.0f);

      memcpy(mem, testBlock, 16);
      a.Store<4>(mem);
      EZ_TEST_BOOL(mem[0] == 1.0f && mem[1] == 2.0f && mem[2] == 3.0f && mem[3] == 4.0f);
    }
  }

#if 0
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Length")
  {
    const ezSimdVec4f vOp1(-4.0, 4.0f, -2.0f, -0.0f);
    const ezSimdVec4f compArray[4] = { ezSimdVec4f(1.0f, 0.0f, 0.0f, 0.0f),
                                  ezSimdVec4f(0.0f, 1.0f, 0.0f, 0.0f),
                                  ezSimdVec4f(0.0f, 0.0f, 1.0f, 0.0f),
                                  ezSimdVec4f(0.0f, 0.0f, 0.0f, 1.0f) };

    // GetLength
    EZ_TEST_FLOAT(vOp1.GetLength<4>(), 6.0f, ezMath::BasicType<ezMathTestType>::SmallEpsilon());

    // GetLengthSquared
    EZ_TEST_FLOAT(vOp1.GetLengthSquared(), 36.0f, ezMath::BasicType<ezMathTestType>::SmallEpsilon());

    // GetLengthAndNormalize
    ezSimdVec4f vLengthAndNorm = vOp1;
    ezMathTestType fLength = vLengthAndNorm.GetLengthAndNormalize();
    EZ_TEST_FLOAT(vLengthAndNorm.GetLength(), 1.0f, ezMath::BasicType<ezMathTestType>::SmallEpsilon());
    EZ_TEST_FLOAT(fLength, 6.0f, ezMath::BasicType<ezMathTestType>::SmallEpsilon());
    EZ_TEST_FLOAT(vLengthAndNorm.x * vLengthAndNorm.x + vLengthAndNorm.y * vLengthAndNorm.y
                + vLengthAndNorm.z * vLengthAndNorm.z + vLengthAndNorm.w * vLengthAndNorm.w, 1.0f, ezMath::BasicType<ezMathTestType>::SmallEpsilon());
    EZ_TEST_BOOL(vLengthAndNorm.IsNormalized(ezMath::BasicType<ezMathTestType>::SmallEpsilon()));

    // GetNormalized
    ezSimdVec4f vGetNorm = vOp1.GetNormalized();
    EZ_TEST_FLOAT(vGetNorm.x * vGetNorm.x + vGetNorm.y * vGetNorm.y + vGetNorm.z * vGetNorm.z + vGetNorm.w * vGetNorm.w, 1.0f, ezMath::BasicType<ezMathTestType>::SmallEpsilon());
    EZ_TEST_BOOL(vGetNorm.IsNormalized(ezMath::BasicType<ezMathTestType>::SmallEpsilon()));

    // Normalize
    ezSimdVec4f vNorm = vOp1;
    vNorm.Normalize();
    EZ_TEST_FLOAT(vNorm.x * vNorm.x + vNorm.y * vNorm.y + vNorm.z * vNorm.z + vNorm.w * vNorm.w, 1.0f, ezMath::BasicType<ezMathTestType>::SmallEpsilon());
    EZ_TEST_BOOL(vNorm.IsNormalized(ezMath::BasicType<ezMathTestType>::SmallEpsilon()));

    // NormalizeIfNotZero
    ezSimdVec4f vNormCond = vNorm * ezMath::BasicType<ezMathTestType>::DefaultEpsilon();
    EZ_TEST_BOOL(vNormCond.NormalizeIfNotZero(vOp1, ezMath::BasicType<ezMathTestType>::LargeEpsilon()) == EZ_FAILURE);
    EZ_TEST_BOOL(vNormCond == vOp1);
    vNormCond = vNorm * ezMath::BasicType<ezMathTestType>::DefaultEpsilon();
    EZ_TEST_BOOL(vNormCond.NormalizeIfNotZero(vOp1, ezMath::BasicType<ezMathTestType>::SmallEpsilon()) == EZ_SUCCESS);
    EZ_TEST_VEC4(vNormCond, vNorm, ezMath::BasicType<ezSimdVec4f::ComponentType>::DefaultEpsilon());

    // IsZero
    EZ_TEST_BOOL(ezSimdVec4f::ZeroVector().IsZero());
    for (int i = 0; i < 4; ++i)
    {
      EZ_TEST_BOOL(!compArray[i].IsZero());
    }

    // IsZero(float)
    EZ_TEST_BOOL(ezSimdVec4f::ZeroVector().IsZero(0.0f));
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
      EZ_TEST_BOOL((compArray[i] * (ezMathTestType) 2).IsNormalized((ezMathTestType) 4));
      EZ_TEST_BOOL((compArray[i] * (ezMathTestType) 2).IsNormalized((ezMathTestType) 4));
    }

    if (ezMath::BasicType<ezMathTestType>::SupportsNaN())
    {
      ezMathTestType TypeNaN = ezMath::BasicType<ezMathTestType>::GetNaN();
      const ezSimdVec4f nanArray[4] = {
        ezSimdVec4f(TypeNaN, 0.0f, 0.0f, 0.0f),
        ezSimdVec4f(0.0f, TypeNaN, 0.0f, 0.0f),
        ezSimdVec4f(0.0f, 0.0f, TypeNaN, 0.0f),
        ezSimdVec4f(0.0f, 0.0f, 0.0f, TypeNaN) };

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

        EZ_TEST_BOOL(!(compArray[i] * ezMath::BasicType<ezMathTestType>::GetInfinity()).IsValid());
        EZ_TEST_BOOL(!(compArray[i] * -ezMath::BasicType<ezMathTestType>::GetInfinity()).IsValid());
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Operators")
  {
    const ezSimdVec4f vOp1(-4.0, 0.2f, -7.0f, -0.0f);
    const ezSimdVec4f vOp2( 2.0, 0.3f,  0.0f,  1.0f);
    const ezSimdVec4f compArray[4] = { ezSimdVec4f(1.0f, 0.0f, 0.0f, 0.0f),
                                  ezSimdVec4f(0.0f, 1.0f, 0.0f, 0.0f),
                                  ezSimdVec4f(0.0f, 0.0f, 1.0f, 0.0f),
                                  ezSimdVec4f(0.0f, 0.0f, 0.0f, 1.0f) };
    // IsIdentical
    EZ_TEST_BOOL(vOp1.IsIdentical(vOp1));
    for (int i = 0; i < 4; ++i)
    {
      EZ_TEST_BOOL(!vOp1.IsIdentical(vOp1 + (ezMathTestType) ezMath::BasicType<ezMathTestType>::SmallEpsilon() * compArray[i]));
      EZ_TEST_BOOL(!vOp1.IsIdentical(vOp1 - (ezMathTestType) ezMath::BasicType<ezMathTestType>::SmallEpsilon() * compArray[i]));
    }

    // IsEqual
    EZ_TEST_BOOL(vOp1.IsEqual(vOp1, 0.0f));
    for (int i = 0; i < 4; ++i)
    {
      EZ_TEST_BOOL(vOp1.IsEqual(vOp1 + ezMath::BasicType<ezMathTestType>::SmallEpsilon()   * compArray[i], 2 * ezMath::BasicType<ezMathTestType>::SmallEpsilon()));
      EZ_TEST_BOOL(vOp1.IsEqual(vOp1 - ezMath::BasicType<ezMathTestType>::SmallEpsilon()   * compArray[i], 2 * ezMath::BasicType<ezMathTestType>::SmallEpsilon()));
      EZ_TEST_BOOL(vOp1.IsEqual(vOp1 + ezMath::BasicType<ezMathTestType>::DefaultEpsilon() * compArray[i], 2 * ezMath::BasicType<ezMathTestType>::DefaultEpsilon()));
      EZ_TEST_BOOL(vOp1.IsEqual(vOp1 - ezMath::BasicType<ezMathTestType>::DefaultEpsilon() * compArray[i], 2 * ezMath::BasicType<ezMathTestType>::DefaultEpsilon()));
    }

    // operator-
    ezSimdVec4f vNegated = -vOp1;
    EZ_TEST_BOOL(vOp1.x == -vNegated.x && vOp1.y == -vNegated.y && vOp1.z == -vNegated.z && vOp1.w == -vNegated.w);

    // operator+= (ezSimdVec4f)
    ezSimdVec4f vPlusAssign = vOp1;
    vPlusAssign += vOp2;
    EZ_TEST_BOOL(vPlusAssign.IsEqual(ezSimdVec4f(-2.0f, 0.5f, -7.0f, 1.0f), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));

    // operator-= (ezSimdVec4f)
    ezSimdVec4f vMinusAssign = vOp1;
    vMinusAssign -= vOp2;
    EZ_TEST_BOOL(vMinusAssign.IsEqual(ezSimdVec4f(-6.0f, -0.1f, -7.0f, -1.0f), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));

    // operator*= (float)
    ezSimdVec4f vMulFloat = vOp1;
    vMulFloat *= 2.0f;
    EZ_TEST_BOOL(vMulFloat.IsEqual(ezSimdVec4f(-8.0f, 0.4f, -14.0f, -0.0f), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));
    vMulFloat *= 0.0f;
    EZ_TEST_BOOL(vMulFloat.IsEqual(ezSimdVec4f::ZeroVector(), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));

    // operator/= (float)
    ezSimdVec4f vDivFloat = vOp1;
    vDivFloat /= 2.0f;
    EZ_TEST_BOOL(vDivFloat.IsEqual(ezSimdVec4f(-2.0f, 0.1f, -3.5f, -0.0f), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));

    //operator+ (ezSimdVec4f, ezSimdVec4f)
    ezSimdVec4f vPlus = (vOp1 + vOp2);
    EZ_TEST_BOOL(vPlus.IsEqual(ezSimdVec4f(-2.0f, 0.5f, -7.0f, 1.0f), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));

    //operator- (ezSimdVec4f, ezSimdVec4f)
    ezSimdVec4f vMinus = (vOp1 - vOp2);
    EZ_TEST_BOOL(vMinus.IsEqual(ezSimdVec4f(-6.0f, -0.1f, -7.0f, -1.0f), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));

    // operator* (float, ezSimdVec4f)
    ezSimdVec4f vMulFloatVec4 = ((ezMathTestType) 2 * vOp1);
    EZ_TEST_BOOL(vMulFloatVec4.IsEqual(ezSimdVec4f(-8.0f, 0.4f, -14.0f, -0.0f), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));
    vMulFloatVec4 = ((ezMathTestType) 0 * vOp1);
    EZ_TEST_BOOL(vMulFloatVec4.IsEqual(ezSimdVec4f::ZeroVector(), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));

    // operator* (ezSimdVec4f, float)
    ezSimdVec4f vMulVec4Float = (vOp1 * (ezMathTestType) 2);
    EZ_TEST_BOOL(vMulVec4Float.IsEqual(ezSimdVec4f(-8.0f, 0.4f, -14.0f, -0.0f), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));
    vMulVec4Float = (vOp1 * (ezMathTestType) 0);
    EZ_TEST_BOOL(vMulVec4Float.IsEqual(ezSimdVec4f::ZeroVector(), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));

    // operator/ (ezSimdVec4f, float)
    ezSimdVec4f vDivVec4Float = (vOp1 / (ezMathTestType) 2);
    EZ_TEST_BOOL(vDivVec4Float.IsEqual(ezSimdVec4f(-2.0f, 0.1f, -3.5f, -0.0f), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));

    // operator== (ezSimdVec4f, ezSimdVec4f)
    EZ_TEST_BOOL(vOp1 == vOp1);
    for (int i = 0; i < 4; ++i)
    {
      EZ_TEST_BOOL( !(vOp1 == (vOp1 + (ezMathTestType) ezMath::BasicType<ezMathTestType>::SmallEpsilon() * compArray[i])) );
      EZ_TEST_BOOL( !(vOp1 == (vOp1 - (ezMathTestType) ezMath::BasicType<ezMathTestType>::SmallEpsilon() * compArray[i])) );
    }

    // operator!= (ezSimdVec4f, ezSimdVec4f)
    EZ_TEST_BOOL(!(vOp1 != vOp1));
    for (int i = 0; i < 4; ++i)
    {
      EZ_TEST_BOOL(vOp1 != (vOp1 + (ezMathTestType) ezMath::BasicType<ezMathTestType>::SmallEpsilon() * compArray[i]));
      EZ_TEST_BOOL(vOp1 != (vOp1 - (ezMathTestType) ezMath::BasicType<ezMathTestType>::SmallEpsilon() * compArray[i]));
    }

    // operator< (ezSimdVec4f, ezSimdVec4f)
    for (int i = 0; i < 4; ++i)
    {
      for (int j = 0; j < 4; ++j)
      {
        if (i == j)
        {
          EZ_TEST_BOOL( !(compArray[i] < compArray[j]) );
          EZ_TEST_BOOL( !(compArray[j] < compArray[i]) );
        }
        else if (i < j)
        {
          EZ_TEST_BOOL( !(compArray[i] < compArray[j]) );
          EZ_TEST_BOOL( compArray[j] < compArray[i] );
        }
        else
        {
          EZ_TEST_BOOL( !(compArray[j] < compArray[i]) );
          EZ_TEST_BOOL( compArray[i] < compArray[j] );
        }
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Common")
  {
    const ezSimdVec4f vOp1(-4.0, 0.2f, -7.0f, -0.0f);
    const ezSimdVec4f vOp2( 2.0, -0.3f, 0.5f,  1.0f);

    // Dot
    EZ_TEST_FLOAT(vOp1.Dot(vOp2), -11.56f, ezMath::BasicType<ezMathTestType>::SmallEpsilon());
    EZ_TEST_FLOAT(vOp2.Dot(vOp1), -11.56f, ezMath::BasicType<ezMathTestType>::SmallEpsilon());

    // CompMin
    EZ_TEST_BOOL(vOp1.CompMin(vOp2).IsEqual(ezSimdVec4f(-4.0f, -0.3f, -7.0f, -0.0f), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));
    EZ_TEST_BOOL(vOp2.CompMin(vOp1).IsEqual(ezSimdVec4f(-4.0f, -0.3f, -7.0f, -0.0f), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));

    // CompMax
    EZ_TEST_BOOL(vOp1.CompMax(vOp2).IsEqual(ezSimdVec4f(2.0f, 0.2f, 0.5f, 1.0f), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));
    EZ_TEST_BOOL(vOp2.CompMax(vOp1).IsEqual(ezSimdVec4f(2.0f, 0.2f, 0.5f, 1.0f), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));

    // CompMult
    EZ_TEST_BOOL(vOp1.CompMult(vOp2).IsEqual(ezSimdVec4f(-8.0f, -0.06f, -3.5f, 0.0f), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));
    EZ_TEST_BOOL(vOp2.CompMult(vOp1).IsEqual(ezSimdVec4f(-8.0f, -0.06f, -3.5f, 0.0f), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));

    // CompDiv
    EZ_TEST_BOOL(vOp1.CompDiv(vOp2).IsEqual(ezSimdVec4f(-2.0f, -0.66666666f, -14.0f, 0.0f), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));
  }
#endif
}
