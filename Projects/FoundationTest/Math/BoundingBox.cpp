#include <PCH.h>
#include <Foundation/Math/BoundingBox.h>

EZ_CREATE_SIMPLE_TEST(Math, BoundingBox)
{
  EZ_TEST_BLOCK(true, "Constructor(SetElements)")
  {
    ezBoundingBoxT b(ezVec3T (-1,-2,-3), ezVec3T(1, 2, 3));

    EZ_TEST(b.m_vMin == ezVec3T(-1,-2,-3));
    EZ_TEST(b.m_vMax == ezVec3T( 1, 2, 3));
  }

  EZ_TEST_BLOCK(true, "SetElements")
  {
    ezBoundingBoxT b;
    b.SetElements(ezVec3T (-1,-2,-3), ezVec3T(1, 2, 3));

    EZ_TEST(b.m_vMin == ezVec3T(-1,-2,-3));
    EZ_TEST(b.m_vMax == ezVec3T( 1, 2, 3));
  }

  EZ_TEST_BLOCK(true, "SetFromPoints")
  {
    ezVec3T p[6] =
    {
      ezVec3T(-4, 0, 0),
      ezVec3T(5, 0, 0),
      ezVec3T(0, -6, 0),
      ezVec3T(0, 7, 0),
      ezVec3T(0, 0, -8),
      ezVec3T(0, 0, 9),
    };

    ezBoundingBoxT b;
    b.SetFromPoints(p, 6);

    EZ_TEST(b.m_vMin == ezVec3T(-4, -6, -8));
    EZ_TEST(b.m_vMax == ezVec3T( 5,  7,  9));
  }

  EZ_TEST_BLOCK(true, "SetInvalid")
  {
    ezBoundingBoxT b;
    b.SetInvalid();

    EZ_TEST(!b.IsValid());
  }

  EZ_TEST_BLOCK(true, "SetCenterAndHalfExtents")
  {
    ezBoundingBoxT b;
    b.SetCenterAndHalfExtents(ezVec3T(1,2,3), ezVec3T(4,5,6));

    EZ_TEST(b.m_vMin == ezVec3T(-3,-3,-3));
    EZ_TEST(b.m_vMax == ezVec3T( 5, 7, 9));
  }

  EZ_TEST_BLOCK(true, "GetCorners")
  {
    ezBoundingBoxT b;
    b.SetElements(ezVec3T(-1,-2,-3), ezVec3T(1,2,3));

    ezVec3T c[8];
    b.GetCorners(c);

    EZ_TEST(c[0] == ezVec3T(-1, -2, -3));
    EZ_TEST(c[1] == ezVec3T(-1, -2,  3));
    EZ_TEST(c[2] == ezVec3T(-1,  2, -3));
    EZ_TEST(c[3] == ezVec3T(-1,  2,  3));
    EZ_TEST(c[4] == ezVec3T( 1, -2, -3));
    EZ_TEST(c[5] == ezVec3T( 1, -2,  3));
    EZ_TEST(c[6] == ezVec3T( 1,  2, -3));
    EZ_TEST(c[7] == ezVec3T( 1,  2,  3));
  }

  EZ_TEST_BLOCK(true, "ExpandToInclue (Point)")
  {
    ezBoundingBoxT b;
    b.SetInvalid();
    b.ExpandToInclude(ezVec3T(1,2,3));

    EZ_TEST(b.m_vMin == ezVec3T(1,2,3));
    EZ_TEST(b.m_vMax == ezVec3T(1,2,3));


    b.ExpandToInclude(ezVec3T(2,3,4));

    EZ_TEST(b.m_vMin == ezVec3T(1,2,3));
    EZ_TEST(b.m_vMax == ezVec3T(2,3,4));

    b.ExpandToInclude(ezVec3T(0,1,2));

    EZ_TEST(b.m_vMin == ezVec3T(0,1,2));
    EZ_TEST(b.m_vMax == ezVec3T(2,3,4));
  }

  EZ_TEST_BLOCK(true, "ExpandToInclude (Box)")
  {
    ezBoundingBoxT b1, b2;

    b1.SetElements(ezVec3T(-1,-2,-3), ezVec3T(1,2,3));
    b2.SetElements(ezVec3T(0), ezVec3T(4,5,6));

    b1.ExpandToInclude(b2);

    EZ_TEST(b1.m_vMin == ezVec3T(-1,-2,-3));
    EZ_TEST(b1.m_vMax == ezVec3T( 4, 5, 6));

    b2.SetElements(ezVec3T(-4,-5,-6), ezVec3T(0));

    b1.ExpandToInclude(b2);

    EZ_TEST(b1.m_vMin == ezVec3T(-4,-5,-6));
    EZ_TEST(b1.m_vMax == ezVec3T( 4, 5, 6));
  }

  EZ_TEST_BLOCK(true, "ExpandToInclude (array)")
  {
    ezVec3T v[4] = 
    {
      ezVec3T(1, 1, 1),
      ezVec3T(-1, -1, -1),
      ezVec3T(2, 2, 2),
      ezVec3T(4, 4, 4)
    };

    ezBoundingBoxT b;
    b.SetInvalid();
    b.ExpandToInclude(v, 2, sizeof(ezVec3T) * 2);

    EZ_TEST(b.m_vMin == ezVec3T(1, 1, 1));
    EZ_TEST(b.m_vMax == ezVec3T(2, 2, 2));

    b.ExpandToInclude(v, 4, sizeof(ezVec3T));

    EZ_TEST(b.m_vMin == ezVec3T(-1,-1,-1));
    EZ_TEST(b.m_vMax == ezVec3T( 4, 4, 4));
  }

  EZ_TEST_BLOCK(true, "ExpandToCube")
  {
    ezBoundingBoxT b;
    b.SetCenterAndHalfExtents(ezVec3T(1,2,3), ezVec3T(4,5,6));

    b.ExpandToCube();

    EZ_TEST_VEC3(b.GetCenter(), ezVec3T(1,2,3), ezMath::BasicType<ezMathTestType>::DefaultEpsilon());
    EZ_TEST_VEC3(b.GetHalfExtents(), ezVec3T(6,6,6), ezMath::BasicType<ezMathTestType>::DefaultEpsilon());
  }

  EZ_TEST_BLOCK(true, "Grow")
  {
    ezBoundingBoxT b(ezVec3T(1,2,3), ezVec3T(4,5,6));
    b.Grow(ezVec3T(2,4,6));

    EZ_TEST(b.m_vMin == ezVec3T(-1,-2,-3));
    EZ_TEST(b.m_vMax == ezVec3T( 6, 9,12));
  }

  EZ_TEST_BLOCK(true, "Contains (Point)")
  {
    ezBoundingBoxT b(ezVec3T(0), ezVec3T(0));

    EZ_TEST(b.Contains(ezVec3T(0)));
    EZ_TEST(!b.Contains(ezVec3T( 1, 0, 0)));
    EZ_TEST(!b.Contains(ezVec3T(-1, 0, 0)));
  }

  EZ_TEST_BLOCK(true, "Contains (Box)")
  {
    ezBoundingBoxT b1(ezVec3T(-3), ezVec3T(3));
    ezBoundingBoxT b2(ezVec3T(-1), ezVec3T(1));
    ezBoundingBoxT b3(ezVec3T(-1), ezVec3T(4));

    EZ_TEST(b1.Contains(b1));
    EZ_TEST(b2.Contains(b2));
    EZ_TEST(b3.Contains(b3));

    EZ_TEST(b1.Contains(b2));
    EZ_TEST(!b1.Contains(b3));

    EZ_TEST(!b2.Contains(b1));
    EZ_TEST(!b2.Contains(b3));

    EZ_TEST(!b3.Contains(b1));
    EZ_TEST(b3.Contains(b2));
  }

  EZ_TEST_BLOCK(true, "Contains (Array)")
  {
    ezBoundingBoxT b(ezVec3T(1), ezVec3T(5));

    ezVec3T v[4] =
    {
      ezVec3T(0),
      ezVec3T(1),
      ezVec3T(5),
      ezVec3T(6)
    };

    EZ_TEST(!b.Contains(&v[0], 4, sizeof(ezVec3T)));
    EZ_TEST( b.Contains(&v[1], 2, sizeof(ezVec3T)));
    EZ_TEST( b.Contains(&v[2], 1, sizeof(ezVec3T)));

    EZ_TEST(!b.Contains(&v[1], 2, sizeof(ezVec3T) * 2));
  }

  EZ_TEST_BLOCK(true, "Contains (Sphere)")
  {
    ezBoundingBoxT b(ezVec3T(1), ezVec3T(5));
    
    EZ_TEST(b.Contains(ezBoundingSphereT(ezVec3T(3), 2)));
    EZ_TEST(!b.Contains(ezBoundingSphereT(ezVec3T(3), 2.1f)));
    EZ_TEST(!b.Contains(ezBoundingSphereT(ezVec3T(8), 2)));
  }

  EZ_TEST_BLOCK(true, "Overlaps (box)")
  {
    ezBoundingBoxT b1(ezVec3T(-3), ezVec3T(3));
    ezBoundingBoxT b2(ezVec3T(-1), ezVec3T(1));
    ezBoundingBoxT b3(ezVec3T(1), ezVec3T(4));
    ezBoundingBoxT b4(ezVec3T(-4, 1, 1), ezVec3T(4, 2, 2));

    EZ_TEST(b1.Overlaps(b1));
    EZ_TEST(b2.Overlaps(b2));
    EZ_TEST(b3.Overlaps(b3));
    EZ_TEST(b4.Overlaps(b4));

    EZ_TEST(b1.Overlaps(b2));
    EZ_TEST(b1.Overlaps(b3));
    EZ_TEST(b1.Overlaps(b4));

    EZ_TEST(!b2.Overlaps(b3));
    EZ_TEST(!b2.Overlaps(b4));

    EZ_TEST(b3.Overlaps(b4));
  }

  EZ_TEST_BLOCK(true, "Overlaps (Array)")
  {
    ezBoundingBoxT b(ezVec3T(1), ezVec3T(5));

    ezVec3T v[4] =
    {
      ezVec3T(0),
      ezVec3T(1),
      ezVec3T(5),
      ezVec3T(6)
    };

    EZ_TEST(!b.Overlaps(&v[0], 1, sizeof(ezVec3T)));
    EZ_TEST(!b.Overlaps(&v[3], 1, sizeof(ezVec3T)));

    EZ_TEST( b.Overlaps(&v[0], 4, sizeof(ezVec3T)));
    EZ_TEST( b.Overlaps(&v[1], 2, sizeof(ezVec3T)));
    EZ_TEST( b.Overlaps(&v[2], 1, sizeof(ezVec3T)));

    EZ_TEST( b.Overlaps(&v[1], 2, sizeof(ezVec3T) * 2));
  }

  EZ_TEST_BLOCK(true, "Overlaps (Sphere)")
  {
    ezBoundingBoxT b(ezVec3T(1), ezVec3T(5));

    EZ_TEST(b.Overlaps(ezBoundingSphereT(ezVec3T(3), 2)));
    EZ_TEST(b.Overlaps(ezBoundingSphereT(ezVec3T(3), 2.1f)));
    EZ_TEST(!b.Overlaps(ezBoundingSphereT(ezVec3T(8), 2)));
  }

  EZ_TEST_BLOCK(true, "IsIdentical, ==, !=")
  {
    ezBoundingBoxT b1, b2, b3;

    b1.SetElements(ezVec3T(1), ezVec3T(2));
    b2.SetElements(ezVec3T(1), ezVec3T(2));
    b3.SetElements(ezVec3T(1), ezVec3T(2.01f));

    EZ_TEST(b1.IsIdentical(b1));
    EZ_TEST(b2.IsIdentical(b2));
    EZ_TEST(b3.IsIdentical(b3));

    EZ_TEST(b1 == b1);
    EZ_TEST(b2 == b2);
    EZ_TEST(b3 == b3);

    EZ_TEST(b1.IsIdentical(b2));
    EZ_TEST(b2.IsIdentical(b1));

    EZ_TEST(!b1.IsIdentical(b3));
    EZ_TEST(!b2.IsIdentical(b3));
    EZ_TEST(!b3.IsIdentical(b1));
    EZ_TEST(!b3.IsIdentical(b1));

    EZ_TEST(b1 == b2);
    EZ_TEST(b2 == b1);

    EZ_TEST(b1 != b3);
    EZ_TEST(b2 != b3);
    EZ_TEST(b3 != b1);
    EZ_TEST(b3 != b1);
  }

  EZ_TEST_BLOCK(true, "IsEqual")
  {
    ezBoundingBoxT b1, b2;
    b1.SetElements(ezVec3T(-1), ezVec3T(1));
    b2.SetElements(ezVec3T(-1), ezVec3T(2));

    EZ_TEST(!b1.IsEqual(b2));
    EZ_TEST(!b1.IsEqual(b2, 0.5f));
    EZ_TEST(b1.IsEqual(b2, 1));
    EZ_TEST(b1.IsEqual(b2, 2));
  }

  EZ_TEST_BLOCK(true, "GetCenter")
  {
    ezBoundingBoxT b(ezVec3T(3), ezVec3T(7));

    EZ_TEST(b.GetCenter() == ezVec3T(5));
  }

  EZ_TEST_BLOCK(true, "GetExtents")
  {
    ezBoundingBoxT b(ezVec3T(3), ezVec3T(7));

    EZ_TEST(b.GetExtents() == ezVec3T(4));
  }

  EZ_TEST_BLOCK(true, "GetHalfExtents")
  {
    ezBoundingBoxT b(ezVec3T(3), ezVec3T(7));

    EZ_TEST(b.GetHalfExtents() == ezVec3T(2));
  }

  EZ_TEST_BLOCK(true, "Translate")
  {
    ezBoundingBoxT b(ezVec3T(3), ezVec3T(5));

    b.Translate(ezVec3T(1,2,3));

    EZ_TEST(b.m_vMin == ezVec3T(4,5,6));
    EZ_TEST(b.m_vMax == ezVec3T(6,7,8));
  }

  EZ_TEST_BLOCK(true, "ScaleFromCenter")
  {
    ezBoundingBoxT b(ezVec3T(3), ezVec3T(5));

    b.ScaleFromCenter(ezVec3T(1,2,3));

    EZ_TEST(b.m_vMin == ezVec3T(3,2,1));
    EZ_TEST(b.m_vMax == ezVec3T(5,6,7));
  }

  EZ_TEST_BLOCK(true, "ScaleFromOrigin")
  {
    ezBoundingBoxT b(ezVec3T(3), ezVec3T(5));

    b.ScaleFromOrigin(ezVec3T(1,2,3));

    EZ_TEST(b.m_vMin == ezVec3T(3,6,9));
    EZ_TEST(b.m_vMax == ezVec3T(5,10,15));
  }

  EZ_TEST_BLOCK(true, "TransformFromOrigin")
  {
    ezBoundingBoxT b(ezVec3T(3), ezVec3T(5));

    ezMat4T m;
    m.SetScalingMatrix(ezVec3T(2));

    b.TransformFromOrigin(m);

    EZ_TEST(b.m_vMin == ezVec3T(6,6,6));
    EZ_TEST(b.m_vMax == ezVec3T(10,10,10));
  }

  EZ_TEST_BLOCK(true, "TransformFromCenter")
  {
    ezBoundingBoxT b(ezVec3T(3), ezVec3T(5));

    ezMat4T m;
    m.SetScalingMatrix(ezVec3T(2));

    b.TransformFromCenter(m);

    EZ_TEST(b.m_vMin == ezVec3T(2,2,2));
    EZ_TEST(b.m_vMax == ezVec3T(6,6,6));
  }

  EZ_TEST_BLOCK(true, "GetClampedPoint")
  {
    ezBoundingBoxT b(ezVec3T(-1,-2,-3), ezVec3T(1,2,3));

    EZ_TEST(b.GetClampedPoint(ezVec3T(-2,0,0)) == ezVec3T(-1,0,0));
    EZ_TEST(b.GetClampedPoint(ezVec3T( 2,0,0)) == ezVec3T( 1,0,0));

    EZ_TEST(b.GetClampedPoint(ezVec3T(0,-3,0)) == ezVec3T(0,-2,0));
    EZ_TEST(b.GetClampedPoint(ezVec3T(0, 3,0)) == ezVec3T(0, 2,0));

    EZ_TEST(b.GetClampedPoint(ezVec3T(0,0,-4)) == ezVec3T(0,0,-3));
    EZ_TEST(b.GetClampedPoint(ezVec3T(0,0, 4)) == ezVec3T(0,0, 3));
  }

  EZ_TEST_BLOCK(true, "GetDistanceTo (point)")
  {
    ezBoundingBoxT b(ezVec3T(-1,-2,-3), ezVec3T(1,2,3));

    EZ_TEST(b.GetDistanceTo(ezVec3T(-2,0,0)) == 1);
    EZ_TEST(b.GetDistanceTo(ezVec3T( 2,0,0)) == 1);

    EZ_TEST(b.GetDistanceTo(ezVec3T(0,-4,0)) == 2);
    EZ_TEST(b.GetDistanceTo(ezVec3T(0, 4,0)) == 2);

    EZ_TEST(b.GetDistanceTo(ezVec3T(0,0,-6)) == 3);
    EZ_TEST(b.GetDistanceTo(ezVec3T(0,0, 6)) == 3);
  }

  EZ_TEST_BLOCK(true, "GetDistanceTo (Sphere)")
  {
    ezBoundingBoxT b(ezVec3T(1), ezVec3T(5));

    EZ_TEST(b.GetDistanceTo(ezBoundingSphereT(ezVec3T(3), 2)) < 0);
    EZ_TEST(b.GetDistanceTo(ezBoundingSphereT(ezVec3T(5), 1)) < 0);
    EZ_TEST_FLOAT(b.GetDistanceTo(ezBoundingSphereT(ezVec3T(8, 2, 2), 2)), 1, 0.001f);
  }

  EZ_TEST_BLOCK(true, "GetDistanceTo (box)")
  {
    ezBoundingBoxT b(ezVec3T(1), ezVec3T(5));

    ezBoundingBoxT b1, b2, b3;
    b1.SetCenterAndHalfExtents(ezVec3T(3), ezVec3T(2));
    b2.SetCenterAndHalfExtents(ezVec3T(5), ezVec3T(1));
    b3.SetCenterAndHalfExtents(ezVec3T(9,2,2), ezVec3T(2));

    EZ_TEST(b.GetDistanceTo(b1) <= 0);
    EZ_TEST(b.GetDistanceTo(b2) <= 0);
    EZ_TEST_FLOAT(b.GetDistanceTo(b3), 2, 0.001f);
  }

  EZ_TEST_BLOCK(true, "GetDistanceSquaredTo (point)")
  {
    ezBoundingBoxT b(ezVec3T(-1,-2,-3), ezVec3T(1,2,3));

    EZ_TEST(b.GetDistanceSquaredTo(ezVec3T(-2,0,0)) == 1);
    EZ_TEST(b.GetDistanceSquaredTo(ezVec3T( 2,0,0)) == 1);

    EZ_TEST(b.GetDistanceSquaredTo(ezVec3T(0,-4,0)) == 4);
    EZ_TEST(b.GetDistanceSquaredTo(ezVec3T(0, 4,0)) == 4);

    EZ_TEST(b.GetDistanceSquaredTo(ezVec3T(0,0,-6)) == 9);
    EZ_TEST(b.GetDistanceSquaredTo(ezVec3T(0,0, 6)) == 9);
  }

  EZ_TEST_BLOCK(true, "GetDistanceSquaredTo (box)")
  {
    ezBoundingBoxT b(ezVec3T(1), ezVec3T(5));

    ezBoundingBoxT b1, b2, b3;
    b1.SetCenterAndHalfExtents(ezVec3T(3), ezVec3T(2));
    b2.SetCenterAndHalfExtents(ezVec3T(5), ezVec3T(1));
    b3.SetCenterAndHalfExtents(ezVec3T(9,2,2), ezVec3T(2));

    EZ_TEST(b.GetDistanceSquaredTo(b1) <= 0);
    EZ_TEST(b.GetDistanceSquaredTo(b2) <= 0);
    EZ_TEST_FLOAT(b.GetDistanceSquaredTo(b3), 4, 0.001f);
  }

  EZ_TEST_BLOCK(true, "GetBoundingSphere")
  {
    ezBoundingBoxT b;
    b.SetCenterAndHalfExtents(ezVec3T(5, 4, 2), ezVec3T(3));

    ezBoundingSphereT s = b.GetBoundingSphere();

    EZ_TEST(s.m_vCenter == ezVec3T(5, 4, 2));
    EZ_TEST_FLOAT(s.m_fRadius, ezVec3T(3).GetLength(), 0.001f);
  }

  EZ_TEST_BLOCK(true, "GetRayIntersection")
  {
    if (ezMath::BasicType<ezMathTestType>::SupportsInfinity())
    {
      const ezVec3T c = ezVec3T(10);

      ezBoundingBoxT b;
      b.SetCenterAndHalfExtents(c, ezVec3T(2, 4, 8));

      for (ezMathTestType x = b.m_vMin.x - (ezMathTestType) 1; x < b.m_vMax.x + (ezMathTestType) 1; x += (ezMathTestType) 0.2f)
      {
        for (ezMathTestType y = b.m_vMin.y - (ezMathTestType) 1; y < b.m_vMax.y + (ezMathTestType) 1; y += (ezMathTestType) 0.2f)
        {
          for (ezMathTestType z = b.m_vMin.z - (ezMathTestType) 1; z < b.m_vMax.z + (ezMathTestType) 1; z += (ezMathTestType) 0.2f)
          {
            const ezVec3T v (x, y, z);

            if (b.Contains(v))
              continue;

            const ezVec3T vTarget = b.GetClampedPoint(v);

            const ezVec3T vDir = (vTarget - c).GetNormalized();

            const ezVec3T vSource = vTarget + vDir * (ezMathTestType) 3;

            ezMathTestType f;
            ezVec3T vi;
            EZ_TEST(b.GetRayIntersection(vSource, -vDir, &f, &vi) == true); // \todo FIX Division by Zero
            EZ_TEST_FLOAT(f, 3, 0.001f);
            EZ_TEST(vi.IsEqual(vTarget, 0.0001f));

            EZ_TEST(b.GetRayIntersection(vSource, vDir, &f, &vi) == false);
            EZ_TEST(b.GetRayIntersection(vTarget, vDir, &f, &vi) == false);
          }
        }
      }
    }
  }

  EZ_TEST_BLOCK(true, "GetLineSegmentIntersection")
  {
    if (ezMath::BasicType<ezMathTestType>::SupportsInfinity())
    {
      const ezVec3T c = ezVec3T(10);

      ezBoundingBoxT b;
      b.SetCenterAndHalfExtents(c, ezVec3T(2, 4, 8));

      for (ezMathTestType x = b.m_vMin.x - (ezMathTestType) 1; x < b.m_vMax.x + (ezMathTestType) 1; x += (ezMathTestType) 0.2f)
      {
        for (ezMathTestType y = b.m_vMin.y - (ezMathTestType) 1; y < b.m_vMax.y + (ezMathTestType) 1; y += (ezMathTestType) 0.2f)
        {
          for (ezMathTestType z = b.m_vMin.z - (ezMathTestType) 1; z < b.m_vMax.z + (ezMathTestType) 1; z += (ezMathTestType) 0.2f)
          {
            const ezVec3T v (x, y, z);

            if (b.Contains(v))
              continue;

            const ezVec3T vTarget0 = b.GetClampedPoint(v);

            const ezVec3T vDir = (vTarget0 - c).GetNormalized();

            const ezVec3T vTarget = vTarget0 - vDir * (ezMathTestType) 1;
            const ezVec3T vSource = vTarget0  + vDir * (ezMathTestType) 3;

            ezMathTestType f;
            ezVec3T vi;
            EZ_TEST(b.GetLineSegmentIntersection(vSource, vTarget, &f, &vi) == true);
            EZ_TEST_FLOAT(f, 0.75f, 0.001f);
            EZ_TEST(vi.IsEqual(vTarget0, 0.0001f));
          }
        }
      }
    }
  }
}