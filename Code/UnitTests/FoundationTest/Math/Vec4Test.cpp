#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec4.h>

template <typename Type>
void TestVec4()
{
  using ezVec4Type = ezVec4Template<Type>;
  using ezVec2Type = ezVec2Template<Type>;
  using ezVec3Type = ezVec3Template<Type>;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    if (ezMath::SupportsNaN<Type>())
    {
      // In debug the default constructor initializes everything with NaN.
      ezVec4Type vDefCtor;
      EZ_TEST_BOOL(ezMath::IsNaN(vDefCtor.x) && ezMath::IsNaN(vDefCtor.y) /* && ezMath::IsNaN(vDefCtor.z) && ezMath::IsNaN(vDefCtor.w)*/);
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    Type testBlock[4] = {
      (Type)1, (Type)2, (Type)3, (Type)4};
    ezVec4Type* pDefCtor = ::new ((void*)&testBlock[0]) ezVec4Type;
    EZ_TEST_BOOL(pDefCtor->x == (Type)1 && pDefCtor->y == (Type)2 && pDefCtor->z == (Type)3 &&
                 pDefCtor->w == (Type)4);
#endif

    // Make sure the class didn't accidentally change in size.
    EZ_TEST_BOOL(sizeof(ezVec4) == 16);
    EZ_TEST_BOOL(sizeof(ezVec4d) == 32);

    ezVec4Type vInit1F(2.0f);
    EZ_TEST_BOOL(vInit1F.x == 2.0f && vInit1F.y == 2.0f && vInit1F.z == 2.0f && vInit1F.w == 2.0f);

    ezVec4Type vInit4F(1.0f, 2.0f, 3.0f, 4.0f);
    EZ_TEST_BOOL(vInit4F.x == 1.0f && vInit4F.y == 2.0f && vInit4F.z == 3.0f && vInit4F.w == 4.0f);

    ezVec4Type vCopy(vInit4F);
    EZ_TEST_BOOL(vCopy.x == 1.0f && vCopy.y == 2.0f && vCopy.z == 3.0f && vCopy.w == 4.0f);

    ezVec4Type vZero = ezVec4Type::MakeZero();
    EZ_TEST_BOOL(vZero.x == 0.0f && vZero.y == 0.0f && vZero.z == 0.0f && vZero.w == 0.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Conversion")
  {
    ezVec4Type vData(1.0f, 2.0f, 3.0f, 4.0f);
    ezVec2Type vToVec2 = vData.GetAsVec2();
    EZ_TEST_BOOL(vToVec2.x == vData.x && vToVec2.y == vData.y);

    ezVec3Type vToVec3 = vData.GetAsVec3();
    EZ_TEST_BOOL(vToVec3.x == vData.x && vToVec3.y == vData.y && vToVec3.z == vData.z);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Setter")
  {
    ezVec4Type vSet1F;
    vSet1F.Set(2.0f);
    EZ_TEST_BOOL(vSet1F.x == 2.0f && vSet1F.y == 2.0f && vSet1F.z == 2.0f && vSet1F.w == 2.0f);

    ezVec4Type vSet4F;
    vSet4F.Set(1.0f, 2.0f, 3.0f, 4.0f);
    EZ_TEST_BOOL(vSet4F.x == 1.0f && vSet4F.y == 2.0f && vSet4F.z == 3.0f && vSet4F.w == 4.0f);

    ezVec4Type vSetZero;
    vSetZero.SetZero();
    EZ_TEST_BOOL(vSetZero.x == 0.0f && vSetZero.y == 0.0f && vSetZero.z == 0.0f && vSetZero.w == 0.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Length")
  {
    const ezVec4Type vOp1((Type)-4.0, 4.0f, (Type)-2.0, (Type)-0.0);
    const ezVec4Type compArray[4] = {
      ezVec4Type(1.0f, 0.0f, 0.0f, 0.0f), ezVec4Type(0.0f, 1.0f, 0.0f, 0.0f), ezVec4Type(0.0f, 0.0f, 1.0f, 0.0f), ezVec4Type(0.0f, 0.0f, 0.0f, 1.0f)};

    // GetLength
    EZ_TEST_FLOAT(vOp1.GetLength(), 6.0f, ezMath::SmallEpsilon<Type>());

    // GetLengthSquared
    EZ_TEST_FLOAT(vOp1.GetLengthSquared(), 36.0f, ezMath::SmallEpsilon<Type>());

    // GetLengthAndNormalize
    ezVec4Type vLengthAndNorm = vOp1;
    Type fLength = vLengthAndNorm.GetLengthAndNormalize();
    EZ_TEST_FLOAT(vLengthAndNorm.GetLength(), 1.0f, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(fLength, 6.0f, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(vLengthAndNorm.x * vLengthAndNorm.x + vLengthAndNorm.y * vLengthAndNorm.y + vLengthAndNorm.z * vLengthAndNorm.z +
                    vLengthAndNorm.w * vLengthAndNorm.w,
      1.0f, ezMath::SmallEpsilon<Type>());
    EZ_TEST_BOOL(vLengthAndNorm.IsNormalized(ezMath::SmallEpsilon<Type>()));

    // GetNormalized
    ezVec4Type vGetNorm = vOp1.GetNormalized();
    EZ_TEST_FLOAT(vGetNorm.x * vGetNorm.x + vGetNorm.y * vGetNorm.y + vGetNorm.z * vGetNorm.z + vGetNorm.w * vGetNorm.w, 1.0f,
      ezMath::SmallEpsilon<Type>());
    EZ_TEST_BOOL(vGetNorm.IsNormalized(ezMath::SmallEpsilon<Type>()));

    // Normalize
    ezVec4Type vNorm = vOp1;
    vNorm.Normalize();
    EZ_TEST_FLOAT(vNorm.x * vNorm.x + vNorm.y * vNorm.y + vNorm.z * vNorm.z + vNorm.w * vNorm.w, 1.0f, ezMath::SmallEpsilon<Type>());
    EZ_TEST_BOOL(vNorm.IsNormalized(ezMath::SmallEpsilon<Type>()));

    // NormalizeIfNotZero
    ezVec4Type vNormCond = vNorm * ezMath::DefaultEpsilon<Type>();
    EZ_TEST_BOOL(vNormCond.NormalizeIfNotZero(vOp1, ezMath::LargeEpsilon<Type>()) == EZ_FAILURE);
    EZ_TEST_BOOL(vNormCond == vOp1);
    vNormCond = vNorm * ezMath::DefaultEpsilon<Type>();
    EZ_TEST_BOOL(vNormCond.NormalizeIfNotZero(vOp1, ezMath::SmallEpsilon<Type>()) == EZ_SUCCESS);
    EZ_TEST_VEC4(vNormCond, vNorm, ezMath::DefaultEpsilon<Type>());

    // IsZero
    EZ_TEST_BOOL(ezVec4Type::MakeZero().IsZero());
    for (int i = 0; i < 4; ++i)
    {
      EZ_TEST_BOOL(!compArray[i].IsZero());
    }

    // IsZero(float)
    EZ_TEST_BOOL(ezVec4Type::MakeZero().IsZero(0.0f));
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
      EZ_TEST_BOOL((compArray[i] * (Type)2).IsNormalized((Type)4));
      EZ_TEST_BOOL((compArray[i] * (Type)2).IsNormalized((Type)4));
    }

    if (ezMath::SupportsNaN<Type>())
    {
      Type TypeNaN = ezMath::NaN<Type>();
      const ezVec4Type nanArray[4] = {ezVec4Type(TypeNaN, 0.0f, 0.0f, 0.0f), ezVec4Type(0.0f, TypeNaN, 0.0f, 0.0f), ezVec4Type(0.0f, 0.0f, TypeNaN, 0.0f),
        ezVec4Type(0.0f, 0.0f, 0.0f, TypeNaN)};

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

        EZ_TEST_BOOL(!(compArray[i] * ezMath::Infinity<Type>()).IsValid());
        EZ_TEST_BOOL(!(compArray[i] * -ezMath::Infinity<Type>()).IsValid());
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Operators")
  {
    const ezVec4Type vOp1((Type)-4.0, (Type)0.2, (Type)-7.0, (Type)-0.0);
    const ezVec4Type vOp2((Type)2.0, (Type)0.3, (Type)0.0, (Type)1.0);
    const ezVec4Type compArray[4] = {
      ezVec4Type((Type)1.0, (Type)0.0, (Type)0.0, (Type)0.0), ezVec4Type((Type)0.0, (Type)1.0, (Type)0.0, (Type)0.0), ezVec4Type((Type)0.0, (Type)0.0, (Type)1.0, (Type)0.0), ezVec4Type((Type)0.0, (Type)0.0, (Type)0.0, (Type)1.0)};
    // IsIdentical
    EZ_TEST_BOOL(vOp1.IsIdentical(vOp1));
    for (int i = 0; i < 4; ++i)
    {
      EZ_TEST_BOOL(!vOp1.IsIdentical(vOp1 + (Type)ezMath::SmallEpsilon<Type>() * compArray[i]));
      EZ_TEST_BOOL(!vOp1.IsIdentical(vOp1 - (Type)ezMath::SmallEpsilon<Type>() * compArray[i]));
    }

    // IsEqual
    EZ_TEST_BOOL(vOp1.IsEqual(vOp1, (Type)0.0));
    for (int i = 0; i < 4; ++i)
    {
      EZ_TEST_BOOL(vOp1.IsEqual(vOp1 + ezMath::SmallEpsilon<Type>() * compArray[i], 2 * ezMath::SmallEpsilon<Type>()));
      EZ_TEST_BOOL(vOp1.IsEqual(vOp1 - ezMath::SmallEpsilon<Type>() * compArray[i], 2 * ezMath::SmallEpsilon<Type>()));
      EZ_TEST_BOOL(vOp1.IsEqual(vOp1 + ezMath::DefaultEpsilon<Type>() * compArray[i], 2 * ezMath::DefaultEpsilon<Type>()));
      EZ_TEST_BOOL(vOp1.IsEqual(vOp1 - ezMath::DefaultEpsilon<Type>() * compArray[i], 2 * ezMath::DefaultEpsilon<Type>()));
    }

    // operator-
    ezVec4Type vNegated = -vOp1;
    EZ_TEST_BOOL(vOp1.x == -vNegated.x && vOp1.y == -vNegated.y && vOp1.z == -vNegated.z && vOp1.w == -vNegated.w);

    // operator+= (ezVec4Type)
    ezVec4Type vPlusAssign = vOp1;
    vPlusAssign += vOp2;
    EZ_TEST_BOOL(vPlusAssign.IsEqual(ezVec4Type((Type)-2.0, (Type)0.5, (Type)-7.0, (Type)1.0), ezMath::SmallEpsilon<Type>()));

    // operator-= (ezVec4Type)
    ezVec4Type vMinusAssign = vOp1;
    vMinusAssign -= vOp2;
    EZ_TEST_BOOL(vMinusAssign.IsEqual(ezVec4Type((Type)-6.0, (Type)-0.1, (Type)-7.0, (Type)-1.0), ezMath::SmallEpsilon<Type>()));

    // operator*= (float)
    ezVec4Type vMulFloat = vOp1;
    vMulFloat *= 2.0f;
    EZ_TEST_BOOL(vMulFloat.IsEqual(ezVec4Type((Type)-8.0, (Type)0.4, (Type)-14.0, (Type)-0.0), ezMath::SmallEpsilon<Type>()));
    vMulFloat *= 0.0f;
    EZ_TEST_BOOL(vMulFloat.IsEqual(ezVec4Type::MakeZero(), ezMath::SmallEpsilon<Type>()));

    // operator/= (float)
    ezVec4Type vDivFloat = vOp1;
    vDivFloat /= 2.0f;
    EZ_TEST_BOOL(vDivFloat.IsEqual(ezVec4Type((Type)-2.0, (Type)0.1, (Type)-3.5, (Type)-0.0), ezMath::SmallEpsilon<Type>()));

    // operator+ (ezVec4Type, ezVec4Type)
    ezVec4Type vPlus = (vOp1 + vOp2);
    EZ_TEST_BOOL(vPlus.IsEqual(ezVec4Type((Type)-2.0, (Type)0.5, (Type)-7.0, (Type)1.0), ezMath::SmallEpsilon<Type>()));

    // operator- (ezVec4Type, ezVec4Type)
    ezVec4Type vMinus = (vOp1 - vOp2);
    EZ_TEST_BOOL(vMinus.IsEqual(ezVec4Type((Type)-6.0, (Type)-0.1, (Type)-7.0, (Type)-1.0), ezMath::SmallEpsilon<Type>()));

    // operator* (float, ezVec4Type)
    ezVec4Type vMulFloatVec4 = ((Type)2 * vOp1);
    EZ_TEST_BOOL(vMulFloatVec4.IsEqual(ezVec4Type((Type)-8.0, (Type)0.4, (Type)-14.0, (Type)-0.0), ezMath::SmallEpsilon<Type>()));
    vMulFloatVec4 = ((Type)0 * vOp1);
    EZ_TEST_BOOL(vMulFloatVec4.IsEqual(ezVec4Type::MakeZero(), ezMath::SmallEpsilon<Type>()));

    // operator* (ezVec4Type, float)
    ezVec4Type vMulVec4Float = (vOp1 * (Type)2);
    EZ_TEST_BOOL(vMulVec4Float.IsEqual(ezVec4Type((Type)-8.0, (Type)0.4, (Type)-14.0, (Type)-0.0), ezMath::SmallEpsilon<Type>()));
    vMulVec4Float = (vOp1 * (Type)0);
    EZ_TEST_BOOL(vMulVec4Float.IsEqual(ezVec4Type::MakeZero(), ezMath::SmallEpsilon<Type>()));

    // operator/ (ezVec4Type, float)
    ezVec4Type vDivVec4Float = (vOp1 / (Type)2);
    EZ_TEST_BOOL(vDivVec4Float.IsEqual(ezVec4Type((Type)-2.0, (Type)0.1, (Type)-3.5, (Type)-0.0), ezMath::SmallEpsilon<Type>()));

    // operator== (ezVec4Type, ezVec4Type)
    EZ_TEST_BOOL(vOp1 == vOp1);
    for (int i = 0; i < 4; ++i)
    {
      EZ_TEST_BOOL(!(vOp1 == (vOp1 + (Type)ezMath::SmallEpsilon<Type>() * compArray[i])));
      EZ_TEST_BOOL(!(vOp1 == (vOp1 - (Type)ezMath::SmallEpsilon<Type>() * compArray[i])));
    }

    // operator!= (ezVec4Type, ezVec4Type)
    EZ_TEST_BOOL(!(vOp1 != vOp1));
    for (int i = 0; i < 4; ++i)
    {
      EZ_TEST_BOOL(vOp1 != (vOp1 + (Type)ezMath::SmallEpsilon<Type>() * compArray[i]));
      EZ_TEST_BOOL(vOp1 != (vOp1 - (Type)ezMath::SmallEpsilon<Type>() * compArray[i]));
    }

    // operator< (ezVec4Type, ezVec4Type)
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
    const ezVec4Type vOp1((Type)-4.0, (Type)0.2, (Type)-7.0, (Type)-0.0);
    const ezVec4Type vOp2((Type)2.0, (Type)-0.3, (Type)0.5, (Type)1.0);

    // Dot
    EZ_TEST_FLOAT(vOp1.Dot(vOp2), (Type)-11.56, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(vOp2.Dot(vOp1), (Type)-11.56, ezMath::SmallEpsilon<Type>());

    // CompMin
    EZ_TEST_BOOL(vOp1.CompMin(vOp2).IsEqual(ezVec4Type((Type)-4.0, (Type)-0.3, (Type)-7.0, (Type)-0.0), ezMath::SmallEpsilon<Type>()));
    EZ_TEST_BOOL(vOp2.CompMin(vOp1).IsEqual(ezVec4Type((Type)-4.0, (Type)-0.3, (Type)-7.0, (Type)-0.0), ezMath::SmallEpsilon<Type>()));

    // CompMax
    EZ_TEST_BOOL(vOp1.CompMax(vOp2).IsEqual(ezVec4Type((Type)2.0, (Type)0.2, (Type)0.5, (Type)1.0), ezMath::SmallEpsilon<Type>()));
    EZ_TEST_BOOL(vOp2.CompMax(vOp1).IsEqual(ezVec4Type((Type)2.0, (Type)0.2, (Type)0.5, (Type)1.0), ezMath::SmallEpsilon<Type>()));

    // CompClamp
    EZ_TEST_BOOL(vOp1.CompClamp(vOp1, vOp2).IsEqual(ezVec4Type((Type)-4.0, (Type)-0.3, (Type)-7.0, (Type)-0.0), ezMath::SmallEpsilon<Type>()));
    EZ_TEST_BOOL(vOp2.CompClamp(vOp1, vOp2).IsEqual(ezVec4Type((Type)2.0, (Type)0.2, (Type)0.5, (Type)1.0), ezMath::SmallEpsilon<Type>()));

    // CompMul
    EZ_TEST_BOOL(vOp1.CompMul(vOp2).IsEqual(ezVec4Type((Type)-8.0, (Type)-0.06, (Type)-3.5, (Type)0.0), ezMath::SmallEpsilon<Type>()));
    EZ_TEST_BOOL(vOp2.CompMul(vOp1).IsEqual(ezVec4Type((Type)-8.0, (Type)-0.06, (Type)-3.5, (Type)0.0), ezMath::SmallEpsilon<Type>()));

    // CompDiv
    EZ_TEST_BOOL(vOp1.CompDiv(vOp2).IsEqual(ezVec4Type((Type)-2.0, (Type)(-2.0/3.0), (Type)-14.0, (Type)0.0), ezMath::SmallEpsilon<Type>()));

    // Abs
    EZ_TEST_VEC4(vOp1.Abs(), ezVec4Type((Type)4.0, (Type)0.2, (Type)7.0, (Type)0.0), ezMath::SmallEpsilon<Type>());
  }
}


EZ_CREATE_SIMPLE_TEST(Math, Vec4f)
{
  TestVec4<float>();
}
EZ_CREATE_SIMPLE_TEST(Math, Vec4d)
{
  TestVec4<double>();
}
