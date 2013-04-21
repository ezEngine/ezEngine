#include <PCH.h>

EZ_CREATE_SIMPLE_TEST(Math, Vec2)
{
  EZ_TEST_BLOCK(true, "Constructor")
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    // In debug the default constructor initializes everything with NaN.
    ezVec2 vDefCtor;
    EZ_TEST(ezMath::IsNaN(vDefCtor.x) && ezMath::IsNaN(vDefCtor.y));
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    float testBlock[2] = { 1.0f, 2.0f };
    ezVec2* pDefCtor = ::new ((void*)&testBlock[0]) ezVec2;
    EZ_TEST(pDefCtor->x == 1.0f && pDefCtor->y == 2.0f);
#endif
  }

  EZ_TEST_BLOCK(true, "Constructor(x,y)")
  {
    ezVec2 v(1, 2);
    EZ_TEST_FLOAT(v.x, 1, 0);
    EZ_TEST_FLOAT(v.y, 2, 0);
  }

  EZ_TEST_BLOCK(true, "Constructor(xy)")
  {
    ezVec2 v(3.0f);
    EZ_TEST_VEC2(v, ezVec2(3.0f, 3.0f), 0);
  }

  EZ_TEST_BLOCK(true, "ZeroVector")
  {
    EZ_TEST_VEC2(ezVec2::ZeroVector(), ezVec2(0.0f, 0.0f), 0);
  }

  EZ_TEST_BLOCK(true, "GetAsVec3")
  {
    EZ_TEST_VEC3(ezVec2(2.0f, 3.0f).GetAsVec3(4.0f), ezVec3(2.0f, 3.0f, 4.0f), 0);
  }

  EZ_TEST_BLOCK(true, "GetAsVec4")
  {
    EZ_TEST_VEC4(ezVec2(2.0f, 3.0f).GetAsVec4(4.0f, 5.0f), ezVec4(2.0f, 3.0f, 4.0f, 5.0f), 0);
  }

  EZ_TEST_BLOCK(true, "Set(x, y)")
  {
    ezVec2 v;
    v.Set(2.0f, 3.0f);

    EZ_TEST_FLOAT(v.x, 2.0f, 0);
    EZ_TEST_FLOAT(v.y, 3.0f, 0);
  }

  EZ_TEST_BLOCK(true, "Set(xy)")
  {
    ezVec2 v;
    v.Set(4.0f);

    EZ_TEST_FLOAT(v.x, 4.0f, 0);
    EZ_TEST_FLOAT(v.y, 4.0f, 0);
  }

  EZ_TEST_BLOCK(true, "SetZero")
  {
    ezVec2 v;
    v.Set(4.0f);
    v.SetZero();

    EZ_TEST_FLOAT(v.x, 0.0f, 0);
    EZ_TEST_FLOAT(v.y, 0.0f, 0);
  }

  EZ_TEST_BLOCK(true, "GetLength")
  {
    ezVec2 v(0.0f);
    EZ_TEST_FLOAT(v.GetLength(), 0.0f, 0.0001f);

    v.Set(1.0f, 0.0f);
    EZ_TEST_FLOAT(v.GetLength(), 1.0f, 0.0001f);

    v.Set(0.0f, 1.0f);
    EZ_TEST_FLOAT(v.GetLength(), 1.0f, 0.0001f);

    v.Set(2.0f, 3.0f);
    EZ_TEST_FLOAT(v.GetLength(), ezMath::Sqrt(4.0f + 9.0f), 0.0001f);
  }

  EZ_TEST_BLOCK(true, "GetLengthSquared")
  {
    ezVec2 v(0.0f);
    EZ_TEST_FLOAT(v.GetLengthSquared(), 0.0f, 0.0001f);

    v.Set(1.0f, 0.0f);
    EZ_TEST_FLOAT(v.GetLengthSquared(), 1.0f, 0.0001f);

    v.Set(0.0f, 1.0f);
    EZ_TEST_FLOAT(v.GetLengthSquared(), 1.0f, 0.0001f);

    v.Set(2.0f, 3.0f);
    EZ_TEST_FLOAT(v.GetLengthSquared(), 4.0f + 9.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(true, "GetLengthAndNormalize")
  {
    ezVec2 v(0.5f, 0.0f);
    float l = v.GetLengthAndNormalize();
    EZ_TEST_FLOAT(l, 0.5f, 0.0001f);
    EZ_TEST_FLOAT(v.GetLength(), 1.0f, 0.0001f);

    v.Set(1.0f, 0.0f);
    l = v.GetLengthAndNormalize();
    EZ_TEST_FLOAT(l, 1.0f, 0.0001f);
    EZ_TEST_FLOAT(v.GetLength(), 1.0f, 0.0001f);

    v.Set(0.0f, 1.0f);
    l = v.GetLengthAndNormalize();
    EZ_TEST_FLOAT(l, 1.0f, 0.0001f);
    EZ_TEST_FLOAT(v.GetLength(), 1.0f, 0.0001f);

    v.Set(2.0f, 3.0f);
    l = v.GetLengthAndNormalize();
    EZ_TEST_FLOAT(l, ezMath::Sqrt(4.0f + 9.0f), 0.0001f);
    EZ_TEST_FLOAT(v.GetLength(), 1.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(true, "GetNormalized")
  {
    ezVec2 v;

    v.Set(10, 0);
    EZ_TEST_VEC2(v.GetNormalized(), ezVec2(1, 0), 0.0001f);

    v.Set(0, 10);
    EZ_TEST_VEC2(v.GetNormalized(), ezVec2(0, 1), 0.0001f);
  }

  EZ_TEST_BLOCK(true, "Normalize")
  {
    ezVec2 v;

    v.Set(10, 0);
    v.Normalize();
    EZ_TEST_VEC2(v, ezVec2(1, 0), 0.0001f);

    v.Set(0, 10);
    v.Normalize();
    EZ_TEST_VEC2(v, ezVec2(0, 1), 0.0001f);
  }

  EZ_TEST_BLOCK(true, "NormalizeIfNotZero")
  {
    ezVec2 v;

    v.Set(10, 0);
    EZ_TEST(v.NormalizeIfNotZero() == EZ_SUCCESS);
    EZ_TEST_VEC2(v, ezVec2(1, 0), 0.0001f);

    v.Set(0, 10);
    EZ_TEST(v.NormalizeIfNotZero() == EZ_SUCCESS);
    EZ_TEST_VEC2(v, ezVec2(0, 1), 0.0001f);

    v.SetZero();
    EZ_TEST(v.NormalizeIfNotZero() == EZ_FAILURE);
    EZ_TEST_VEC2(v, ezVec2(1, 0), 0.0001f);
  }

  EZ_TEST_BLOCK(true, "IsZero")
  {
    ezVec2 v;
    
    v.Set(1.0f);
    EZ_TEST(v.IsZero() == false);

    v.Set(0.0001f);
    EZ_TEST(v.IsZero() == false);
    EZ_TEST(v.IsZero(0.001f) == true);
  }

  EZ_TEST_BLOCK(true, "IsNormalized")
  {
    ezVec2 v;

    v.SetZero();
    EZ_TEST(v.IsNormalized() == false);

    v.Set(1, 0);
    EZ_TEST(v.IsNormalized() == true);

    v.Set(0, 1);
    EZ_TEST(v.IsNormalized() == true);

    v.Set(0.1f, 1);
    EZ_TEST(v.IsNormalized() == false);
  }

  EZ_TEST_BLOCK(true, "IsNaN")
  {
    ezVec2 v(0.0f);

    EZ_TEST(!v.IsNaN());

    v.x = ezMath::NaN();
    EZ_TEST(v.IsNaN());
  }

  EZ_TEST_BLOCK(true, "IsValid")
  {
    ezVec2 v(0.0f);

    EZ_TEST(v.IsValid());

    v.x = ezMath::NaN();
    EZ_TEST(!v.IsValid());
  }

  EZ_TEST_BLOCK(true, "operator-")
  {
    ezVec2 v(1.0f);

    EZ_TEST_VEC2(-v, ezVec2(-1.0f), 0.0001f);
  }

  EZ_TEST_BLOCK(true, "operator+=")
  {
    ezVec2 v(1.0f, 2.0f);

    v += ezVec2(3.0f, 4.0f);
    EZ_TEST_VEC2(v, ezVec2(4.0f, 6.0f), 0.0001f);
  }

  EZ_TEST_BLOCK(true, "operator-=")
  {
    ezVec2 v(1.0f, 2.0f);

    v -= ezVec2(3.0f, 5.0f);
    EZ_TEST_VEC2(v, ezVec2(-2.0f, -3.0f), 0.0001f);
  }

  EZ_TEST_BLOCK(true, "operator*=(float)")
  {
    ezVec2 v(1.0f, 2.0f);

    v *= 3.0f;
    EZ_TEST_VEC2(v, ezVec2(3.0f, 6.0f), 0.0001f);
  }

  EZ_TEST_BLOCK(true, "operator/=(float)")
  {
    ezVec2 v(1.0f, 2.0f);

    v /= 2.0f;
    EZ_TEST_VEC2(v, ezVec2(0.5f, 1.0f), 0.0001f);
  }

  EZ_TEST_BLOCK(true, "IsIdentical")
  {
    ezVec2 v1(1.0f, 2.0f);
    ezVec2 v2 = v1;

    EZ_TEST(v1.IsIdentical(v2));

    v2.x += 0.00001f;
    EZ_TEST(!v1.IsIdentical(v2));
  }

  EZ_TEST_BLOCK(true, "IsEqual")
  {
    ezVec2 v1(1.0f, 2.0f);
    ezVec2 v2 = v1;

    EZ_TEST(v1.IsEqual(v2, 0.00001f));

    v2.x += 0.00001f;
    EZ_TEST(!v1.IsEqual(v2, 0.000001f));
    EZ_TEST(v1.IsEqual(v2, 0.0001f));
  }

  EZ_TEST_BLOCK(true, "GetAngleBetween")
  {
    ezVec2 v1(1.0f, 0.0f);
    ezVec2 v2(0.0f, 1.0f);

    EZ_TEST_FLOAT(v1.GetAngleBetween(v1),  0.0f, 0.001f);
    EZ_TEST_FLOAT(v2.GetAngleBetween(v2),  0.0f, 0.001f);
    EZ_TEST_FLOAT(v1.GetAngleBetween(v2),   90.0f, 0.001f);
    EZ_TEST_FLOAT(v1.GetAngleBetween(-v1), 180.0f, 0.001f);
  }

  EZ_TEST_BLOCK(true, "Dot")
  {
    ezVec2 v1(1.0f, 0.0f);
    ezVec2 v2(0.0f, 1.0f);
    ezVec2 v0(0.0f, 0.0f);

    EZ_TEST_FLOAT(v0.Dot(v0),   0.0f, 0.001f);
    EZ_TEST_FLOAT(v1.Dot(v1),   1.0f, 0.001f);
    EZ_TEST_FLOAT(v2.Dot(v2),   1.0f, 0.001f);
    EZ_TEST_FLOAT(v1.Dot(v2),   0.0f, 0.001f);
    EZ_TEST_FLOAT(v1.Dot(-v1), -1.0f, 0.001f);
  }

  EZ_TEST_BLOCK(true, "CompMin")
  {
    ezVec2 v1(2, 3);
    ezVec2 v2 = v1.CompMin(ezVec2(1, 4));
    EZ_TEST_VEC2(v2, ezVec2(1, 3), 0);

    v2 = v1.CompMin(ezVec2(3, 1));
    EZ_TEST_VEC2(v2, ezVec2(2, 1), 0);
  }

  EZ_TEST_BLOCK(true, "CompMax")
  {
    ezVec2 v1(2, 3.5f);
    ezVec2 v2 = v1.CompMax(ezVec2(1, 4));
    EZ_TEST_VEC2(v2, ezVec2(2, 4), 0);

    v2 = v1.CompMax(ezVec2(3, 1));
    EZ_TEST_VEC2(v2, ezVec2(3, 3.5f), 0);
  }

  EZ_TEST_BLOCK(true, "CompMult")
  {
    ezVec2 v1(2, 3);
    ezVec2 v2 = v1.CompMult(ezVec2(2, 4));
    EZ_TEST_VEC2(v2, ezVec2(4, 12), 0);

    v2 = v1.CompMult(ezVec2(3, 7));
    EZ_TEST_VEC2(v2, ezVec2(6, 21), 0);
  }

  EZ_TEST_BLOCK(true, "CompDiv")
  {
    ezVec2 v1(12, 32);
    ezVec2 v2 = v1.CompDiv(ezVec2(3, 4));
    EZ_TEST_VEC2(v2, ezVec2(4, 8), 0);
  }

  EZ_TEST_BLOCK(true, "MakeOrthogonalTo")
  {
    ezVec2 v;
    
    v.Set(1, 1);
    v.MakeOrthogonalTo(ezVec2(1, 0));
    EZ_TEST_VEC2(v, ezVec2(0, 1), 0.001f);

    v.Set(1, 1);
    v.MakeOrthogonalTo(ezVec2(0, 1));
    EZ_TEST_VEC2(v, ezVec2(1, 0), 0.001f);
  }

  EZ_TEST_BLOCK(true, "GetOrthogonalVector")
  {
    ezVec2 v;
    
    v.Set(1, 0);
    EZ_TEST_VEC2(v.GetOrthogonalVector(), ezVec2(0, 1), 0.001f);

    v.Set(0, 1);
    EZ_TEST_VEC2(v.GetOrthogonalVector(), ezVec2(-1, 0), 0.001f);
  }

  EZ_TEST_BLOCK(true, "GetReflectedVector")
  {
    ezVec2 v, v2;

    v.Set(1, 1);
    v2 = v.GetReflectedVector(ezVec2(0, -1));
    EZ_TEST_VEC2(v2, ezVec2(1, -1), 0.0001f);
  }

  EZ_TEST_BLOCK(true, "operator+")
  {
    ezVec2 v = ezVec2(1, 2) + ezVec2(3, 4);
    EZ_TEST_VEC2(v, ezVec2(4, 6), 0.0001f);
  }

  EZ_TEST_BLOCK(true, "operator-")
  {
    ezVec2 v = ezVec2(1, 2) - ezVec2(3, 5);
    EZ_TEST_VEC2(v, ezVec2(-2, -3), 0.0001f);
  }

  EZ_TEST_BLOCK(true, "operator* (vec, float) | operator* (float, vec)")
  {
    ezVec2 v = ezVec2(1, 2) * 3;
    EZ_TEST_VEC2(v, ezVec2(3, 6), 0.0001f);

    v = 7 * ezVec2(1, 2);
    EZ_TEST_VEC2(v, ezVec2(7, 14), 0.0001f);
  }

  EZ_TEST_BLOCK(true, "operator/ (vec, float)")
  {
    ezVec2 v = ezVec2(2, 4) / 2;
    EZ_TEST_VEC2(v, ezVec2(1, 2), 0.0001f);
  }

  EZ_TEST_BLOCK(true, "operator== | operator!=")
  {
    ezVec2 v1(1.0f, 2.0f);
    ezVec2 v2 = v1;

    EZ_TEST(v1 == v2);

    v2.x += 0.00001f;
    EZ_TEST(v1 != v2);
  }
}