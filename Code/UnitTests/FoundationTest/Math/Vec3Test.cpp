#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Random.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec4.h>

template <typename Type>
void TestVec3()
{
  using ezVec3Type = ezVec3Template<Type>;
  using ezVec2Type = ezVec2Template<Type>;
  using ezVec4Type = ezVec4Template<Type>;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    if (ezMath::SupportsNaN<Type>())
    {
      // In debug the default constructor initializes everything with NaN.
      ezVec3Type vDefCtor;
      EZ_TEST_BOOL(ezMath::IsNaN(vDefCtor.x) && ezMath::IsNaN(vDefCtor.y) && ezMath::IsNaN(vDefCtor.z));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    Type testBlock[3] = {(Type)1, (Type)2, (Type)3};
    ezVec3Type* pDefCtor = ::new ((void*)&testBlock[0]) ezVec3Type;
    EZ_TEST_BOOL(pDefCtor->x == (Type)1 && pDefCtor->y == (Type)2 && pDefCtor->z == (Type)3);
#endif

    // Make sure the class didn't accidentally change in size.
    EZ_TEST_BOOL(sizeof(ezVec3) == 12);
    EZ_TEST_BOOL(sizeof(ezVec3d) == 24);

    ezVec3Type vInit1F(2.0f);
    EZ_TEST_BOOL(vInit1F.x == 2.0f && vInit1F.y == 2.0f && vInit1F.z == 2.0f);

    ezVec3Type vInit4F(1.0f, 2.0f, 3.0f);
    EZ_TEST_BOOL(vInit4F.x == 1.0f && vInit4F.y == 2.0f && vInit4F.z == 3.0f);

    ezVec3Type vCopy(vInit4F);
    EZ_TEST_BOOL(vCopy.x == 1.0f && vCopy.y == 2.0f && vCopy.z == 3.0f);

    ezVec3Type vZero = ezVec3Type::MakeZero();
    EZ_TEST_BOOL(vZero.x == 0.0f && vZero.y == 0.0f && vZero.z == 0.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Conversion")
  {
    ezVec3Type vData(1.0f, 2.0f, 3.0f);
    ezVec2Type vToVec2 = vData.GetAsVec2();
    EZ_TEST_BOOL(vToVec2.x == vData.x && vToVec2.y == vData.y);

    ezVec4Type vToVec4 = vData.GetAsVec4(42.0f);
    EZ_TEST_BOOL(vToVec4.x == vData.x && vToVec4.y == vData.y && vToVec4.z == vData.z && vToVec4.w == 42.0f);

    ezVec4Type vToVec4Pos = vData.GetAsPositionVec4();
    EZ_TEST_BOOL(vToVec4Pos.x == vData.x && vToVec4Pos.y == vData.y && vToVec4Pos.z == vData.z && vToVec4Pos.w == 1.0f);

    ezVec4Type vToVec4Dir = vData.GetAsDirectionVec4();
    EZ_TEST_BOOL(vToVec4Dir.x == vData.x && vToVec4Dir.y == vData.y && vToVec4Dir.z == vData.z && vToVec4Dir.w == 0.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Setter")
  {
    ezVec3Type vSet1F;
    vSet1F.Set(2.0f);
    EZ_TEST_BOOL(vSet1F.x == 2.0f && vSet1F.y == 2.0f && vSet1F.z == 2.0f);

    ezVec3Type vSet4F;
    vSet4F.Set(1.0f, 2.0f, 3.0f);
    EZ_TEST_BOOL(vSet4F.x == 1.0f && vSet4F.y == 2.0f && vSet4F.z == 3.0f);

    ezVec3Type vSetZero;
    vSetZero.SetZero();
    EZ_TEST_BOOL(vSetZero.x == 0.0f && vSetZero.y == 0.0f && vSetZero.z == 0.0f);
  }


  {
    const ezVec3Type vOp1(-4.0, 4.0f, -2.0f);
    const ezVec3Type compArray[3] = {ezVec3Type(1.0f, 0.0f, 0.0f), ezVec3Type(0.0f, 1.0f, 0.0f), ezVec3Type(0.0f, 0.0f, 1.0f)};

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetLength")
    {
      EZ_TEST_FLOAT(vOp1.GetLength(), 6.0f, ezMath::SmallEpsilon<Type>());
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetLength")
    {
      ezVec3Type vSetLength = vOp1.GetNormalized() * ezMath::DefaultEpsilon<Type>();
      EZ_TEST_BOOL(vSetLength.SetLength(4.0f, ezMath::LargeEpsilon<Type>()) == EZ_FAILURE);
      EZ_TEST_BOOL(vSetLength == ezVec3Type::MakeZero());
      vSetLength = vOp1.GetNormalized() * (Type)0.001;
      EZ_TEST_BOOL(vSetLength.SetLength(4.0f, (Type)ezMath::DefaultEpsilon<Type>()) == EZ_SUCCESS);
      EZ_TEST_FLOAT(vSetLength.GetLength(), 4.0f, ezMath::SmallEpsilon<Type>());
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetLengthSquared")
    {
      EZ_TEST_FLOAT(vOp1.GetLengthSquared(), 36.0f, ezMath::SmallEpsilon<Type>());
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetLengthAndNormalize")
    {
      ezVec3Type vLengthAndNorm = vOp1;
      Type fLength = vLengthAndNorm.GetLengthAndNormalize();
      EZ_TEST_FLOAT(vLengthAndNorm.GetLength(), 1.0f, ezMath::SmallEpsilon<Type>());
      EZ_TEST_FLOAT(fLength, 6.0f, ezMath::SmallEpsilon<Type>());
      EZ_TEST_FLOAT(vLengthAndNorm.x * vLengthAndNorm.x + vLengthAndNorm.y * vLengthAndNorm.y + vLengthAndNorm.z * vLengthAndNorm.z, 1.0f,
        ezMath::SmallEpsilon<Type>());
      EZ_TEST_BOOL(vLengthAndNorm.IsNormalized(ezMath::SmallEpsilon<Type>()));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetNormalized")
    {
      ezVec3Type vGetNorm = vOp1.GetNormalized();
      EZ_TEST_FLOAT(vGetNorm.x * vGetNorm.x + vGetNorm.y * vGetNorm.y + vGetNorm.z * vGetNorm.z, 1.0f, ezMath::SmallEpsilon<Type>());
      EZ_TEST_BOOL(vGetNorm.IsNormalized(ezMath::SmallEpsilon<Type>()));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "Normalize")
    {
      ezVec3Type vNorm = vOp1;
      vNorm.Normalize();
      EZ_TEST_FLOAT(vNorm.x * vNorm.x + vNorm.y * vNorm.y + vNorm.z * vNorm.z, 1.0f, ezMath::SmallEpsilon<Type>());
      EZ_TEST_BOOL(vNorm.IsNormalized(ezMath::SmallEpsilon<Type>()));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "NormalizeIfNotZero")
    {
      ezVec3Type vNorm = vOp1;
      vNorm.Normalize();

      ezVec3Type vNormCond = vNorm * ezMath::DefaultEpsilon<Type>();
      EZ_TEST_BOOL(vNormCond.NormalizeIfNotZero(vOp1, ezMath::LargeEpsilon<Type>()) == EZ_FAILURE);
      EZ_TEST_BOOL(vNormCond == vOp1);
      vNormCond = vNorm * ezMath::DefaultEpsilon<Type>();
      EZ_TEST_BOOL(vNormCond.NormalizeIfNotZero(vOp1, ezMath::SmallEpsilon<Type>()) == EZ_SUCCESS);
      EZ_TEST_VEC3(vNormCond, vNorm, ezMath::DefaultEpsilon<Type>());
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsZero")
    {
      EZ_TEST_BOOL(ezVec3Type::MakeZero().IsZero());
      for (int i = 0; i < 3; ++i)
      {
        EZ_TEST_BOOL(!compArray[i].IsZero());
      }
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsZero(float)")
    {
      EZ_TEST_BOOL(ezVec3Type::MakeZero().IsZero(0.0f));
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
        EZ_TEST_BOOL((compArray[i] * (Type)2).IsNormalized((Type)4));
        EZ_TEST_BOOL((compArray[i] * (Type)2).IsNormalized((Type)4));
      }
    }

    if (ezMath::SupportsNaN<Type>())
    {
      Type fNaN = ezMath::NaN<Type>();
      const ezVec3Type nanArray[3] = {ezVec3Type(fNaN, 0.0f, 0.0f), ezVec3Type(0.0f, fNaN, 0.0f), ezVec3Type(0.0f, 0.0f, fNaN)};

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

          EZ_TEST_BOOL(!(compArray[i] * ezMath::Infinity<Type>()).IsValid());
          EZ_TEST_BOOL(!(compArray[i] * -ezMath::Infinity<Type>()).IsValid());
        }
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Operators")
  {
    const ezVec3Type vOp1((Type)-4.0, (Type)0.2, (Type)-7.0);
    const ezVec3Type vOp2((Type)2.0, (Type)0.3, (Type)0.0);
    const ezVec3Type compArray[3] = {ezVec3Type((Type)1.0, (Type)0.0, (Type)0.0), ezVec3Type((Type)0.0, (Type)1.0, (Type)0.0), ezVec3Type((Type)0.0, (Type)0.0, (Type)1.0)};
    // IsIdentical
    EZ_TEST_BOOL(vOp1.IsIdentical(vOp1));
    for (int i = 0; i < 3; ++i)
    {
      EZ_TEST_BOOL(!vOp1.IsIdentical(vOp1 + (Type)ezMath::SmallEpsilon<Type>() * compArray[i]));
      EZ_TEST_BOOL(!vOp1.IsIdentical(vOp1 - (Type)ezMath::SmallEpsilon<Type>() * compArray[i]));
    }

    // IsEqual
    EZ_TEST_BOOL(vOp1.IsEqual(vOp1, 0.0));
    for (int i = 0; i < 3; ++i)
    {
      EZ_TEST_BOOL(vOp1.IsEqual(vOp1 + ezMath::SmallEpsilon<Type>() * compArray[i], 2 * ezMath::SmallEpsilon<Type>()));
      EZ_TEST_BOOL(vOp1.IsEqual(vOp1 - ezMath::SmallEpsilon<Type>() * compArray[i], 2 * ezMath::SmallEpsilon<Type>()));
      EZ_TEST_BOOL(vOp1.IsEqual(vOp1 + ezMath::DefaultEpsilon<Type>() * compArray[i], 2 * ezMath::DefaultEpsilon<Type>()));
      EZ_TEST_BOOL(vOp1.IsEqual(vOp1 - ezMath::DefaultEpsilon<Type>() * compArray[i], 2 * ezMath::DefaultEpsilon<Type>()));
    }

    // operator-
    ezVec3Type vNegated = -vOp1;
    EZ_TEST_BOOL(vOp1.x == -vNegated.x && vOp1.y == -vNegated.y && vOp1.z == -vNegated.z);

    // operator+= (ezVec3Type)
    ezVec3Type vPlusAssign = vOp1;
    vPlusAssign += vOp2;
    EZ_TEST_BOOL(vPlusAssign.IsEqual(ezVec3Type((Type)-2.0, (Type)0.5, (Type)-7.0), ezMath::SmallEpsilon<Type>()));

    // operator-= (ezVec3Type)
    ezVec3Type vMinusAssign = vOp1;
    vMinusAssign -= vOp2;
    EZ_TEST_BOOL(vMinusAssign.IsEqual(ezVec3Type((Type)-6.0, (Type)-0.1, (Type)-7.0), ezMath::SmallEpsilon<Type>()));

    // operator*= (float)
    ezVec3Type vMulFloat = vOp1;
    vMulFloat *= (Type)2.0;
    EZ_TEST_BOOL(vMulFloat.IsEqual(ezVec3Type((Type)-8.0, (Type)0.4, (Type)-14.0), ezMath::SmallEpsilon<Type>()));
    vMulFloat *= (Type)0.0;
    EZ_TEST_BOOL(vMulFloat.IsEqual(ezVec3Type::MakeZero(), ezMath::SmallEpsilon<Type>()));

    // operator/= (float)
    ezVec3Type vDivFloat = vOp1;
    vDivFloat /= (Type)2.0;
    EZ_TEST_BOOL(vDivFloat.IsEqual(ezVec3Type((Type)-2.0, (Type)0.1, (Type)-3.5), ezMath::SmallEpsilon<Type>()));

    // operator+ (ezVec3Type, ezVec3Type)
    ezVec3Type vPlus = (vOp1 + vOp2);
    EZ_TEST_BOOL(vPlus.IsEqual(ezVec3Type((Type)-2.0, (Type)0.5, (Type)-7.0), ezMath::SmallEpsilon<Type>()));

    // operator- (ezVec3Type, ezVec3Type)
    ezVec3Type vMinus = (vOp1 - vOp2);
    EZ_TEST_BOOL(vMinus.IsEqual(ezVec3Type((Type)-6.0, (Type)-0.1, (Type)-7.0), ezMath::SmallEpsilon<Type>()));

    // operator* (float, ezVec3Type)
    ezVec3Type vMulFloatVec3 = ((Type)2 * vOp1);
    EZ_TEST_BOOL(
      vMulFloatVec3.IsEqual(ezVec3Type((Type)-8.0, (Type)0.4, (Type)-14.0), ezMath::SmallEpsilon<Type>()));
    vMulFloatVec3 = ((Type)0 * vOp1);
    EZ_TEST_BOOL(vMulFloatVec3.IsEqual(ezVec3Type::MakeZero(), ezMath::SmallEpsilon<Type>()));

    // operator* (ezVec3Type, float)
    ezVec3Type vMulVec3Float = (vOp1 * (Type)2);
    EZ_TEST_BOOL(vMulVec3Float.IsEqual(ezVec3Type((Type)-8.0, (Type)0.4, (Type)-14.0), ezMath::SmallEpsilon<Type>()));
    vMulVec3Float = (vOp1 * (Type)0);
    EZ_TEST_BOOL(vMulVec3Float.IsEqual(ezVec3Type::MakeZero(), ezMath::SmallEpsilon<Type>()));

    // operator/ (ezVec3Type, float)
    ezVec3Type vDivVec3Float = (vOp1 / (Type)2);
    EZ_TEST_BOOL(vDivVec3Float.IsEqual(ezVec3Type((Type)-2.0, (Type)0.1, (Type)-3.5), ezMath::SmallEpsilon<Type>()));

    // operator== (ezVec3Type, ezVec3Type)
    EZ_TEST_BOOL(vOp1 == vOp1);
    for (int i = 0; i < 3; ++i)
    {
      EZ_TEST_BOOL(!(vOp1 == (vOp1 + (Type)ezMath::SmallEpsilon<Type>() * compArray[i])));
      EZ_TEST_BOOL(!(vOp1 == (vOp1 - (Type)ezMath::SmallEpsilon<Type>() * compArray[i])));
    }

    // operator!= (ezVec3Type, ezVec3Type)
    EZ_TEST_BOOL(!(vOp1 != vOp1));
    for (int i = 0; i < 3; ++i)
    {
      EZ_TEST_BOOL(vOp1 != (vOp1 + (Type)ezMath::SmallEpsilon<Type>() * compArray[i]));
      EZ_TEST_BOOL(vOp1 != (vOp1 - (Type)ezMath::SmallEpsilon<Type>() * compArray[i]));
    }

    // operator< (ezVec3Type, ezVec3Type)
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
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
    const ezVec3Type vOp1((Type)-4.0, (Type)0.2, (Type)-7.0);
    const ezVec3Type vOp2((Type)2.0, (Type)-0.3, (Type)0.5);

    const ezVec3Type compArray[3] = {ezVec3Type((Type)1.0, (Type)0.0, (Type)0.0), ezVec3Type((Type)0.0, (Type)1.0, (Type)0.0), ezVec3Type((Type)0.0, (Type)0.0, (Type)1.0)};

    // GetAngleBetween
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        EZ_TEST_FLOAT(compArray[i].GetAngleBetween(compArray[j]).GetDegree(), i == j ? (Type)0.0 : (Type)90.0, ezMath::DefaultEpsilon<float>());
      }
    }

    // Dot
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        EZ_TEST_FLOAT(compArray[i].Dot(compArray[j]), i == j ? (Type)1.0 : (Type)0.0, ezMath::SmallEpsilon<Type>());
      }
    }
    EZ_TEST_FLOAT(vOp1.Dot(vOp2), (Type)-11.56, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(vOp2.Dot(vOp1), (Type)-11.56, ezMath::SmallEpsilon<Type>());

    // Cross
    // Right-handed coordinate system check
    EZ_TEST_BOOL(compArray[0].CrossRH(compArray[1]).IsEqual(compArray[2], ezMath::SmallEpsilon<Type>()));
    EZ_TEST_BOOL(compArray[1].CrossRH(compArray[2]).IsEqual(compArray[0], ezMath::SmallEpsilon<Type>()));
    EZ_TEST_BOOL(compArray[2].CrossRH(compArray[0]).IsEqual(compArray[1], ezMath::SmallEpsilon<Type>()));

    // CompMin
    EZ_TEST_BOOL(vOp1.CompMin(vOp2).IsEqual(ezVec3Type((Type)-4.0, (Type)-0.3, (Type)-7.0), ezMath::SmallEpsilon<Type>()));
    EZ_TEST_BOOL(vOp2.CompMin(vOp1).IsEqual(ezVec3Type((Type)-4.0, (Type)-0.3, (Type)-7.0), ezMath::SmallEpsilon<Type>()));

    // CompMax
    EZ_TEST_BOOL(vOp1.CompMax(vOp2).IsEqual(ezVec3Type((Type)2.0, (Type)0.2, (Type)0.5), ezMath::SmallEpsilon<Type>()));
    EZ_TEST_BOOL(vOp2.CompMax(vOp1).IsEqual(ezVec3Type((Type)2.0, (Type)0.2, (Type)0.5), ezMath::SmallEpsilon<Type>()));

    // CompClamp
    EZ_TEST_BOOL(vOp1.CompClamp(vOp1, vOp2).IsEqual(ezVec3Type((Type)-4.0, (Type)-0.3, (Type)-7.0), ezMath::SmallEpsilon<Type>()));
    EZ_TEST_BOOL(vOp2.CompClamp(vOp1, vOp2).IsEqual(ezVec3Type((Type)2.0, (Type)0.2, (Type)0.5), ezMath::SmallEpsilon<Type>()));

    // CompMul
    EZ_TEST_BOOL(vOp1.CompMul(vOp2).IsEqual(ezVec3Type((Type)-8.0, (Type)-0.06, (Type)-3.5), ezMath::SmallEpsilon<Type>()));
    EZ_TEST_BOOL(vOp2.CompMul(vOp1).IsEqual(ezVec3Type((Type)-8.0, (Type)-0.06, (Type)-3.5), ezMath::SmallEpsilon<Type>()));

    // CompDiv
    EZ_TEST_BOOL(vOp1.CompDiv(vOp2).IsEqual(ezVec3Type((Type)-2.0, (Type)(-2.0/3.0), (Type)-14.0), ezMath::SmallEpsilon<Type>()));

    // Abs
    EZ_TEST_VEC3(vOp1.Abs(), ezVec3Type((Type)4.0, (Type)0.2, (Type)7.0), ezMath::SmallEpsilon<Type>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetAngleBetween")
  {
    ezVec3Type v;

    v.Set(1, 0, 0);
    EZ_TEST_FLOAT(v.GetAngleBetween(ezVec3Type(1, 0, 0), ezVec3Type(0, 0, 1)).GetDegree(), 0.0f, 0.001f);

    v.Set(1, -1, 0);
    v.Normalize();
    EZ_TEST_FLOAT(v.GetAngleBetween(ezVec3Type(1, 0, 0), ezVec3Type(0, 0, 1)).GetDegree(), 45.0f, 0.001f);

    v.Set(1, 1, 0);
    v.Normalize();
    EZ_TEST_FLOAT(v.GetAngleBetween(ezVec3Type(1, 0, 0), ezVec3Type(0, 0, 1)).GetDegree(), -45.0f, 0.001f);

    v.Set(-1, 0, 0);
    v.Normalize();
    EZ_TEST_FLOAT(v.GetAngleBetween(ezVec3Type(1, 0, 0), ezVec3Type(0, 0, 1)).GetDegree(), 180.0f, 0.001f);

    v.Set(1, 0, 0);
    v.Normalize();
    EZ_TEST_FLOAT(v.GetAngleBetween(ezVec3Type(0, 1, 0), ezVec3Type(0, 0, 1)).GetDegree(), 90.0f, 0.001f);

    v.Set(0, 1, 0);
    v.Normalize();
    EZ_TEST_FLOAT(v.GetAngleBetween(ezVec3Type(1, 0, 0), ezVec3Type(0, 0, 1)).GetDegree(), -90.0f, 0.001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CalculateNormal")
  {
    ezVec3Type n;
    EZ_TEST_BOOL(n.CalculateNormal(ezVec3Type(-1, 0, 1), ezVec3Type(1, 0, 1), ezVec3Type(0, 0, -1)) == EZ_SUCCESS);
    EZ_TEST_VEC3(n, ezVec3Type(0, 1, 0), 0.001f);

    EZ_TEST_BOOL(n.CalculateNormal(ezVec3Type(-1, 0, -1), ezVec3Type(1, 0, -1), ezVec3Type(0, 0, 1)) == EZ_SUCCESS);
    EZ_TEST_VEC3(n, ezVec3Type(0, -1, 0), 0.001f);

    EZ_TEST_BOOL(n.CalculateNormal(ezVec3Type(-1, 0, 1), ezVec3Type(1, 0, 1), ezVec3Type(1, 0, 1)) == EZ_FAILURE);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeOrthogonalTo")
  {
    ezVec3Type v;

    v.Set(1, 1, 0);
    v.MakeOrthogonalTo(ezVec3Type(1, 0, 0));
    EZ_TEST_VEC3(v, ezVec3Type(0, 1, 0), 0.001f);

    v.Set(1, 1, 0);
    v.MakeOrthogonalTo(ezVec3Type(0, 1, 0));
    EZ_TEST_VEC3(v, ezVec3Type(1, 0, 0), 0.001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetOrthogonalVector")
  {
    ezVec3Type v;

    for (Type i = 1; i < 360; i += 3.0f)
    {
      v.Set(i, i * 3, i * 7);
      EZ_TEST_FLOAT(v.GetOrthogonalVector().Dot(v), 0.0f, 0.001f);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetReflectedVector")
  {
    ezVec3Type v, v2;

    v.Set(1, 1, 0);
    v2 = v.GetReflectedVector(ezVec3Type(0, -1, 0));
    EZ_TEST_VEC3(v2, ezVec3Type(1, -1, 0), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeRandomPointInSphere")
  {
    ezVec3Type v;

    ezRandom rng;
    rng.Initialize(0xEEFF0011AABBCCDDULL);

    ezVec3Type avg;
    avg.SetZero();

    const ezUInt32 uiNumSamples = 100'000;
    for (ezUInt32 i = 0; i < uiNumSamples; ++i)
    {
      v = ezVec3Type::MakeRandomPointInSphere(rng);

      EZ_TEST_BOOL(v.GetLength() <= (Type)1.0 + ezMath::SmallEpsilon<Type>());
      EZ_TEST_BOOL(!v.IsZero());

      avg += v;
    }

    avg /= (float)uiNumSamples;

    // the average point cloud center should be within at least 10% of the sphere's center
    // otherwise the points aren't equally distributed
    EZ_TEST_BOOL(avg.IsZero((Type)0.1));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeRandomDirection")
  {
    ezVec3Type v;

    ezRandom rng;
    rng.InitializeFromCurrentTime();

    ezVec3Type avg;
    avg.SetZero();

    const ezUInt32 uiNumSamples = 100'000;
    for (ezUInt32 i = 0; i < uiNumSamples; ++i)
    {
      v = ezVec3Type::MakeRandomDirection(rng);

      EZ_TEST_BOOL(v.IsNormalized());

      avg += v;
    }

    avg /= (float)uiNumSamples;

    // the average point cloud center should be within at least 10% of the sphere's center
    // otherwise the points aren't equally distributed
    EZ_TEST_BOOL(avg.IsZero((Type)0.1));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeRandomDeviationX")
  {
    ezVec3Type v;
    ezVec3Type avg;
    avg.SetZero();

    ezRandom rng;
    rng.InitializeFromCurrentTime();

    const ezAngleTemplate<Type> dev = ezAngleTemplate<Type>::MakeFromDegree(65);
    const ezUInt32 uiNumSamples = 100'000;
    const ezVec3Type vAxis(1, 0, 0);

    for (ezUInt32 i = 0; i < uiNumSamples; ++i)
    {
      v = ezVec3Type::MakeRandomDeviationX(rng, dev);

      EZ_TEST_BOOL(v.IsNormalized());

      EZ_TEST_BOOL(vAxis.GetAngleBetween(v).GetRadian() <= dev.GetRadian() + ezMath::DefaultEpsilon<float>());

      avg += v;
    }

    // average direction should be close to the main axis
    avg.Normalize();
    EZ_TEST_BOOL(avg.IsEqual(vAxis, (Type)0.1));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeRandomDeviationY")
  {
    ezVec3Type v;
    ezVec3Type avg;
    avg.SetZero();

    ezRandom rng;
    rng.InitializeFromCurrentTime();

    const ezAngleTemplate<Type> dev = ezAngleTemplate<Type>::MakeFromDegree(65);
    const ezUInt32 uiNumSamples = 100'000;
    const ezVec3Type vAxis(0, 1, 0);

    for (ezUInt32 i = 0; i < uiNumSamples; ++i)
    {
      v = ezVec3Type::MakeRandomDeviationY(rng, dev);

      EZ_TEST_BOOL(v.IsNormalized());

      EZ_TEST_BOOL(vAxis.GetAngleBetween(v).GetRadian() <= dev.GetRadian() + ezMath::DefaultEpsilon<float>());

      avg += v;
    }

    // average direction should be close to the main axis
    avg.Normalize();
    EZ_TEST_BOOL(avg.IsEqual(vAxis, (Type)0.1));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeRandomDeviationZ")
  {
    ezVec3Type v;
    ezVec3Type avg;
    avg.SetZero();

    ezRandom rng;
    rng.InitializeFromCurrentTime();

    const ezAngleTemplate<Type> dev = ezAngleTemplate<Type>::MakeFromDegree(65);
    const ezUInt32 uiNumSamples = 100'000;
    const ezVec3Type vAxis(0, 0, 1);

    for (ezUInt32 i = 0; i < uiNumSamples; ++i)
    {
      v = ezVec3Type::MakeRandomDeviationZ(rng, dev);

      EZ_TEST_BOOL(v.IsNormalized());

      EZ_TEST_BOOL(vAxis.GetAngleBetween(v).GetRadian() <= dev.GetRadian() + ezMath::DefaultEpsilon<float>());

      avg += v;
    }

    // average direction should be close to the main axis
    avg.Normalize();
    EZ_TEST_BOOL(avg.IsEqual(vAxis, (Type)0.1));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeRandomDeviation")
  {
    ezVec3Type v;

    ezRandom rng;
    rng.InitializeFromCurrentTime();

    const ezAngleTemplate<Type> dev = ezAngleTemplate<Type>::MakeFromDegree(65);
    const ezUInt32 uiNumSamples = 100'000;
    ezVec3Type vAxis;

    for (ezUInt32 i = 0; i < uiNumSamples; ++i)
    {
      vAxis = ezVec3Type::MakeRandomDirection(rng);

      v = ezVec3Type::MakeRandomDeviation(rng, dev, vAxis);

      EZ_TEST_BOOL(v.IsNormalized());

      EZ_TEST_BOOL(vAxis.GetAngleBetween(v).GetDegree() <= dev.GetDegree() + (Type)1.0);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeRandomDeviation")
  {
    EZ_TEST_VEC3(ezVec3Type::MakeOrthogonalVector(ezVec3Type(1, 0, 0)), ezVec3Type(0, 0, 1), 0.001f);
    EZ_TEST_VEC3(ezVec3Type::MakeOrthogonalVector(ezVec3Type(0, 1, 0)), ezVec3Type(0, 0, -1), 0.001f);
    EZ_TEST_VEC3(ezVec3Type::MakeOrthogonalVector(ezVec3Type(0, 0, 1)), ezVec3Type(-1, 0, 0), 0.001f);

    ezVec3Type dir;

    dir.Set(1, 2, 3);
    dir.Normalize();
    EZ_TEST_FLOAT(dir.Dot(ezVec3Type::MakeOrthogonalVector(dir)), 0.0f, 0.001f);

    dir.Set(1, 0, -2);
    dir.Normalize();
    EZ_TEST_FLOAT(dir.Dot(ezVec3Type::MakeOrthogonalVector(dir)), 0.0f, 0.001f);

    dir.Set(-1, 2, 0);
    dir.Normalize();
    EZ_TEST_FLOAT(dir.Dot(ezVec3Type::MakeOrthogonalVector(dir)), 0.0f, 0.001f);
  }
}


EZ_CREATE_SIMPLE_TEST(Math, Vec3f)
{
  TestVec3<float>();
}
EZ_CREATE_SIMPLE_TEST(Math, Vec3d)
{
  TestVec3<double>();
}
