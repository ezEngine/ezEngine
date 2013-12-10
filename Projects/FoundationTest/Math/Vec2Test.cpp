#include <PCH.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec4.h>

#include <Foundation/Math/FixedPoint.h>

EZ_CREATE_SIMPLE_TEST(Math, Vec2)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    if (ezMath::BasicType<ezMathTestType>::SupportsNaN())
    {
      // In debug the default constructor initializes everything with NaN.
      ezVec2T vDefCtor;
      EZ_TEST_BOOL(ezMath::IsNaN(vDefCtor.x) && ezMath::IsNaN(vDefCtor.y));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    ezVec2T::ComponentType testBlock[2] = { (ezVec2T::ComponentType) 1, (ezVec2T::ComponentType) 2 };
    ezVec2T* pDefCtor = ::new ((void*)&testBlock[0]) ezVec2T;
    EZ_TEST_BOOL(pDefCtor->x == (ezVec2T::ComponentType) 1 && pDefCtor->y == (ezVec2T::ComponentType) 2);
#endif
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor(x,y)")
  {
    ezVec2T v(1, 2);
    EZ_TEST_FLOAT(v.x, 1, 0);
    EZ_TEST_FLOAT(v.y, 2, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor(xy)")
  {
    ezVec2T v(3);
    EZ_TEST_VEC2(v, ezVec2T(3, 3), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ZeroVector")
  {
    EZ_TEST_VEC2(ezVec2T::ZeroVector(), ezVec2T(0, 0), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetAsVec3")
  {
    EZ_TEST_VEC3(ezVec2T(2, 3).GetAsVec3(4), ezVec3T(2, 3, 4), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetAsVec4")
  {
    EZ_TEST_VEC4(ezVec2T(2, 3).GetAsVec4(4, 5), ezVec4T(2, 3, 4, 5), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Set(x, y)")
  {
    ezVec2T v;
    v.Set(2, 3);

    EZ_TEST_FLOAT(v.x, 2, 0);
    EZ_TEST_FLOAT(v.y, 3, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Set(xy)")
  {
    ezVec2T v;
    v.Set(4);

    EZ_TEST_FLOAT(v.x, 4, 0);
    EZ_TEST_FLOAT(v.y, 4, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetZero")
  {
    ezVec2T v;
    v.Set(4);
    v.SetZero();

    EZ_TEST_FLOAT(v.x, 0, 0);
    EZ_TEST_FLOAT(v.y, 0, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetLength")
  {
    ezVec2T v(0);
    EZ_TEST_FLOAT(v.GetLength(), 0, 0.0001f);

    v.Set(1, 0);
    EZ_TEST_FLOAT(v.GetLength(), 1, 0.0001f);

    v.Set(0, 1);
    EZ_TEST_FLOAT(v.GetLength(), 1, 0.0001f);

    v.Set(2, 3);
    EZ_TEST_FLOAT(v.GetLength(), ezMath::Sqrt((ezMathTestType) (4 + 9)), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetLengthSquared")
  {
    ezVec2T v(0);
    EZ_TEST_FLOAT(v.GetLengthSquared(), 0, 0.0001f);

    v.Set(1, 0);
    EZ_TEST_FLOAT(v.GetLengthSquared(), 1, 0.0001f);

    v.Set(0, 1);
    EZ_TEST_FLOAT(v.GetLengthSquared(), 1, 0.0001f);

    v.Set(2, 3);
    EZ_TEST_FLOAT(v.GetLengthSquared(), 4 + 9, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetLengthAndNormalize")
  {
    ezVec2T v(0.5f, 0);
    ezVec2T::ComponentType l = v.GetLengthAndNormalize();
    EZ_TEST_FLOAT(l, 0.5f, 0.0001f);
    EZ_TEST_FLOAT(v.GetLength(), 1, 0.0001f);

    v.Set(1, 0);
    l = v.GetLengthAndNormalize();
    EZ_TEST_FLOAT(l, 1, 0.0001f);
    EZ_TEST_FLOAT(v.GetLength(), 1, 0.0001f);

    v.Set(0, 1);
    l = v.GetLengthAndNormalize();
    EZ_TEST_FLOAT(l, 1, 0.0001f);
    EZ_TEST_FLOAT(v.GetLength(), 1, 0.0001f);

    v.Set(2, 3);
    l = v.GetLengthAndNormalize();
    EZ_TEST_FLOAT(l, ezMath::Sqrt((ezMathTestType) (4 + 9)), 0.0001f);
    EZ_TEST_FLOAT(v.GetLength(), 1, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetNormalized")
  {
    ezVec2T v;

    v.Set(10, 0);
    EZ_TEST_VEC2(v.GetNormalized(), ezVec2T(1, 0), 0.001f);

    v.Set(0, 10);
    EZ_TEST_VEC2(v.GetNormalized(), ezVec2T(0, 1), 0.001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Normalize")
  {
    ezVec2T v;

    v.Set(10, 0);
    v.Normalize();
    EZ_TEST_VEC2(v, ezVec2T(1, 0), 0.001f);

    v.Set(0, 10);
    v.Normalize();
    EZ_TEST_VEC2(v, ezVec2T(0, 1), 0.001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "NormalizeIfNotZero")
  {
    ezVec2T v;

    v.Set(10, 0);
    EZ_TEST_BOOL(v.NormalizeIfNotZero() == EZ_SUCCESS);
    EZ_TEST_VEC2(v, ezVec2T(1, 0), 0.001f);

    v.Set(0, 10);
    EZ_TEST_BOOL(v.NormalizeIfNotZero() == EZ_SUCCESS);
    EZ_TEST_VEC2(v, ezVec2T(0, 1), 0.001f);

    v.SetZero();
    EZ_TEST_BOOL(v.NormalizeIfNotZero() == EZ_FAILURE);
    EZ_TEST_VEC2(v, ezVec2T(1, 0), 0.001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsZero")
  {
    ezVec2T v;
    
    v.Set(1);
    EZ_TEST_BOOL(v.IsZero() == false);

    v.Set(0.001f);
    EZ_TEST_BOOL(v.IsZero() == false);
    EZ_TEST_BOOL(v.IsZero(0.01f) == true);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsNormalized")
  {
    ezVec2T v;

    v.SetZero();
    EZ_TEST_BOOL(v.IsNormalized(ezMath::BasicType<ezMathTestType>::HugeEpsilon()) == false);

    v.Set(1, 0);
    EZ_TEST_BOOL(v.IsNormalized(ezMath::BasicType<ezMathTestType>::HugeEpsilon()) == true);

    v.Set(0, 1);
    EZ_TEST_BOOL(v.IsNormalized(ezMath::BasicType<ezMathTestType>::HugeEpsilon()) == true);

    v.Set(0.1f, 1);
    EZ_TEST_BOOL(v.IsNormalized(ezMath::BasicType<ezMathTestType>::DefaultEpsilon()) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsNaN")
  {
    if (ezMath::BasicType<ezMathTestType>::SupportsNaN())
    {
      ezVec2T v(0);

      EZ_TEST_BOOL(!v.IsNaN());

      v.x = ezMath::BasicType<ezMathTestType>::GetNaN();
      EZ_TEST_BOOL(v.IsNaN());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsValid")
  {
    if (ezMath::BasicType<ezMathTestType>::SupportsNaN())
    {
      ezVec2T v(0);

      EZ_TEST_BOOL(v.IsValid());

      v.x = ezMath::BasicType<ezMathTestType>::GetNaN();
      EZ_TEST_BOOL(!v.IsValid());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator-")
  {
    ezVec2T v(1);

    EZ_TEST_VEC2(-v, ezVec2T(-1), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator+=")
  {
    ezVec2T v(1, 2);

    v += ezVec2T(3, 4);
    EZ_TEST_VEC2(v, ezVec2T(4, 6), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator-=")
  {
    ezVec2T v(1, 2);

    v -= ezVec2T(3, 5);
    EZ_TEST_VEC2(v, ezVec2T(-2, -3), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*=(float)")
  {
    ezVec2T v(1, 2);

    v *= 3;
    EZ_TEST_VEC2(v, ezVec2T(3, 6), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator/=(float)")
  {
    ezVec2T v(1, 2);

    v /= 2;
    EZ_TEST_VEC2(v, ezVec2T(0.5f, 1), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsIdentical")
  {
    ezVec2T v1(1, 2);
    ezVec2T v2 = v1;

    EZ_TEST_BOOL(v1.IsIdentical(v2));

    v2.x += ezVec2T::ComponentType(0.001f);
    EZ_TEST_BOOL(!v1.IsIdentical(v2));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsEqual")
  {
    ezVec2T v1(1, 2);
    ezVec2T v2 = v1;

    EZ_TEST_BOOL(v1.IsEqual(v2, 0.00001f));

    v2.x += ezVec2T::ComponentType(0.001f);
    EZ_TEST_BOOL(!v1.IsEqual(v2, 0.0001f));
    EZ_TEST_BOOL(v1.IsEqual(v2, 0.01f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetAngleBetween")
  {
    ezVec2T v1(1, 0);
    ezVec2T v2(0, 1);

    EZ_TEST_FLOAT(v1.GetAngleBetween(v1).GetDegree(),  0, 0.001f);
    EZ_TEST_FLOAT(v2.GetAngleBetween(v2).GetDegree(),  0, 0.001f);
    EZ_TEST_FLOAT(v1.GetAngleBetween(v2).GetDegree(),   90, 0.001f);
    EZ_TEST_FLOAT(v1.GetAngleBetween(-v1).GetDegree(), 180, 0.001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Dot")
  {
    ezVec2T v1(1, 0);
    ezVec2T v2(0, 1);
    ezVec2T v0(0, 0);

    EZ_TEST_FLOAT(v0.Dot(v0),   0, 0.001f);
    EZ_TEST_FLOAT(v1.Dot(v1),   1, 0.001f);
    EZ_TEST_FLOAT(v2.Dot(v2),   1, 0.001f);
    EZ_TEST_FLOAT(v1.Dot(v2),   0, 0.001f);
    EZ_TEST_FLOAT(v1.Dot(-v1), -1, 0.001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CompMin")
  {
    ezVec2T v1(2, 3);
    ezVec2T v2 = v1.CompMin(ezVec2T(1, 4));
    EZ_TEST_VEC2(v2, ezVec2T(1, 3), 0);

    v2 = v1.CompMin(ezVec2T(3, 1));
    EZ_TEST_VEC2(v2, ezVec2T(2, 1), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CompMax")
  {
    ezVec2T v1(2, 3.5f);
    ezVec2T v2 = v1.CompMax(ezVec2T(1, 4));
    EZ_TEST_VEC2(v2, ezVec2T(2, 4), 0);

    v2 = v1.CompMax(ezVec2T(3, 1));
    EZ_TEST_VEC2(v2, ezVec2T(3, 3.5f), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CompMult")
  {
    ezVec2T v1(2, 3);
    ezVec2T v2 = v1.CompMult(ezVec2T(2, 4));
    EZ_TEST_VEC2(v2, ezVec2T(4, 12), 0);

    v2 = v1.CompMult(ezVec2T(3, 7));
    EZ_TEST_VEC2(v2, ezVec2T(6, 21), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CompDiv")
  {
    ezVec2T v1(12, 32);
    ezVec2T v2 = v1.CompDiv(ezVec2T(3, 4));
    EZ_TEST_VEC2(v2, ezVec2T(4, 8), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeOrthogonalTo")
  {
    ezVec2T v;
    
    v.Set(1, 1);
    v.MakeOrthogonalTo(ezVec2T(1, 0));
    EZ_TEST_VEC2(v, ezVec2T(0, 1), 0.001f);

    v.Set(1, 1);
    v.MakeOrthogonalTo(ezVec2T(0, 1));
    EZ_TEST_VEC2(v, ezVec2T(1, 0), 0.001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetOrthogonalVector")
  {
    ezVec2T v;
   
    for (float i = 1; i < 360; i += 3)
    {
      v.Set(i, i * 3);
      EZ_TEST_FLOAT(v.GetOrthogonalVector().Dot(v), 0, 0.001f);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetReflectedVector")
  {
    ezVec2T v, v2;

    v.Set(1, 1);
    v2 = v.GetReflectedVector(ezVec2T(0, -1));
    EZ_TEST_VEC2(v2, ezVec2T(1, -1), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator+")
  {
    ezVec2T v = ezVec2T(1, 2) + ezVec2T(3, 4);
    EZ_TEST_VEC2(v, ezVec2T(4, 6), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator-")
  {
    ezVec2T v = ezVec2T(1, 2) - ezVec2T(3, 5);
    EZ_TEST_VEC2(v, ezVec2T(-2, -3), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator* (vec, float) | operator* (float, vec)")
  {
    ezVec2T v = ezVec2T(1, 2) * ezVec2T::ComponentType(3);
    EZ_TEST_VEC2(v, ezVec2T(3, 6), 0.0001f);

    v = ezVec2T::ComponentType(7) * ezVec2T(1, 2);
    EZ_TEST_VEC2(v, ezVec2T(7, 14), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator/ (vec, float)")
  {
    ezVec2T v = ezVec2T(2, 4) / ezVec2T::ComponentType(2);
    EZ_TEST_VEC2(v, ezVec2T(1, 2), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator== | operator!=")
  {
    ezVec2T v1(1, 2);
    ezVec2T v2 = v1;

    EZ_TEST_BOOL(v1 == v2);

    v2.x += ezVec2T::ComponentType(0.001f);
    EZ_TEST_BOOL(v1 != v2);
  }
}