#include <TestFramework/Framework/TestFramework.h>
#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/Mat4.h>

EZ_CREATE_SIMPLE_TEST(Math, BoundingBox)
{
  EZ_TEST_BLOCK(true, "Constructor(SetElements)")
  {
    ezBoundingBox b(ezVec3 (-1,-2,-3), ezVec3(1, 2, 3));

    EZ_TEST(b.m_vMin == ezVec3(-1,-2,-3));
    EZ_TEST(b.m_vMax == ezVec3( 1, 2, 3));
  }

  EZ_TEST_BLOCK(true, "SetElements")
  {
    ezBoundingBox b;
    b.SetElements(ezVec3 (-1,-2,-3), ezVec3(1, 2, 3));

    EZ_TEST(b.m_vMin == ezVec3(-1,-2,-3));
    EZ_TEST(b.m_vMax == ezVec3( 1, 2, 3));
  }

  EZ_TEST_BLOCK(true, "SetFromPoints")
  {
    ezVec3 p[6] =
    {
      ezVec3(-4, 0, 0),
      ezVec3(5, 0, 0),
      ezVec3(0, -6, 0),
      ezVec3(0, 7, 0),
      ezVec3(0, 0, -8),
      ezVec3(0, 0, 9),
    };

    ezBoundingBox b;
    b.SetFromPoints(p, 6);

    EZ_TEST(b.m_vMin == ezVec3(-4, -6, -8));
    EZ_TEST(b.m_vMax == ezVec3( 5,  7,  9));
  }

  EZ_TEST_BLOCK(true, "SetInvalid")
  {
    ezBoundingBox b;
    b.SetInvalid();

    EZ_TEST(!b.IsValid());
  }

  EZ_TEST_BLOCK(true, "SetCenterAndHalfExtents")
  {
    ezBoundingBox b;
    b.SetCenterAndHalfExtents(ezVec3(1,2,3), ezVec3(4,5,6));

    EZ_TEST(b.m_vMin == ezVec3(-3,-3,-3));
    EZ_TEST(b.m_vMax == ezVec3( 5, 7, 9));
  }

  EZ_TEST_BLOCK(true, "GetCorners")
  {
    ezBoundingBox b;
    b.SetElements(ezVec3(-1,-2,-3), ezVec3(1,2,3));

    ezVec3 c[8];
    b.GetCorners(c);

    EZ_TEST(c[0] == ezVec3(-1, -2, -3));
    EZ_TEST(c[1] == ezVec3(-1, -2,  3));
    EZ_TEST(c[2] == ezVec3(-1,  2, -3));
    EZ_TEST(c[3] == ezVec3(-1,  2,  3));
    EZ_TEST(c[4] == ezVec3( 1, -2, -3));
    EZ_TEST(c[5] == ezVec3( 1, -2,  3));
    EZ_TEST(c[6] == ezVec3( 1,  2, -3));
    EZ_TEST(c[7] == ezVec3( 1,  2,  3));
  }

  EZ_TEST_BLOCK(true, "ExpandToInclue (Point)")
  {
    ezBoundingBox b;
    b.SetInvalid();
    b.ExpandToInclude(ezVec3(1,2,3));

    EZ_TEST(b.m_vMin == ezVec3(1,2,3));
    EZ_TEST(b.m_vMax == ezVec3(1,2,3));


    b.ExpandToInclude(ezVec3(2,3,4));

    EZ_TEST(b.m_vMin == ezVec3(1,2,3));
    EZ_TEST(b.m_vMax == ezVec3(2,3,4));

    b.ExpandToInclude(ezVec3(0,1,2));

    EZ_TEST(b.m_vMin == ezVec3(0,1,2));
    EZ_TEST(b.m_vMax == ezVec3(2,3,4));
  }

  EZ_TEST_BLOCK(true, "ExpandToInclude (Box)")
  {
    ezBoundingBox b1, b2;

    b1.SetElements(ezVec3(-1,-2,-3), ezVec3(1,2,3));
    b2.SetElements(ezVec3(0), ezVec3(4,5,6));

    b1.ExpandToInclude(b2);

    EZ_TEST(b1.m_vMin == ezVec3(-1,-2,-3));
    EZ_TEST(b1.m_vMax == ezVec3( 4, 5, 6));

    b2.SetElements(ezVec3(-4,-5,-6), ezVec3(0));

    b1.ExpandToInclude(b2);

    EZ_TEST(b1.m_vMin == ezVec3(-4,-5,-6));
    EZ_TEST(b1.m_vMax == ezVec3( 4, 5, 6));
  }

  EZ_TEST_BLOCK(true, "ExpandToInclude (array)")
  {
    ezVec3 v[4] = 
    {
      ezVec3(1, 1, 1),
      ezVec3(-1, -1, -1),
      ezVec3(2, 2, 2),
      ezVec3(4, 4, 4)
    };

    ezBoundingBox b;
    b.SetInvalid();
    b.ExpandToInclude(v, 2, sizeof(ezVec3) * 2);

    EZ_TEST(b.m_vMin == ezVec3(1, 1, 1));
    EZ_TEST(b.m_vMax == ezVec3(2, 2, 2));

    b.ExpandToInclude(v, 4, sizeof(ezVec3));

    EZ_TEST(b.m_vMin == ezVec3(-1,-1,-1));
    EZ_TEST(b.m_vMax == ezVec3( 4, 4, 4));
  }

  EZ_TEST_BLOCK(true, "ExpandToCube")
  {
    ezBoundingBox b;
    b.SetCenterAndHalfExtents(ezVec3(1,2,3), ezVec3(4,5,6));

    b.ExpandToCube();

    EZ_TEST(b.GetCenter() == ezVec3(1,2,3));
    EZ_TEST(b.GetHalfExtents() == ezVec3(6,6,6));
  }

  EZ_TEST_BLOCK(true, "Grow")
  {
    ezBoundingBox b(ezVec3(1,2,3), ezVec3(4,5,6));
    b.Grow(ezVec3(2,4,6));

    EZ_TEST(b.m_vMin == ezVec3(-1,-2,-3));
    EZ_TEST(b.m_vMax == ezVec3( 6, 9,12));
  }

  EZ_TEST_BLOCK(true, "Contains (Point)")
  {
    ezBoundingBox b(ezVec3(0), ezVec3(0));

    EZ_TEST(b.Contains(ezVec3(0)));
    EZ_TEST(!b.Contains(ezVec3( 1, 0, 0)));
    EZ_TEST(!b.Contains(ezVec3(-1, 0, 0)));
  }

  EZ_TEST_BLOCK(true, "Contains (Box)")
  {
    ezBoundingBox b1(ezVec3(-3), ezVec3(3));
    ezBoundingBox b2(ezVec3(-1), ezVec3(1));
    ezBoundingBox b3(ezVec3(-1), ezVec3(4));

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
    ezBoundingBox b(ezVec3(1), ezVec3(5));

    ezVec3 v[4] =
    {
      ezVec3(0),
      ezVec3(1),
      ezVec3(5),
      ezVec3(6)
    };

    EZ_TEST(!b.Contains(&v[0], 4, sizeof(ezVec3)));
    EZ_TEST( b.Contains(&v[1], 2, sizeof(ezVec3)));
    EZ_TEST( b.Contains(&v[2], 1, sizeof(ezVec3)));

    EZ_TEST(!b.Contains(&v[1], 2, sizeof(ezVec3) * 2));
  }

  EZ_TEST_BLOCK(true, "Contains (Sphere)")
  {
    ezBoundingBox b(ezVec3(1), ezVec3(5));
    
    EZ_TEST(b.Contains(ezBoundingSphere(ezVec3(3), 2.0f)));
    EZ_TEST(!b.Contains(ezBoundingSphere(ezVec3(3), 2.1f)));
    EZ_TEST(!b.Contains(ezBoundingSphere(ezVec3(8), 2.0f)));
  }

  EZ_TEST_BLOCK(true, "Overlaps (box)")
  {
    ezBoundingBox b1(ezVec3(-3), ezVec3(3));
    ezBoundingBox b2(ezVec3(-1), ezVec3(1));
    ezBoundingBox b3(ezVec3(1), ezVec3(4));
    ezBoundingBox b4(ezVec3(-4, 1, 1), ezVec3(4, 2, 2));

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
    ezBoundingBox b(ezVec3(1), ezVec3(5));

    ezVec3 v[4] =
    {
      ezVec3(0),
      ezVec3(1),
      ezVec3(5),
      ezVec3(6)
    };

    EZ_TEST(!b.Overlaps(&v[0], 1, sizeof(ezVec3)));
    EZ_TEST(!b.Overlaps(&v[3], 1, sizeof(ezVec3)));

    EZ_TEST( b.Overlaps(&v[0], 4, sizeof(ezVec3)));
    EZ_TEST( b.Overlaps(&v[1], 2, sizeof(ezVec3)));
    EZ_TEST( b.Overlaps(&v[2], 1, sizeof(ezVec3)));

    EZ_TEST( b.Overlaps(&v[1], 2, sizeof(ezVec3) * 2));
  }

  EZ_TEST_BLOCK(true, "Overlaps (Sphere)")
  {
    ezBoundingBox b(ezVec3(1), ezVec3(5));

    EZ_TEST(b.Overlaps(ezBoundingSphere(ezVec3(3), 2.0f)));
    EZ_TEST(b.Overlaps(ezBoundingSphere(ezVec3(3), 2.1f)));
    EZ_TEST(!b.Overlaps(ezBoundingSphere(ezVec3(8), 2.0f)));
  }

  EZ_TEST_BLOCK(true, "IsIdentical, ==, !=")
  {
    ezBoundingBox b1, b2, b3;

    b1.SetElements(ezVec3(1), ezVec3(2));
    b2.SetElements(ezVec3(1), ezVec3(2));
    b3.SetElements(ezVec3(1), ezVec3(2.01f));

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
    ezBoundingBox b1, b2;
    b1.SetElements(ezVec3(-1), ezVec3(1));
    b2.SetElements(ezVec3(-1), ezVec3(2));

    EZ_TEST(!b1.IsEqual(b2));
    EZ_TEST(!b1.IsEqual(b2, 0.5f));
    EZ_TEST(b1.IsEqual(b2, 1.0f));
    EZ_TEST(b1.IsEqual(b2, 2.0f));
  }

  EZ_TEST_BLOCK(true, "GetCenter")
  {
    ezBoundingBox b(ezVec3(3), ezVec3(7));

    EZ_TEST(b.GetCenter() == ezVec3(5));
  }

  EZ_TEST_BLOCK(true, "GetExtents")
  {
    ezBoundingBox b(ezVec3(3), ezVec3(7));

    EZ_TEST(b.GetExtents() == ezVec3(4));
  }

  EZ_TEST_BLOCK(true, "GetHalfExtents")
  {
    ezBoundingBox b(ezVec3(3), ezVec3(7));

    EZ_TEST(b.GetHalfExtents() == ezVec3(2));
  }

  EZ_TEST_BLOCK(true, "Translate")
  {
    ezBoundingBox b(ezVec3(3), ezVec3(5));

    b.Translate(ezVec3(1,2,3));

    EZ_TEST(b.m_vMin == ezVec3(4,5,6));
    EZ_TEST(b.m_vMax == ezVec3(6,7,8));
  }

  EZ_TEST_BLOCK(true, "ScaleFromCenter")
  {
    ezBoundingBox b(ezVec3(3), ezVec3(5));

    b.ScaleFromCenter(ezVec3(1,2,3));

    EZ_TEST(b.m_vMin == ezVec3(3,2,1));
    EZ_TEST(b.m_vMax == ezVec3(5,6,7));
  }

  EZ_TEST_BLOCK(true, "ScaleFromOrigin")
  {
    ezBoundingBox b(ezVec3(3), ezVec3(5));

    b.ScaleFromOrigin(ezVec3(1,2,3));

    EZ_TEST(b.m_vMin == ezVec3(3,6,9));
    EZ_TEST(b.m_vMax == ezVec3(5,10,15));
  }

  EZ_TEST_BLOCK(true, "TransformFromOrigin")
  {
    ezBoundingBox b(ezVec3(3), ezVec3(5));

    ezMat4 m;
    m.SetScalingMatrix(ezVec3(2.0f));

    b.TransformFromOrigin(m);

    EZ_TEST(b.m_vMin == ezVec3(6,6,6));
    EZ_TEST(b.m_vMax == ezVec3(10,10,10));
  }

  EZ_TEST_BLOCK(true, "TransformFromCenter")
  {
    ezBoundingBox b(ezVec3(3), ezVec3(5));

    ezMat4 m;
    m.SetScalingMatrix(ezVec3(2.0f));

    b.TransformFromCenter(m);

    EZ_TEST(b.m_vMin == ezVec3(2,2,2));
    EZ_TEST(b.m_vMax == ezVec3(6,6,6));
  }

  EZ_TEST_BLOCK(true, "GetClampedPoint")
  {
    ezBoundingBox b(ezVec3(-1,-2,-3), ezVec3(1,2,3));

    EZ_TEST(b.GetClampedPoint(ezVec3(-2,0,0)) == ezVec3(-1,0,0));
    EZ_TEST(b.GetClampedPoint(ezVec3( 2,0,0)) == ezVec3( 1,0,0));

    EZ_TEST(b.GetClampedPoint(ezVec3(0,-3,0)) == ezVec3(0,-2,0));
    EZ_TEST(b.GetClampedPoint(ezVec3(0, 3,0)) == ezVec3(0, 2,0));

    EZ_TEST(b.GetClampedPoint(ezVec3(0,0,-4)) == ezVec3(0,0,-3));
    EZ_TEST(b.GetClampedPoint(ezVec3(0,0, 4)) == ezVec3(0,0, 3));
  }

  EZ_TEST_BLOCK(true, "GetDistanceTo (point)")
  {
    ezBoundingBox b(ezVec3(-1,-2,-3), ezVec3(1,2,3));

    EZ_TEST(b.GetDistanceTo(ezVec3(-2,0,0)) == 1);
    EZ_TEST(b.GetDistanceTo(ezVec3( 2,0,0)) == 1);

    EZ_TEST(b.GetDistanceTo(ezVec3(0,-4,0)) == 2);
    EZ_TEST(b.GetDistanceTo(ezVec3(0, 4,0)) == 2);

    EZ_TEST(b.GetDistanceTo(ezVec3(0,0,-6)) == 3);
    EZ_TEST(b.GetDistanceTo(ezVec3(0,0, 6)) == 3);
  }

  EZ_TEST_BLOCK(true, "GetDistanceTo (Sphere)")
  {
    ezBoundingBox b(ezVec3(1), ezVec3(5));

    EZ_TEST(b.GetDistanceTo(ezBoundingSphere(ezVec3(3), 2.0f)) < 0.0f);
    EZ_TEST(b.GetDistanceTo(ezBoundingSphere(ezVec3(5), 1.0f)) < 0.0f);
    EZ_TEST_FLOAT(b.GetDistanceTo(ezBoundingSphere(ezVec3(8, 2, 2), 2.0f)), 1.0f, 0.001f);
  }

  EZ_TEST_BLOCK(true, "GetDistanceTo (box)")
  {
    ezBoundingBox b(ezVec3(1), ezVec3(5));

    ezBoundingBox b1, b2, b3;
    b1.SetCenterAndHalfExtents(ezVec3(3), ezVec3(2.0f));
    b2.SetCenterAndHalfExtents(ezVec3(5), ezVec3(1.0f));
    b3.SetCenterAndHalfExtents(ezVec3(9,2,2), ezVec3(2.0f));

    EZ_TEST(b.GetDistanceTo(b1) <= 0.0f);
    EZ_TEST(b.GetDistanceTo(b2) <= 0.0f);
    EZ_TEST_FLOAT(b.GetDistanceTo(b3), 2.0f, 0.001f);
  }

  EZ_TEST_BLOCK(true, "GetDistanceSquaredTo (point)")
  {
    ezBoundingBox b(ezVec3(-1,-2,-3), ezVec3(1,2,3));

    EZ_TEST(b.GetDistanceSquaredTo(ezVec3(-2,0,0)) == 1);
    EZ_TEST(b.GetDistanceSquaredTo(ezVec3( 2,0,0)) == 1);

    EZ_TEST(b.GetDistanceSquaredTo(ezVec3(0,-4,0)) == 4);
    EZ_TEST(b.GetDistanceSquaredTo(ezVec3(0, 4,0)) == 4);

    EZ_TEST(b.GetDistanceSquaredTo(ezVec3(0,0,-6)) == 9);
    EZ_TEST(b.GetDistanceSquaredTo(ezVec3(0,0, 6)) == 9);
  }

  EZ_TEST_BLOCK(true, "GetDistanceSquaredTo (box)")
  {
    ezBoundingBox b(ezVec3(1), ezVec3(5));

    ezBoundingBox b1, b2, b3;
    b1.SetCenterAndHalfExtents(ezVec3(3), ezVec3(2.0f));
    b2.SetCenterAndHalfExtents(ezVec3(5), ezVec3(1.0f));
    b3.SetCenterAndHalfExtents(ezVec3(9,2,2), ezVec3(2.0f));

    EZ_TEST(b.GetDistanceSquaredTo(b1) <= 0.0f);
    EZ_TEST(b.GetDistanceSquaredTo(b2) <= 0.0f);
    EZ_TEST_FLOAT(b.GetDistanceSquaredTo(b3), 4.0f, 0.001f);
  }

  EZ_TEST_BLOCK(true, "GetBoundingSphere")
  {
    ezBoundingBox b;
    b.SetCenterAndHalfExtents(ezVec3(5, 4, 2), ezVec3(3.0f));

    ezBoundingSphere s = b.GetBoundingSphere();

    EZ_TEST(s.m_vCenter == ezVec3(5, 4, 2));
    EZ_TEST_FLOAT(s.m_fRadius, ezVec3(3.0f).GetLength(), 0.001f);
  }

  EZ_TEST_BLOCK(true, "GetRayIntersection")
  {
    const ezVec3 c = ezVec3(10);

    ezBoundingBox b;
    b.SetCenterAndHalfExtents(c, ezVec3(2, 4, 8));

    for (float x = b.m_vMin.x - 1.0f; x < b.m_vMax.x + 1.0f; x += 0.2f)
    {
      for (float y = b.m_vMin.y - 1.0f; y < b.m_vMax.y + 1.0f; y += 0.2f)
      {
        for (float z = b.m_vMin.z - 1.0f; z < b.m_vMax.z + 1.0f; z += 0.2f)
        {
          const ezVec3 v (x, y, z);

          if (b.Contains(v))
            continue;

          const ezVec3 vTarget = b.GetClampedPoint(v);

          const ezVec3 vDir = (vTarget - c).GetNormalized();

          const ezVec3 vSource = vTarget + vDir * 3.0f;

          float f;
          ezVec3 vi;
          EZ_TEST(b.GetRayIntersection(vSource, -vDir, &f, &vi) == true);
          EZ_TEST_FLOAT(f, 3.0f, 0.001f);
          EZ_TEST(vi.IsEqual(vTarget, 0.0001f));

          EZ_TEST(b.GetRayIntersection(vSource, vDir, &f, &vi) == false);
          EZ_TEST(b.GetRayIntersection(vTarget, vDir, &f, &vi) == false);
        }
      }
    }
  }

  EZ_TEST_BLOCK(true, "GetLineSegmentIntersection")
  {
    const ezVec3 c = ezVec3(10);

    ezBoundingBox b;
    b.SetCenterAndHalfExtents(c, ezVec3(2, 4, 8));

    for (float x = b.m_vMin.x - 1.0f; x < b.m_vMax.x + 1.0f; x += 0.2f)
    {
      for (float y = b.m_vMin.y - 1.0f; y < b.m_vMax.y + 1.0f; y += 0.2f)
      {
        for (float z = b.m_vMin.z - 1.0f; z < b.m_vMax.z + 1.0f; z += 0.2f)
        {
          const ezVec3 v (x, y, z);

          if (b.Contains(v))
            continue;

          const ezVec3 vTarget0 = b.GetClampedPoint(v);

          const ezVec3 vDir = (vTarget0 - c).GetNormalized();

          const ezVec3 vTarget = vTarget0 - vDir * 1.0f;
          const ezVec3 vSource = vTarget0  + vDir * 3.0f;

          float f;
          ezVec3 vi;
          EZ_TEST(b.GetLineSegmentIntersection(vSource, vTarget, &f, &vi) == true);
          EZ_TEST_FLOAT(f, 0.75f, 0.001f);
          EZ_TEST(vi.IsEqual(vTarget0, 0.0001f));
        }
      }
    }
  }
}