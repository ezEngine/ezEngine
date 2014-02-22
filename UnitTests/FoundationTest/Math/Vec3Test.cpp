#include <PCH.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec4.h>


EZ_CREATE_SIMPLE_TEST(Math, Vec3)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    if (ezMath::BasicType<ezVec3T::ComponentType>::SupportsNaN())
    {
      // In debug the default constructor initializes everything with NaN.
      ezVec3T vDefCtor;
      EZ_TEST_BOOL(ezMath::IsNaN(vDefCtor.x) && ezMath::IsNaN(vDefCtor.y) && ezMath::IsNaN(vDefCtor.z));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    ezVec3T::ComponentType testBlock[3] = { (ezVec3T::ComponentType) 1, (ezVec3T::ComponentType) 2, (ezVec3T::ComponentType) 3 };
    ezVec3T* pDefCtor = ::new ((void*)&testBlock[0]) ezVec3T;
    EZ_TEST_BOOL(pDefCtor->x == (ezVec3T::ComponentType) 1 && 
            pDefCtor->y == (ezVec3T::ComponentType) 2 && 
            pDefCtor->z == (ezVec3T::ComponentType) 3.);
#endif

    // Make sure the class didn't accidentally change in size.
    EZ_TEST_BOOL(sizeof(ezVec3) == 12);
    EZ_TEST_BOOL(sizeof(ezVec3d) == 24);

    ezVec3T vInit1F(2.0f);
    EZ_TEST_BOOL(vInit1F.x == 2.0f && vInit1F.y == 2.0f && vInit1F.z == 2.0f);

    ezVec3T vInit4F(1.0f, 2.0f, 3.0f);
    EZ_TEST_BOOL(vInit4F.x == 1.0f && vInit4F.y == 2.0f && vInit4F.z == 3.0f);

    ezVec3T vCopy(vInit4F);
    EZ_TEST_BOOL(vCopy.x == 1.0f && vCopy.y == 2.0f && vCopy.z == 3.0f);

    ezVec3T vZero = ezVec3T::ZeroVector();
    EZ_TEST_BOOL(vZero.x == 0.0f && vZero.y == 0.0f && vZero.z == 0.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Conversion")
  {
    ezVec3T vData(1.0f, 2.0f, 3.0f);
    ezVec2T vToVec2 = vData.GetAsVec2();
    EZ_TEST_BOOL(vToVec2.x == vData.x && vToVec2.y == vData.y);

    ezVec4T vToVec4 = vData.GetAsVec4(42.0f);
    EZ_TEST_BOOL(vToVec4.x == vData.x && vToVec4.y == vData.y && vToVec4.z == vData.z && vToVec4.w == 42.0f);

    ezVec4T vToVec4Pos = vData.GetAsPositionVec4();
    EZ_TEST_BOOL(vToVec4Pos.x == vData.x && vToVec4Pos.y == vData.y && vToVec4Pos.z == vData.z && vToVec4Pos.w == 1.0f);

    ezVec4T vToVec4Dir = vData.GetAsDirectionVec4();
    EZ_TEST_BOOL(vToVec4Dir.x == vData.x && vToVec4Dir.y == vData.y && vToVec4Dir.z == vData.z && vToVec4Dir.w == 0.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Setter")
  {
    ezVec3T vSet1F;
    vSet1F.Set(2.0f);
    EZ_TEST_BOOL(vSet1F.x == 2.0f && vSet1F.y == 2.0f && vSet1F.z == 2.0f);

    ezVec3T vSet4F;
    vSet4F.Set(1.0f, 2.0f, 3.0f);
    EZ_TEST_BOOL(vSet4F.x == 1.0f && vSet4F.y == 2.0f && vSet4F.z == 3.0f);

    ezVec3T vSetZero;
    vSetZero.SetZero();
    EZ_TEST_BOOL(vSetZero.x == 0.0f && vSetZero.y == 0.0f && vSetZero.z == 0.0f);
  }

  
  {
    const ezVec3T vOp1(-4.0, 4.0f, -2.0f);
    const ezVec3T compArray[3] = { ezVec3T(1.0f, 0.0f, 0.0f),
                                  ezVec3T(0.0f, 1.0f, 0.0f),
                                  ezVec3T(0.0f, 0.0f, 1.0f) };

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetLength")
    {
      EZ_TEST_FLOAT(vOp1.GetLength(), 6.0f, ezMath::BasicType<ezMathTestType>::SmallEpsilon());
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetLength")
    {
      ezVec3T vSetLength = vOp1.GetNormalized() * ezMath::BasicType<ezMathTestType>::DefaultEpsilon();
      EZ_TEST_BOOL(vSetLength.SetLength(4.0f, ezMath::BasicType<ezMathTestType>::LargeEpsilon()) == EZ_FAILURE);
      EZ_TEST_BOOL(vSetLength == ezVec3T::ZeroVector());
      vSetLength = vOp1.GetNormalized() * (ezMathTestType) 0.001;
      EZ_TEST_BOOL(vSetLength.SetLength(4.0f, (ezMathTestType) ezMath::BasicType<ezMathTestType>::DefaultEpsilon()) == EZ_SUCCESS);
      EZ_TEST_FLOAT(vSetLength.GetLength(), 4.0f, ezMath::BasicType<ezMathTestType>::SmallEpsilon());
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetLengthSquared")
    {
      EZ_TEST_FLOAT(vOp1.GetLengthSquared(), 36.0f, ezMath::BasicType<ezMathTestType>::SmallEpsilon());
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetLengthAndNormalize")
    {
      ezVec3T vLengthAndNorm = vOp1;
      ezMathTestType fLength = vLengthAndNorm.GetLengthAndNormalize();
      EZ_TEST_FLOAT(vLengthAndNorm.GetLength(), 1.0f, ezMath::BasicType<ezMathTestType>::SmallEpsilon());
      EZ_TEST_FLOAT(fLength, 6.0f, ezMath::BasicType<ezMathTestType>::SmallEpsilon());
      EZ_TEST_FLOAT(vLengthAndNorm.x * vLengthAndNorm.x + vLengthAndNorm.y * vLengthAndNorm.y
                  + vLengthAndNorm.z * vLengthAndNorm.z, 1.0f, ezMath::BasicType<ezMathTestType>::SmallEpsilon());
      EZ_TEST_BOOL(vLengthAndNorm.IsNormalized(ezMath::BasicType<ezMathTestType>::SmallEpsilon()));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetNormalized")
    {
      ezVec3T vGetNorm = vOp1.GetNormalized();
      EZ_TEST_FLOAT(vGetNorm.x * vGetNorm.x + vGetNorm.y * vGetNorm.y + vGetNorm.z * vGetNorm.z, 1.0f, ezMath::BasicType<ezMathTestType>::SmallEpsilon());
      EZ_TEST_BOOL(vGetNorm.IsNormalized(ezMath::BasicType<ezMathTestType>::SmallEpsilon()));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "Normalize")
    {
      ezVec3T vNorm = vOp1;
      vNorm.Normalize();
      EZ_TEST_FLOAT(vNorm.x * vNorm.x + vNorm.y * vNorm.y + vNorm.z * vNorm.z, 1.0f, ezMath::BasicType<ezMathTestType>::SmallEpsilon());
      EZ_TEST_BOOL(vNorm.IsNormalized(ezMath::BasicType<ezMathTestType>::SmallEpsilon()));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "NormalizeIfNotZero")
    {
      ezVec3T vNorm = vOp1;
      vNorm.Normalize();

      ezVec3T vNormCond = vNorm * ezMath::BasicType<ezMathTestType>::DefaultEpsilon();
      EZ_TEST_BOOL(vNormCond.NormalizeIfNotZero(vOp1, ezMath::BasicType<ezMathTestType>::LargeEpsilon()) == EZ_FAILURE);
      EZ_TEST_BOOL(vNormCond == vOp1);
      vNormCond = vNorm * ezMath::BasicType<ezMathTestType>::DefaultEpsilon();
      EZ_TEST_BOOL(vNormCond.NormalizeIfNotZero(vOp1, ezMath::BasicType<ezMathTestType>::SmallEpsilon()) == EZ_SUCCESS);
      EZ_TEST_VEC3(vNormCond, vNorm, ezMath::BasicType<ezVec3T::ComponentType>::DefaultEpsilon());
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsZero")
    {
      EZ_TEST_BOOL(ezVec3T::ZeroVector().IsZero());
      for (int i = 0; i < 3; ++i)
      {
        EZ_TEST_BOOL(!compArray[i].IsZero());
      }
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsZero(float)")
    {
      EZ_TEST_BOOL(ezVec3T::ZeroVector().IsZero(0.0f));
      for (int i = 0; i < 3; ++i)
      {
        EZ_TEST_BOOL(!compArray[i].IsZero(0.0f));
        EZ_TEST_BOOL(compArray[i].IsZero(1.0f));
        EZ_TEST_BOOL((-compArray[i]).IsZero(1.0f));
      }
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsNormalized (2)")
    {
      for (int i = 0; i < 3; ++i)
      {
        EZ_TEST_BOOL(compArray[i].IsNormalized());
        EZ_TEST_BOOL((-compArray[i]).IsNormalized());
        EZ_TEST_BOOL((compArray[i] * (ezMathTestType) 2).IsNormalized((ezMathTestType) 4));
        EZ_TEST_BOOL((compArray[i] * (ezMathTestType) 2).IsNormalized((ezMathTestType) 4));
      }
    }

    if (ezMath::BasicType<ezMathTestType>::SupportsNaN())
    {
      ezMathTestType fNaN = ezMath::BasicType<ezMathTestType>::GetNaN();
      const ezVec3T nanArray[3] = {
        ezVec3T(fNaN, 0.0f, 0.0f),
        ezVec3T(0.0f, fNaN, 0.0f),
        ezVec3T(0.0f, 0.0f, fNaN) };

      EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsNaN")
      {
        for (int i = 0; i < 3; ++i)
        {
          EZ_TEST_BOOL(nanArray[i].IsNaN());
          EZ_TEST_BOOL(!compArray[i].IsNaN());
        }
      }

      EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsValid")
      {
        for (int i = 0; i < 3; ++i)
        {
          EZ_TEST_BOOL(!nanArray[i].IsValid());
          EZ_TEST_BOOL(compArray[i].IsValid());

          EZ_TEST_BOOL(!(compArray[i] * ezMath::BasicType<ezMathTestType>::GetInfinity()).IsValid());
          EZ_TEST_BOOL(!(compArray[i] * -ezMath::BasicType<ezMathTestType>::GetInfinity()).IsValid());
        }
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Operators")
  {
    const ezVec3T vOp1(-4.0, 0.2f, -7.0f);
    const ezVec3T vOp2( 2.0, 0.3f,  0.0f);
    const ezVec3T compArray[3] = { ezVec3T(1.0f, 0.0f, 0.0f),
                                  ezVec3T(0.0f, 1.0f, 0.0f),
                                  ezVec3T(0.0f, 0.0f, 1.0f) };
    // IsIdentical
    EZ_TEST_BOOL(vOp1.IsIdentical(vOp1));
    for (int i = 0; i < 3; ++i)
    {
      EZ_TEST_BOOL(!vOp1.IsIdentical(vOp1 + (ezMathTestType) ezMath::BasicType<ezMathTestType>::SmallEpsilon() * compArray[i]));
      EZ_TEST_BOOL(!vOp1.IsIdentical(vOp1 - (ezMathTestType) ezMath::BasicType<ezMathTestType>::SmallEpsilon() * compArray[i]));
    }

    // IsEqual
    EZ_TEST_BOOL(vOp1.IsEqual(vOp1, 0.0f));
    for (int i = 0; i < 3; ++i)
    {
      EZ_TEST_BOOL(vOp1.IsEqual(vOp1 + ezMath::BasicType<ezMathTestType>::SmallEpsilon()   * compArray[i], 2 * ezMath::BasicType<ezMathTestType>::SmallEpsilon()));
      EZ_TEST_BOOL(vOp1.IsEqual(vOp1 - ezMath::BasicType<ezMathTestType>::SmallEpsilon()   * compArray[i], 2 * ezMath::BasicType<ezMathTestType>::SmallEpsilon()));
      EZ_TEST_BOOL(vOp1.IsEqual(vOp1 + ezMath::BasicType<ezMathTestType>::DefaultEpsilon() * compArray[i], 2 * ezMath::BasicType<ezMathTestType>::DefaultEpsilon()));
      EZ_TEST_BOOL(vOp1.IsEqual(vOp1 - ezMath::BasicType<ezMathTestType>::DefaultEpsilon() * compArray[i], 2 * ezMath::BasicType<ezMathTestType>::DefaultEpsilon()));
    }

    // operator-
    ezVec3T vNegated = -vOp1;
    EZ_TEST_BOOL(vOp1.x == -vNegated.x && vOp1.y == -vNegated.y && vOp1.z == -vNegated.z);
    
    // operator+= (ezVec3T)
    ezVec3T vPlusAssign = vOp1;
    vPlusAssign += vOp2;
    EZ_TEST_BOOL(vPlusAssign.IsEqual(ezVec3T(-2.0f, 0.5f, -7.0f), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));

    // operator-= (ezVec3T)
    ezVec3T vMinusAssign = vOp1;
    vMinusAssign -= vOp2;
    EZ_TEST_BOOL(vMinusAssign.IsEqual(ezVec3T(-6.0f, -0.1f, -7.0f), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));

    // operator*= (float)
    ezVec3T vMulFloat = vOp1;
    vMulFloat *= 2.0f;
    EZ_TEST_BOOL(vMulFloat.IsEqual(ezVec3T(-8.0f, 0.4f, -14.0f), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));
    vMulFloat *= 0.0f;
    EZ_TEST_BOOL(vMulFloat.IsEqual(ezVec3T::ZeroVector(), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));

    // operator/= (float)
    ezVec3T vDivFloat = vOp1;
    vDivFloat /= 2.0f;
    EZ_TEST_BOOL(vDivFloat.IsEqual(ezVec3T(-2.0f, 0.1f, -3.5f), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));

    //operator+ (ezVec3T, ezVec3T)
    ezVec3T vPlus = (vOp1 + vOp2);
    EZ_TEST_BOOL(vPlus.IsEqual(ezVec3T(-2.0f, 0.5f, -7.0f), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));

    //operator- (ezVec3T, ezVec3T)
    ezVec3T vMinus = (vOp1 - vOp2);
    EZ_TEST_BOOL(vMinus.IsEqual(ezVec3T(-6.0f, -0.1f, -7.0f), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));

    // operator* (float, ezVec3T)
    ezVec3T vMulFloatVec3 = ((ezMathTestType) 2 * vOp1);
    EZ_TEST_BOOL(vMulFloatVec3.IsEqual(ezVec3T((ezMathTestType) -8.0, (ezMathTestType) 0.4, (ezMathTestType) -14.0), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));
    vMulFloatVec3 = ((ezMathTestType) 0 * vOp1);
    EZ_TEST_BOOL(vMulFloatVec3.IsEqual(ezVec3T::ZeroVector(), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));

    // operator* (ezVec3T, float)
    ezVec3T vMulVec3Float = (vOp1 * (ezMathTestType) 2);
    EZ_TEST_BOOL(vMulVec3Float.IsEqual(ezVec3T(-8.0f, 0.4f, -14.0f), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));
    vMulVec3Float = (vOp1 * (ezMathTestType) 0);
    EZ_TEST_BOOL(vMulVec3Float.IsEqual(ezVec3T::ZeroVector(), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));

    // operator/ (ezVec3T, float)
    ezVec3T vDivVec3Float = (vOp1 / (ezMathTestType) 2);
    EZ_TEST_BOOL(vDivVec3Float.IsEqual(ezVec3T(-2.0f, 0.1f, -3.5f), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));

    // operator== (ezVec3T, ezVec3T)
    EZ_TEST_BOOL(vOp1 == vOp1);
    for (int i = 0; i < 3; ++i)
    {
      EZ_TEST_BOOL( !(vOp1 == (vOp1 + (ezMathTestType) ezMath::BasicType<ezMathTestType>::SmallEpsilon() * compArray[i])) );
      EZ_TEST_BOOL( !(vOp1 == (vOp1 - (ezMathTestType) ezMath::BasicType<ezMathTestType>::SmallEpsilon() * compArray[i])) );
    }

    // operator!= (ezVec3T, ezVec3T)
    EZ_TEST_BOOL(!(vOp1 != vOp1));
    for (int i = 0; i < 3; ++i)
    {
      EZ_TEST_BOOL(vOp1 != (vOp1 + (ezMathTestType) ezMath::BasicType<ezMathTestType>::SmallEpsilon() * compArray[i]));
      EZ_TEST_BOOL(vOp1 != (vOp1 - (ezMathTestType) ezMath::BasicType<ezMathTestType>::SmallEpsilon() * compArray[i]));
    }

    // operator< (ezVec3T, ezVec3T)
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
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
    const ezVec3T vOp1(-4.0, 0.2f, -7.0f);
    const ezVec3T vOp2( 2.0, -0.3f, 0.5f);

    const ezVec3T compArray[3] = { ezVec3T(1.0f, 0.0f, 0.0f),
                                  ezVec3T(0.0f, 1.0f, 0.0f),
                                  ezVec3T(0.0f, 0.0f, 1.0f) };

    // GetAngleBetween
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        EZ_TEST_FLOAT(compArray[i].GetAngleBetween(compArray[j]).GetDegree(), i == j ? 0.0f : 90.0f, 0.00001f);
      }
    }

    // Dot
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        EZ_TEST_FLOAT(compArray[i].Dot(compArray[j]), i == j ? 1.0f : 0.0f, ezMath::BasicType<ezMathTestType>::SmallEpsilon());
      }
    }
    EZ_TEST_FLOAT(vOp1.Dot(vOp2), -11.56f, ezMath::BasicType<ezMathTestType>::SmallEpsilon());
    EZ_TEST_FLOAT(vOp2.Dot(vOp1), -11.56f, ezMath::BasicType<ezMathTestType>::SmallEpsilon());

    // Cross
    // Right-handed coordinate system check
    EZ_TEST_BOOL(compArray[0].Cross(compArray[1]).IsEqual(compArray[2], ezMath::BasicType<ezMathTestType>::SmallEpsilon()));
    EZ_TEST_BOOL(compArray[1].Cross(compArray[2]).IsEqual(compArray[0], ezMath::BasicType<ezMathTestType>::SmallEpsilon()));
    EZ_TEST_BOOL(compArray[2].Cross(compArray[0]).IsEqual(compArray[1], ezMath::BasicType<ezMathTestType>::SmallEpsilon()));

    // CompMin
    EZ_TEST_BOOL(vOp1.CompMin(vOp2).IsEqual(ezVec3T(-4.0f, -0.3f, -7.0f), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));
    EZ_TEST_BOOL(vOp2.CompMin(vOp1).IsEqual(ezVec3T(-4.0f, -0.3f, -7.0f), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));

    // CompMax
    EZ_TEST_BOOL(vOp1.CompMax(vOp2).IsEqual(ezVec3T(2.0f, 0.2f, 0.5f), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));
    EZ_TEST_BOOL(vOp2.CompMax(vOp1).IsEqual(ezVec3T(2.0f, 0.2f, 0.5f), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));

    // CompMult
    EZ_TEST_BOOL(vOp1.CompMult(vOp2).IsEqual(ezVec3T(-8.0f, -0.06f, -3.5f), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));
    EZ_TEST_BOOL(vOp2.CompMult(vOp1).IsEqual(ezVec3T(-8.0f, -0.06f, -3.5f), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));

    // CompDiv
    EZ_TEST_BOOL(vOp1.CompDiv(vOp2).IsEqual(ezVec3T(-2.0f, -0.66666666f, -14.0f), ezMath::BasicType<ezMathTestType>::SmallEpsilon()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CalculateNormal")
  {
    ezVec3T n;
    EZ_TEST_BOOL(n.CalculateNormal(ezVec3T(-1, 0, 1), ezVec3T(1, 0, 1), ezVec3T(0, 0, -1)) == EZ_SUCCESS);
    EZ_TEST_VEC3(n, ezVec3T(0, 1, 0), 0.001f);

    EZ_TEST_BOOL(n.CalculateNormal(ezVec3T(-1, 0, -1), ezVec3T(1, 0, -1), ezVec3T(0, 0, 1)) == EZ_SUCCESS);
    EZ_TEST_VEC3(n, ezVec3T(0, -1, 0), 0.001f);

    EZ_TEST_BOOL(n.CalculateNormal(ezVec3T(-1, 0, 1), ezVec3T(1, 0, 1), ezVec3T(1, 0, 1)) == EZ_FAILURE);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeOrthogonalTo")
  {
    ezVec3T v;
    
    v.Set(1, 1, 0);
    v.MakeOrthogonalTo(ezVec3T(1, 0, 0));
    EZ_TEST_VEC3(v, ezVec3T(0, 1, 0), 0.001f);

    v.Set(1, 1, 0);
    v.MakeOrthogonalTo(ezVec3T(0, 1, 0));
    EZ_TEST_VEC3(v, ezVec3T(1, 0, 0), 0.001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetOrthogonalVector")
  {
    ezVec3T v;
   
    for (float i = 1; i < 360; i += 3.0f)
    {
      v.Set(i, i * 3, i * 7);
      EZ_TEST_FLOAT(v.GetOrthogonalVector().Dot(v), 0.0f, 0.001f);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetReflectedVector")
  {
    ezVec3T v, v2;

    v.Set(1, 1, 0);
    v2 = v.GetReflectedVector(ezVec3T(0, -1, 0));
    EZ_TEST_VEC3(v2, ezVec3T(1, -1, 0), 0.0001f);
  }
}