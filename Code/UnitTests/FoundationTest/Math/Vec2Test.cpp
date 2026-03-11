#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec4.h>

#include <Foundation/Math/FixedPoint.h>

template <typename Type>
void TestVec2()
{
  using ezVec2Type = ezVec2Template<Type>;
  using ezVec3Type = ezVec3Template<Type>;
  using ezVec4Type = ezVec4Template<Type>;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    if (ezMath::SupportsNaN<Type>())
    {
      // In debug the default constructor initializes everything with NaN.
      ezVec2Type vDefCtor;
      EZ_TEST_BOOL(ezMath::IsNaN(vDefCtor.x) && ezMath::IsNaN(vDefCtor.y));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    Type testBlock[2] = {(Type)1, (Type)2};
    ezVec2Type* pDefCtor = ::new ((void*)&testBlock[0]) ezVec2Type;
    EZ_TEST_BOOL(pDefCtor->x == (Type)1 && pDefCtor->y == (Type)2);
#endif
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor(x,y)")
  {
    ezVec2Type v(1, 2);
    EZ_TEST_FLOAT(v.x, 1, 0);
    EZ_TEST_FLOAT(v.y, 2, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor(xy)")
  {
    ezVec2Type v(3);
    EZ_TEST_VEC2(v, ezVec2Type(3, 3), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeZero")
  {
    EZ_TEST_VEC2(ezVec2Type::MakeZero(), ezVec2Type(0, 0), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetAsVec3")
  {
    EZ_TEST_VEC3(ezVec2Type(2, 3).GetAsVec3(4), ezVec3Type(2, 3, 4), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetAsVec4")
  {
    EZ_TEST_VEC4(ezVec2Type(2, 3).GetAsVec4(4, 5), ezVec4Type(2, 3, 4, 5), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Set(x, y)")
  {
    ezVec2Type v;
    v.Set(2, 3);

    EZ_TEST_FLOAT(v.x, 2, 0);
    EZ_TEST_FLOAT(v.y, 3, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Set(xy)")
  {
    ezVec2Type v;
    v.Set(4);

    EZ_TEST_FLOAT(v.x, 4, 0);
    EZ_TEST_FLOAT(v.y, 4, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetZero")
  {
    ezVec2Type v;
    v.Set(4);
    v.SetZero();

    EZ_TEST_FLOAT(v.x, 0, 0);
    EZ_TEST_FLOAT(v.y, 0, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetLength")
  {
    ezVec2Type v(0);
    EZ_TEST_FLOAT(v.GetLength(), 0, (Type)0.0001);

    v.Set(1, 0);
    EZ_TEST_FLOAT(v.GetLength(), 1, (Type)0.0001);

    v.Set(0, 1);
    EZ_TEST_FLOAT(v.GetLength(), 1, (Type)0.0001);

    v.Set(2, 3);
    EZ_TEST_FLOAT(v.GetLength(), ezMath::Sqrt((Type)(4 + 9)), (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetLengthSquared")
  {
    ezVec2Type v(0);
    EZ_TEST_FLOAT(v.GetLengthSquared(), 0, (Type)0.0001);

    v.Set(1, 0);
    EZ_TEST_FLOAT(v.GetLengthSquared(), 1, (Type)0.0001);

    v.Set(0, 1);
    EZ_TEST_FLOAT(v.GetLengthSquared(), 1, (Type)0.0001);

    v.Set(2, 3);
    EZ_TEST_FLOAT(v.GetLengthSquared(), 4 + 9, (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetLengthAndNormalize")
  {
    ezVec2Type v(0.5f, 0);
    Type l = v.GetLengthAndNormalize();
    EZ_TEST_FLOAT(l, 0.5f, (Type)0.0001);
    EZ_TEST_FLOAT(v.GetLength(), 1, (Type)0.0001);

    v.Set(1, 0);
    l = v.GetLengthAndNormalize();
    EZ_TEST_FLOAT(l, 1, (Type)0.0001);
    EZ_TEST_FLOAT(v.GetLength(), 1, (Type)0.0001);

    v.Set(0, 1);
    l = v.GetLengthAndNormalize();
    EZ_TEST_FLOAT(l, 1, (Type)0.0001);
    EZ_TEST_FLOAT(v.GetLength(), 1, (Type)0.0001);

    v.Set(2, 3);
    l = v.GetLengthAndNormalize();
    EZ_TEST_FLOAT(l, ezMath::Sqrt((Type)(4 + 9)), (Type)0.0001);
    EZ_TEST_FLOAT(v.GetLength(), 1, (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetNormalized")
  {
    ezVec2Type v;

    v.Set(10, 0);
    EZ_TEST_VEC2(v.GetNormalized(), ezVec2Type(1, 0), (Type)0.001);

    v.Set(0, 10);
    EZ_TEST_VEC2(v.GetNormalized(), ezVec2Type(0, 1), (Type)0.001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Normalize")
  {
    ezVec2Type v;

    v.Set(10, 0);
    v.Normalize();
    EZ_TEST_VEC2(v, ezVec2Type(1, 0), (Type)0.001);

    v.Set(0, 10);
    v.Normalize();
    EZ_TEST_VEC2(v, ezVec2Type(0, 1), (Type)0.001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "NormalizeIfNotZero")
  {
    ezVec2Type v;

    v.Set(10, 0);
    EZ_TEST_BOOL(v.NormalizeIfNotZero() == EZ_SUCCESS);
    EZ_TEST_VEC2(v, ezVec2Type(1, 0), (Type)0.001);

    v.Set(0, 10);
    EZ_TEST_BOOL(v.NormalizeIfNotZero() == EZ_SUCCESS);
    EZ_TEST_VEC2(v, ezVec2Type(0, 1), (Type)0.001);

    v.SetZero();
    EZ_TEST_BOOL(v.NormalizeIfNotZero() == EZ_FAILURE);
    EZ_TEST_VEC2(v, ezVec2Type(1, 0), (Type)0.001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsZero")
  {
    ezVec2Type v;

    v.Set(1);
    EZ_TEST_BOOL(v.IsZero() == false);

    v.Set(0.001f);
    EZ_TEST_BOOL(v.IsZero() == false);
    EZ_TEST_BOOL(v.IsZero(0.01f) == true);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsNormalized")
  {
    ezVec2Type v;

    v.SetZero();
    EZ_TEST_BOOL(v.IsNormalized(ezMath::HugeEpsilon<Type>()) == false);

    v.Set(1, 0);
    EZ_TEST_BOOL(v.IsNormalized(ezMath::HugeEpsilon<Type>()) == true);

    v.Set(0, 1);
    EZ_TEST_BOOL(v.IsNormalized(ezMath::HugeEpsilon<Type>()) == true);

    v.Set(0.1f, 1);
    EZ_TEST_BOOL(v.IsNormalized(ezMath::DefaultEpsilon<Type>()) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsNaN")
  {
    if (ezMath::SupportsNaN<Type>())
    {
      ezVec2Type v(0);

      EZ_TEST_BOOL(!v.IsNaN());

      v.x = ezMath::NaN<Type>();
      EZ_TEST_BOOL(v.IsNaN());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsValid")
  {
    if (ezMath::SupportsNaN<Type>())
    {
      ezVec2Type v(0);

      EZ_TEST_BOOL(v.IsValid());

      v.x = ezMath::NaN<Type>();
      EZ_TEST_BOOL(!v.IsValid());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator-")
  {
    ezVec2Type v(1);

    EZ_TEST_VEC2(-v, ezVec2Type(-1), (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator+=")
  {
    ezVec2Type v(1, 2);

    v += ezVec2Type(3, 4);
    EZ_TEST_VEC2(v, ezVec2Type(4, 6), (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator-=")
  {
    ezVec2Type v(1, 2);

    v -= ezVec2Type(3, 5);
    EZ_TEST_VEC2(v, ezVec2Type(-2, -3), (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*=(float)")
  {
    ezVec2Type v(1, 2);

    v *= 3;
    EZ_TEST_VEC2(v, ezVec2Type(3, 6), (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator/=(float)")
  {
    ezVec2Type v(1, 2);

    v /= 2;
    EZ_TEST_VEC2(v, ezVec2Type(0.5f, 1), (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsIdentical")
  {
    ezVec2Type v1(1, 2);
    ezVec2Type v2 = v1;

    EZ_TEST_BOOL(v1.IsIdentical(v2));

    v2.x += (Type)0.001;
    EZ_TEST_BOOL(!v1.IsIdentical(v2));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsEqual")
  {
    ezVec2Type v1(1, 2);
    ezVec2Type v2 = v1;

    EZ_TEST_BOOL(v1.IsEqual(v2, ezMath::DefaultEpsilon<Type>()));

    v2.x += (Type)0.001;
    EZ_TEST_BOOL(!v1.IsEqual(v2, (Type)0.0001));
    EZ_TEST_BOOL(v1.IsEqual(v2, (Type)0.01));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetAngleBetween")
  {
    ezVec2Type v1(1, 0);
    ezVec2Type v2(0, 1);

    EZ_TEST_FLOAT(v1.GetAngleBetween(v1).GetDegree(), 0, (Type)0.001);
    EZ_TEST_FLOAT(v2.GetAngleBetween(v2).GetDegree(), 0, (Type)0.001);
    EZ_TEST_FLOAT(v1.GetAngleBetween(v2).GetDegree(), 90, (Type)0.001);
    EZ_TEST_FLOAT(v1.GetAngleBetween(-v1).GetDegree(), 180, (Type)0.001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Dot")
  {
    ezVec2Type v1(1, 0);
    ezVec2Type v2(0, 1);
    ezVec2Type v0(0, 0);

    EZ_TEST_FLOAT(v0.Dot(v0), 0, (Type)0.001);
    EZ_TEST_FLOAT(v1.Dot(v1), 1, (Type)0.001);
    EZ_TEST_FLOAT(v2.Dot(v2), 1, (Type)0.001);
    EZ_TEST_FLOAT(v1.Dot(v2), 0, (Type)0.001);
    EZ_TEST_FLOAT(v1.Dot(-v1), -1, (Type)0.001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CompMin")
  {
    ezVec2Type v1(2, 3);
    ezVec2Type v2 = v1.CompMin(ezVec2Type(1, 4));
    EZ_TEST_VEC2(v2, ezVec2Type(1, 3), 0);

    v2 = v1.CompMin(ezVec2Type(3, 1));
    EZ_TEST_VEC2(v2, ezVec2Type(2, 1), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CompMax")
  {
    ezVec2Type v1(2, 3.5f);
    ezVec2Type v2 = v1.CompMax(ezVec2Type(1, 4));
    EZ_TEST_VEC2(v2, ezVec2Type(2, 4), 0);

    v2 = v1.CompMax(ezVec2Type(3, 1));
    EZ_TEST_VEC2(v2, ezVec2Type(3, 3.5f), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CompClamp")
  {
    const ezVec2Type vOp1(-4.0, 0.2f);
    const ezVec2Type vOp2(2.0, -0.3f);

    EZ_TEST_BOOL(vOp1.CompClamp(vOp1, vOp2).IsEqual(ezVec2Type(-4.0f, -0.3f), ezMath::SmallEpsilon<Type>()));
    EZ_TEST_BOOL(vOp2.CompClamp(vOp1, vOp2).IsEqual(ezVec2Type(2.0f, 0.2f), ezMath::SmallEpsilon<Type>()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CompMul")
  {
    ezVec2Type v1(2, 3);
    ezVec2Type v2 = v1.CompMul(ezVec2Type(2, 4));
    EZ_TEST_VEC2(v2, ezVec2Type(4, 12), 0);

    v2 = v1.CompMul(ezVec2Type(3, 7));
    EZ_TEST_VEC2(v2, ezVec2Type(6, 21), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CompDiv")
  {
    ezVec2Type v1(12, 32);
    ezVec2Type v2 = v1.CompDiv(ezVec2Type(3, 4));
    EZ_TEST_VEC2(v2, ezVec2Type(4, 8), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Abs")
  {
    ezVec2Type v1(-5, 7);
    ezVec2Type v2 = v1.Abs();
    EZ_TEST_VEC2(v2, ezVec2Type(5, 7), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeOrthogonalTo")
  {
    ezVec2Type v;

    v.Set(1, 1);
    v.MakeOrthogonalTo(ezVec2Type(1, 0));
    EZ_TEST_VEC2(v, ezVec2Type(0, 1), (Type)0.001);

    v.Set(1, 1);
    v.MakeOrthogonalTo(ezVec2Type(0, 1));
    EZ_TEST_VEC2(v, ezVec2Type(1, 0), (Type)0.001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetOrthogonalVector")
  {
    ezVec2Type v;

    for (Type i = 1; i < 360; i += 3)
    {
      v.Set(i, i * 3);
      EZ_TEST_FLOAT(v.GetOrthogonalVector().Dot(v), 0, (Type)0.001);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetReflectedVector")
  {
    ezVec2Type v, v2;

    v.Set(1, 1);
    v2 = v.GetReflectedVector(ezVec2Type(0, -1));
    EZ_TEST_VEC2(v2, ezVec2Type(1, -1), (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator+")
  {
    ezVec2Type v = ezVec2Type(1, 2) + ezVec2Type(3, 4);
    EZ_TEST_VEC2(v, ezVec2Type(4, 6), (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator-")
  {
    ezVec2Type v = ezVec2Type(1, 2) - ezVec2Type(3, 5);
    EZ_TEST_VEC2(v, ezVec2Type(-2, -3), (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator* (vec, float) | operator* (float, vec)")
  {
    ezVec2Type v = ezVec2Type(1, 2) * (Type)3;
    EZ_TEST_VEC2(v, ezVec2Type(3, 6), (Type)0.0001);

    v = (Type)7 * ezVec2Type(1, 2);
    EZ_TEST_VEC2(v, ezVec2Type(7, 14), (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator/ (vec, float)")
  {
    ezVec2Type v = ezVec2Type(2, 4) / (Type)2;
    EZ_TEST_VEC2(v, ezVec2Type(1, 2), (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator== | operator!=")
  {
    ezVec2Type v1(1, 2);
    ezVec2Type v2 = v1;

    EZ_TEST_BOOL(v1 == v2);

    v2.x += (Type)0.001;
    EZ_TEST_BOOL(v1 != v2);
  }
}


EZ_CREATE_SIMPLE_TEST(Math, Vec2f)
{
  TestVec2<float>();
}
EZ_CREATE_SIMPLE_TEST(Math, Vec2d)
{
  TestVec2<double>();
}
