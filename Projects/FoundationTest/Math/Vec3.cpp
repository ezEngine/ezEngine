#include <PCH.h>

EZ_CREATE_SIMPLE_TEST(Math, Vec3)
{
  EZ_TEST_BLOCK(true, "Constructor")
  {
#if EZ_COMPILE_FOR_DEBUG == 1
    // In debug the default constructor initializes everything with NaN.
    ezVec3 vDefCtor;
    EZ_TEST(ezMath::IsNaN(vDefCtor.x) && ezMath::IsNaN(vDefCtor.y) && ezMath::IsNaN(vDefCtor.z));
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    float testBlock[3] = {1.0f, 2.0f, 3.0f};
    ezVec3* pDefCtor = ::new ((void*)&testBlock[0]) ezVec3;
    EZ_TEST(pDefCtor->x == 1.0f && pDefCtor->y == 2.0f && pDefCtor->z == 3.0f);
#endif

    // Make sure the class didn't accidentally change in size.
    EZ_TEST(sizeof(ezVec3) == 12);

    ezVec3 vInit1F(2.0f);
    EZ_TEST(vInit1F.x == 2.0f && vInit1F.y == 2.0f && vInit1F.z == 2.0f);

    ezVec3 vInit4F(1.0f, 2.0f, 3.0f);
    EZ_TEST(vInit4F.x == 1.0f && vInit4F.y == 2.0f && vInit4F.z == 3.0f);

    ezVec3 vCopy(vInit4F);
    EZ_TEST(vCopy.x == 1.0f && vCopy.y == 2.0f && vCopy.z == 3.0f);

    ezVec3 vZero = ezVec3::ZeroVector();
    EZ_TEST(vZero.x == 0.0f && vZero.y == 0.0f && vZero.z == 0.0f);
  }

  EZ_TEST_BLOCK(true, "Conversion")
  {
    ezVec3 vData(1.0f, 2.0f, 3.0f);
    ezVec2 vToVec2 = vData.GetAsVec2();
    EZ_TEST(vToVec2.x == vData.x && vToVec2.y == vData.y);

    ezVec4 vToVec4 = vData.GetAsVec4(42.0f);
    EZ_TEST(vToVec4.x == vData.x && vToVec4.y == vData.y && vToVec4.z == vData.z && vToVec4.w == 42.0f);

    ezVec4 vToVec4Pos = vData.GetAsPositionVec4();
    EZ_TEST(vToVec4Pos.x == vData.x && vToVec4Pos.y == vData.y && vToVec4Pos.z == vData.z && vToVec4Pos.w == 1.0f);

    ezVec4 vToVec4Dir = vData.GetAsDirectionVec4();
    EZ_TEST(vToVec4Dir.x == vData.x && vToVec4Dir.y == vData.y && vToVec4Dir.z == vData.z && vToVec4Dir.w == 0.0f);
  }

  EZ_TEST_BLOCK(true, "Setter")
  {
    ezVec3 vSet1F;
    vSet1F.Set(2.0f);
    EZ_TEST(vSet1F.x == 2.0f && vSet1F.y == 2.0f && vSet1F.z == 2.0f);

    ezVec3 vSet4F;
    vSet4F.Set(1.0f, 2.0f, 3.0f);
    EZ_TEST(vSet4F.x == 1.0f && vSet4F.y == 2.0f && vSet4F.z == 3.0f);

    ezVec3 vSetZero;
    vSetZero.SetZero();
    EZ_TEST(vSetZero.x == 0.0f && vSetZero.y == 0.0f && vSetZero.z == 0.0f);
  }

  EZ_TEST_BLOCK(true, "Length")
  {
    const ezVec3 vOp1(-4.0, 4.0f, -2.0f);
    const ezVec3 compArray[3] = { ezVec3(1.0f, 0.0f, 0.0f),
                                  ezVec3(0.0f, 1.0f, 0.0f),
                                  ezVec3(0.0f, 0.0f, 1.0f) };

    // GetLength
    EZ_TEST_FLOAT(vOp1.GetLength(), 6.0f, ezMath_SmallEpsilon);

    // SetLength
    ezVec3 vSetLength = vOp1.GetNormalized() * ezMath_DefaultEpsilon;
    EZ_TEST(vSetLength.SetLength(4.0f, ezMath_LargeEpsilon) == EZ_FAILURE);
    EZ_TEST(vSetLength == ezVec3::ZeroVector());
    vSetLength = vOp1.GetNormalized() * ezMath_DefaultEpsilon;
    EZ_TEST(vSetLength.SetLength(4.0f, ezMath_SmallEpsilon) == EZ_SUCCESS);
    EZ_TEST_FLOAT(vSetLength.GetLength(), 4.0f, ezMath_SmallEpsilon);

    // GetLengthSquared
    EZ_TEST_FLOAT(vOp1.GetLengthSquared(), 36.0f, ezMath_SmallEpsilon);

    // GetLengthAndNormalize
    ezVec3 vLengthAndNorm = vOp1;
    float fLength = vLengthAndNorm.GetLengthAndNormalize();
    EZ_TEST_FLOAT(vLengthAndNorm.GetLength(), 1.0f, ezMath_SmallEpsilon);
    EZ_TEST_FLOAT(fLength, 6.0f, ezMath_SmallEpsilon);
    EZ_TEST_FLOAT(vLengthAndNorm.x * vLengthAndNorm.x + vLengthAndNorm.y * vLengthAndNorm.y
                + vLengthAndNorm.z * vLengthAndNorm.z, 1.0f, ezMath_SmallEpsilon);
    EZ_TEST(vLengthAndNorm.IsNormalized(ezMath_SmallEpsilon));

    // GetNormalized
    ezVec3 vGetNorm = vOp1.GetNormalized();
    EZ_TEST_FLOAT(vGetNorm.x * vGetNorm.x + vGetNorm.y * vGetNorm.y + vGetNorm.z * vGetNorm.z, 1.0f, ezMath_SmallEpsilon);
    EZ_TEST(vGetNorm.IsNormalized(ezMath_SmallEpsilon));

    // Normalize
    ezVec3 vNorm = vOp1;
    vNorm.Normalize();
    EZ_TEST_FLOAT(vNorm.x * vNorm.x + vNorm.y * vNorm.y + vNorm.z * vNorm.z, 1.0f, ezMath_SmallEpsilon);
    EZ_TEST(vNorm.IsNormalized(ezMath_SmallEpsilon));

    // NormalizeIfNotZero
    ezVec3 vNormCond = vNorm * ezMath_DefaultEpsilon;
    EZ_TEST(vNormCond.NormalizeIfNotZero(vOp1, ezMath_LargeEpsilon) == EZ_FAILURE);
    EZ_TEST(vNormCond == vOp1);
    vNormCond = vNorm * ezMath_DefaultEpsilon;
    EZ_TEST(vNormCond.NormalizeIfNotZero(vOp1, ezMath_SmallEpsilon) == EZ_SUCCESS);
    EZ_TEST(vNormCond == vNorm);

    // IsZero
    EZ_TEST(ezVec3::ZeroVector().IsZero());
    for (int i = 0; i < 3; ++i)
    {
      EZ_TEST(!compArray[i].IsZero());
    }

    // IsZero(float)
    EZ_TEST(ezVec3::ZeroVector().IsZero(0.0f));
    for (int i = 0; i < 3; ++i)
    {
      EZ_TEST(!compArray[i].IsZero(0.0f));
      EZ_TEST(compArray[i].IsZero(1.0f));
      EZ_TEST((-compArray[i]).IsZero(1.0f));
    }

    // IsNormalized (already tested above)
    for (int i = 0; i < 3; ++i)
    {
      EZ_TEST(compArray[i].IsNormalized());
      EZ_TEST((-compArray[i]).IsNormalized());
      EZ_TEST((compArray[i] * 2.0f).IsNormalized(4.0f));
      EZ_TEST((compArray[i] * 2.0f).IsNormalized(4.0f));
    }

    float fNaN = ezMath::NaN();
    const ezVec3 nanArray[3] = {
      ezVec3(fNaN, 0.0f, 0.0f),
      ezVec3(0.0f, fNaN, 0.0f),
      ezVec3(0.0f, 0.0f, fNaN) };

    // IsNaN
    for (int i = 0; i < 3; ++i)
    {
      EZ_TEST(nanArray[i].IsNaN());
      EZ_TEST(!compArray[i].IsNaN());
    }

    // IsValid
    for (int i = 0; i < 3; ++i)
    {
      EZ_TEST(!nanArray[i].IsValid());
      EZ_TEST(compArray[i].IsValid());
      EZ_TEST(!(compArray[i] * ezMath::Infinity()).IsValid());
      EZ_TEST(!(compArray[i] * -ezMath::Infinity()).IsValid());
    }
  }

  EZ_TEST_BLOCK(true, "Operators")
  {
    const ezVec3 vOp1(-4.0, 0.2f, -7.0f);
    const ezVec3 vOp2( 2.0, 0.3f,  0.0f);
    const ezVec3 compArray[3] = { ezVec3(1.0f, 0.0f, 0.0f),
                                  ezVec3(0.0f, 1.0f, 0.0f),
                                  ezVec3(0.0f, 0.0f, 1.0f) };
    // IsIdentical
    EZ_TEST(vOp1.IsIdentical(vOp1));
    for (int i = 0; i < 3; ++i)
    {
      EZ_TEST(!vOp1.IsIdentical(vOp1 + ezMath_SmallEpsilon * compArray[i]));
      EZ_TEST(!vOp1.IsIdentical(vOp1 - ezMath_SmallEpsilon * compArray[i]));
    }

    // IsEqual
    EZ_TEST(vOp1.IsEqual(vOp1, 0.0f));
    for (int i = 0; i < 3; ++i)
    {
      EZ_TEST(vOp1.IsEqual(vOp1 + ezMath_SmallEpsilon * compArray[i], ezMath_SmallEpsilon));
      EZ_TEST(vOp1.IsEqual(vOp1 - ezMath_SmallEpsilon * compArray[i], ezMath_SmallEpsilon));
      EZ_TEST(vOp1.IsEqual(vOp1 + ezMath_DefaultEpsilon * compArray[i], ezMath_DefaultEpsilon));
      EZ_TEST(vOp1.IsEqual(vOp1 - ezMath_DefaultEpsilon * compArray[i], ezMath_DefaultEpsilon));
    }

    // operator-
    ezVec3 vNegated = -vOp1;
    EZ_TEST(vOp1.x == -vNegated.x && vOp1.y == -vNegated.y && vOp1.z == -vNegated.z);
    
    // operator+= (ezVec3)
    ezVec3 vPlusAssign = vOp1;
    vPlusAssign += vOp2;
    EZ_TEST(vPlusAssign.IsEqual(ezVec3(-2.0f, 0.5f, -7.0f), ezMath_SmallEpsilon));

    // operator-= (ezVec3)
    ezVec3 vMinusAssign = vOp1;
    vMinusAssign -= vOp2;
    EZ_TEST(vMinusAssign.IsEqual(ezVec3(-6.0f, -0.1f, -7.0f), ezMath_SmallEpsilon));

    // operator*= (float)
    ezVec3 vMulFloat = vOp1;
    vMulFloat *= 2.0f;
    EZ_TEST(vMulFloat.IsEqual(ezVec3(-8.0f, 0.4f, -14.0f), ezMath_SmallEpsilon));
    vMulFloat *= 0.0f;
    EZ_TEST(vMulFloat.IsEqual(ezVec3::ZeroVector(), ezMath_SmallEpsilon));

    // operator/= (float)
    ezVec3 vDivFloat = vOp1;
    vDivFloat /= 2.0f;
    EZ_TEST(vDivFloat.IsEqual(ezVec3(-2.0f, 0.1f, -3.5f), ezMath_SmallEpsilon));

    //operator+ (ezVec3, ezVec3)
    ezVec3 vPlus = (vOp1 + vOp2);
    EZ_TEST(vPlus.IsEqual(ezVec3(-2.0f, 0.5f, -7.0f), ezMath_SmallEpsilon));

    //operator- (ezVec3, ezVec3)
    ezVec3 vMinus = (vOp1 - vOp2);
    EZ_TEST(vMinus.IsEqual(ezVec3(-6.0f, -0.1f, -7.0f), ezMath_SmallEpsilon));

    // operator* (float, ezVec3)
    ezVec3 vMulFloatVec3 = (2.0f * vOp1);
    EZ_TEST(vMulFloatVec3.IsEqual(ezVec3(-8.0f, 0.4f, -14.0f), ezMath_SmallEpsilon));
    vMulFloatVec3 = (0.0f * vOp1);
    EZ_TEST(vMulFloatVec3.IsEqual(ezVec3::ZeroVector(), ezMath_SmallEpsilon));

    // operator* (ezVec3, float)
    ezVec3 vMulVec3Float = (vOp1 * 2.0f);
    EZ_TEST(vMulVec3Float.IsEqual(ezVec3(-8.0f, 0.4f, -14.0f), ezMath_SmallEpsilon));
    vMulVec3Float = (vOp1 * 0.0f);
    EZ_TEST(vMulVec3Float.IsEqual(ezVec3::ZeroVector(), ezMath_SmallEpsilon));

    // operator/ (ezVec3, float)
    ezVec3 vDivVec3Float = (vOp1 / 2.0f);
    EZ_TEST(vDivFloat.IsEqual(ezVec3(-2.0f, 0.1f, -3.5f), ezMath_SmallEpsilon));

    // operator== (ezVec3, ezVec3)
    EZ_TEST(vOp1 == vOp1);
    for (int i = 0; i < 3; ++i)
    {
      EZ_TEST( !(vOp1 == (vOp1 + ezMath_SmallEpsilon * compArray[i])) );
      EZ_TEST( !(vOp1 == (vOp1 - ezMath_SmallEpsilon * compArray[i])) );
    }

    // operator!= (ezVec3, ezVec3)
    EZ_TEST(!(vOp1 != vOp1));
    for (int i = 0; i < 3; ++i)
    {
      EZ_TEST(vOp1 != (vOp1 + ezMath_SmallEpsilon * compArray[i]));
      EZ_TEST(vOp1 != (vOp1 - ezMath_SmallEpsilon * compArray[i]));
    }

    // operator< (ezVec3, ezVec3)
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
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
    const ezVec3 vOp1(-4.0, 0.2f, -7.0f);
    const ezVec3 vOp2( 2.0, -0.3f, 0.5f);

    const ezVec3 compArray[3] = { ezVec3(1.0f, 0.0f, 0.0f),
                                  ezVec3(0.0f, 1.0f, 0.0f),
                                  ezVec3(0.0f, 0.0f, 1.0f) };

    // GetAngleBetween
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        EZ_TEST_FLOAT(compArray[i].GetAngleBetween(compArray[j]), i == j ? 0.0f : 90.0f, 0.00001f);
      }
    }

    // Dot
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        EZ_TEST_FLOAT(compArray[i].Dot(compArray[j]), i == j ? 1.0f : 0.0f, ezMath_SmallEpsilon);
      }
    }
    EZ_TEST_FLOAT(vOp1.Dot(vOp2), -11.56f, ezMath_SmallEpsilon);
    EZ_TEST_FLOAT(vOp2.Dot(vOp1), -11.56f, ezMath_SmallEpsilon);

    // Cross
    // Right-handed coordinate system check
    EZ_TEST(compArray[0].Cross(compArray[1]).IsEqual(compArray[2], ezMath_SmallEpsilon));
    EZ_TEST(compArray[1].Cross(compArray[2]).IsEqual(compArray[0], ezMath_SmallEpsilon));
    EZ_TEST(compArray[2].Cross(compArray[0]).IsEqual(compArray[1], ezMath_SmallEpsilon));

    // CompMin
    EZ_TEST(vOp1.CompMin(vOp2).IsEqual(ezVec3(-4.0f, -0.3f, -7.0f), ezMath_SmallEpsilon));
    EZ_TEST(vOp2.CompMin(vOp1).IsEqual(ezVec3(-4.0f, -0.3f, -7.0f), ezMath_SmallEpsilon));

    // CompMax
    EZ_TEST(vOp1.CompMax(vOp2).IsEqual(ezVec3(2.0f, 0.2f, 0.5f), ezMath_SmallEpsilon));
    EZ_TEST(vOp2.CompMax(vOp1).IsEqual(ezVec3(2.0f, 0.2f, 0.5f), ezMath_SmallEpsilon));

    // CompMult
    EZ_TEST(vOp1.CompMult(vOp2).IsEqual(ezVec3(-8.0f, -0.06f, -3.5f), ezMath_SmallEpsilon));
    EZ_TEST(vOp2.CompMult(vOp1).IsEqual(ezVec3(-8.0f, -0.06f, -3.5f), ezMath_SmallEpsilon));

    // CompDiv
    EZ_TEST(vOp1.CompDiv(vOp2).IsEqual(ezVec3(-2.0f, -0.66666666f, -14.0f), ezMath_SmallEpsilon));
  }

  EZ_TEST_BLOCK(true, "Other Common")
  {
    // CalculateNormal

    // MakeOrthogonalTo

    // GetOrthogonalVector

    // GetReflectedVector

    // GetRefractedVector
  }
}