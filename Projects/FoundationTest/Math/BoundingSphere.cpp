#include <PCH.h>

EZ_CREATE_SIMPLE_TEST(Math, BoundingSphere)
{
  EZ_TEST_BLOCK(true, "Constructor")
  {
    ezBoundingSphere s(ezVec3(1,2,3), 4);

    EZ_TEST(s.m_vCenter == ezVec3(1,2,3));
    EZ_TEST(s.m_fRadius == 4.0f);
  }

  EZ_TEST_BLOCK(true, "SetInvalid / IsValid")
  {
    ezBoundingSphere s;
    s.SetElements(ezVec3(1,2,3), 4);

    EZ_TEST(s.IsValid());

    s.SetInvalid();

    EZ_TEST(!s.IsValid());
  }

  EZ_TEST_BLOCK(true, "SetZero / IsZero")
  {
    ezBoundingSphere s;
    s.SetZero();

    EZ_TEST(s.IsValid());
    EZ_TEST(s.m_vCenter.IsZero());
    EZ_TEST(s.m_fRadius == 0.0f);
    EZ_TEST(s.IsZero());
  }

  EZ_TEST_BLOCK(true, "SetElements")
  {
    ezBoundingSphere s;
    s.SetElements(ezVec3(1,2,3), 4);

    EZ_TEST(s.m_vCenter == ezVec3(1,2,3));
    EZ_TEST(s.m_fRadius == 4);
  }

  EZ_TEST_BLOCK(true, "SetFromPoints")
  {
    ezBoundingSphere s;

    ezVec3 p[4] =
    {
      ezVec3( 0, 2, 0),
      ezVec3( 4, 2, 0),
      ezVec3( 2, 0, 0),
      ezVec3( 2, 4, 0)
    };

    s.SetFromPoints(p, 4);

    EZ_TEST(s.m_vCenter == ezVec3(2, 2, 0));
    EZ_TEST(s.m_fRadius == 2);
  }

  EZ_TEST_BLOCK(true, "ExpandToInclude(Point)")
  {
    ezBoundingSphere s;
    s.SetZero();

    s.ExpandToInclude(ezVec3(3, 0, 0));

    EZ_TEST(s.m_vCenter == ezVec3(0, 0, 0));
    EZ_TEST(s.m_fRadius == 3);
  }

  EZ_TEST_BLOCK(true, "ExpandToInclude(array)")
  {
    ezBoundingSphere s;
    s.SetElements(ezVec3(2,2,0), 0.0f);

    ezVec3 p[4] =
    {
      ezVec3( 0, 2, 0),
      ezVec3( 4, 2, 0),
      ezVec3( 2, 0, 0),
      ezVec3( 2, 4, 0)
    };

    s.ExpandToInclude(p, 4);

    EZ_TEST(s.m_vCenter == ezVec3(2, 2, 0));
    EZ_TEST(s.m_fRadius == 2);
  }

  EZ_TEST_BLOCK(true, "ExpandToInclude (sphere)")
  {
    ezBoundingSphere s1, s2, s3;
    s1.SetElements(ezVec3(5, 0, 0), 1);
    s2.SetElements(ezVec3(6, 0, 0), 1);
    s3.SetElements(ezVec3(5, 0, 0), 2);

    s1.ExpandToInclude(s2);
    EZ_TEST(s1.m_vCenter == ezVec3(5, 0, 0));
    EZ_TEST(s1.m_fRadius == 2);

    s1.ExpandToInclude(s3);
    EZ_TEST(s1.m_vCenter == ezVec3(5, 0, 0));
    EZ_TEST(s1.m_fRadius == 2);
  }

  EZ_TEST_BLOCK(true, "ExpandToInclude (box)")
  {
    ezBoundingSphere s;
    s.SetElements(ezVec3(0, 0, 0), 1);

    ezBoundingBox b;
    b.SetCenterAndHalfExtents(ezVec3(0, 0, 0), ezVec3(2.0f));

    s.ExpandToInclude(b);

    EZ_TEST(s.m_vCenter == ezVec3(0));
    EZ_TEST(s.m_fRadius == ezMath::Sqrt(12));
  }

  EZ_TEST_BLOCK(true, "Grow")
  {
    ezBoundingSphere s;
    s.SetElements(ezVec3(1, 2, 3), 4);

    s.Grow(5);

    EZ_TEST(s.m_vCenter == ezVec3(1,2,3));
    EZ_TEST(s.m_fRadius == 9);
  }

  EZ_TEST_BLOCK(true, "IsIdentical, ==, !=")
  {
    ezBoundingSphere s1, s2, s3;

    s1.SetElements(ezVec3(1,2,3), 4);
    s2.SetElements(ezVec3(1,2,3), 4);
    s3.SetElements(ezVec3(1.001f,2.001f,3.001f), 4.001f);

    EZ_TEST(s1 == s1);
    EZ_TEST(s2 == s2);
    EZ_TEST(s3 == s3);

    EZ_TEST(s1 == s2);
    EZ_TEST(s2 == s1);

    EZ_TEST(s1 != s3);
    EZ_TEST(s2 != s3);
    EZ_TEST(s3 != s1);
    EZ_TEST(s3 != s2);
  }

  EZ_TEST_BLOCK(true, "IsEqual")
  {
    ezBoundingSphere s1, s2, s3;

    s1.SetElements(ezVec3(1,2,3), 4);
    s2.SetElements(ezVec3(1,2,3), 4);
    s3.SetElements(ezVec3(1.001f,2.001f,3.001f), 4.001f);

    EZ_TEST(s1.IsEqual(s1));
    EZ_TEST(s2.IsEqual(s2));
    EZ_TEST(s3.IsEqual(s3));

    EZ_TEST(s1.IsEqual(s2));
    EZ_TEST(s2.IsEqual(s1));

    EZ_TEST(!s1.IsEqual(s3, 0.0001f));
    EZ_TEST(!s2.IsEqual(s3, 0.0001f));
    EZ_TEST(!s3.IsEqual(s1, 0.0001f));
    EZ_TEST(!s3.IsEqual(s2, 0.0001f));

    EZ_TEST(s1.IsEqual(s3, 0.002f));
    EZ_TEST(s2.IsEqual(s3, 0.002f));
    EZ_TEST(s3.IsEqual(s1, 0.002f));
    EZ_TEST(s3.IsEqual(s2, 0.002f));
  }

  EZ_TEST_BLOCK(true, "Translate")
  {
    ezBoundingSphere s;
    s.SetElements(ezVec3(1,2,3), 4);

    s.Translate(ezVec3(4,5,6));

    EZ_TEST(s.m_vCenter == ezVec3(5,7,9));
    EZ_TEST(s.m_fRadius == 4);
  }

  EZ_TEST_BLOCK(true, "ScaleFromCenter")
  {
    ezBoundingSphere s;
    s.SetElements(ezVec3(1,2,3), 4);

    s.ScaleFromCenter(5.0f);

    EZ_TEST(s.m_vCenter == ezVec3(1,2,3));
    EZ_TEST(s.m_fRadius == 20);
  }
  
  EZ_TEST_BLOCK(true, "ScaleFromOrigin")
  {
    ezBoundingSphere s;
    s.SetElements(ezVec3(1,2,3), 4);

    s.ScaleFromOrigin(ezVec3(2,3,4));

    EZ_TEST(s.m_vCenter == ezVec3(2,6,12));
    EZ_TEST(s.m_fRadius == 16);
  }

  EZ_TEST_BLOCK(true, "GetDistanceTo (point)")
  {
    ezBoundingSphere s;
    s.SetElements(ezVec3(5, 0, 0), 2);

    EZ_TEST(s.GetDistanceTo(ezVec3(5, 0, 0)) ==-2.0f);
    EZ_TEST(s.GetDistanceTo(ezVec3(7, 0, 0)) == 0.0f);
    EZ_TEST(s.GetDistanceTo(ezVec3(9, 0, 0)) == 2.0f);
  }

  EZ_TEST_BLOCK(true, "GetDistanceTo (sphere)")
  {
    ezBoundingSphere s1, s2, s3;
    s1.SetElements(ezVec3(5, 0, 0), 2);
    s2.SetElements(ezVec3(10, 0, 0), 3);
    s3.SetElements(ezVec3(10, 0, 0), 1);

    EZ_TEST(s1.GetDistanceTo(s2) == 0.0f);
    EZ_TEST(s1.GetDistanceTo(s3) == 2.0f);
  }

  EZ_TEST_BLOCK(true, "GetDistanceTo (array)")
  {
    ezBoundingSphere s;
    s.SetElements(ezVec3(0.0f), 0.0f);

    ezVec3 p[4] =
    {
      ezVec3(5),
      ezVec3(10),
      ezVec3(15),
      ezVec3(7),
    };

    EZ_TEST_FLOAT(s.GetDistanceTo(p, 4), ezVec3(5).GetLength(), 0.001f);
  }

  EZ_TEST_BLOCK(true, "Contains (point)")
  {
    ezBoundingSphere s;
    s.SetElements(ezVec3(5, 0, 0), 2.0f);

    EZ_TEST(s.Contains(ezVec3(3, 0, 0)));
    EZ_TEST(s.Contains(ezVec3(5, 0, 0)));
    EZ_TEST(s.Contains(ezVec3(6, 0, 0)));
    EZ_TEST(s.Contains(ezVec3(7, 0, 0)));

    EZ_TEST(!s.Contains(ezVec3(2, 0, 0)));
    EZ_TEST(!s.Contains(ezVec3(8, 0, 0)));
  }

  EZ_TEST_BLOCK(true, "Contains (array)")
  {
    ezBoundingSphere s(ezVec3(0.0f), 6.0f);

    ezVec3 p[4] =
    {
      ezVec3(3),
      ezVec3(10),
      ezVec3(2),
      ezVec3(7),
    };

    EZ_TEST(s.Contains(p, 2, sizeof(ezVec3) * 2));
    EZ_TEST(!s.Contains(p+1, 2, sizeof(ezVec3) * 2));
    EZ_TEST(!s.Contains(p, 4, sizeof(ezVec3)));
  }

  EZ_TEST_BLOCK(true, "Contains (sphere)")
  {
    ezBoundingSphere s1, s2, s3;
    s1.SetElements(ezVec3(5, 0, 0), 2);
    s2.SetElements(ezVec3(6, 0, 0), 1);
    s3.SetElements(ezVec3(6, 0, 0), 2);

    EZ_TEST(s1.Contains(s1));
    EZ_TEST(s2.Contains(s2));
    EZ_TEST(s3.Contains(s3));

    EZ_TEST(s1.Contains(s2));
    EZ_TEST(!s1.Contains(s3));

    EZ_TEST(!s2.Contains(s3));
    EZ_TEST(s3.Contains(s2));
  }

  EZ_TEST_BLOCK(true, "Contains (box)")
  {
    ezBoundingSphere s(ezVec3(1,2,3), 4);
    ezBoundingBox b1(ezVec3(1,2,3) - ezVec3(1), ezVec3(1,2,3) + ezVec3(1));
    ezBoundingBox b2(ezVec3(1,2,3) - ezVec3(1), ezVec3(1,2,3) + ezVec3(3));

    EZ_TEST(s.Contains(b1));
    EZ_TEST(!s.Contains(b2));
  }


  EZ_TEST_BLOCK(true, "Overlaps (array)")
  {
    ezBoundingSphere s(ezVec3(0.0f), 6.0f);

    ezVec3 p[4] =
    {
      ezVec3(3),
      ezVec3(10),
      ezVec3(2),
      ezVec3(7),
    };

    EZ_TEST(s.Overlaps(p, 2, sizeof(ezVec3) * 2));
    EZ_TEST(!s.Overlaps(p+1, 2, sizeof(ezVec3) * 2));
    EZ_TEST(s.Overlaps(p, 4, sizeof(ezVec3)));
  }

  EZ_TEST_BLOCK(true, "Overlaps (sphere)")
  {
    ezBoundingSphere s1, s2, s3;
    s1.SetElements(ezVec3(5, 0, 0), 2);
    s2.SetElements(ezVec3(6, 0, 0), 2);
    s3.SetElements(ezVec3(8, 0, 0), 1);

    EZ_TEST(s1.Overlaps(s1));
    EZ_TEST(s2.Overlaps(s2));
    EZ_TEST(s3.Overlaps(s3));

    EZ_TEST(s1.Overlaps(s2));
    EZ_TEST(!s1.Overlaps(s3));

    EZ_TEST(s2.Overlaps(s3));
    EZ_TEST(s3.Overlaps(s2));
  }

  EZ_TEST_BLOCK(true, "Overlaps (box)")
  {
    ezBoundingSphere s(ezVec3(1,2,3), 2);
    ezBoundingBox b1(ezVec3(1,2,3), ezVec3(1,2,3) + ezVec3(2));
    ezBoundingBox b2(ezVec3(1,2,3) + ezVec3(2), ezVec3(1,2,3) + ezVec3(3));

    EZ_TEST(s.Overlaps(b1));
    EZ_TEST(!s.Overlaps(b2));
  }

  EZ_TEST_BLOCK(true, "GetBoundingBox")
  {
    ezBoundingSphere s;
    s.SetElements(ezVec3(1, 2, 3), 2.0f);

    ezBoundingBox b = s.GetBoundingBox();

    EZ_TEST(b.m_vMin == ezVec3(-1, 0, 1));
    EZ_TEST(b.m_vMax == ezVec3(3, 4, 5));
  }

  EZ_TEST_BLOCK(true, "GetClampedPoint")
  {
    ezBoundingSphere s(ezVec3(1, 2, 3), 2.0f);

    EZ_TEST(s.GetClampedPoint(ezVec3(2, 2, 3)) == ezVec3(2, 2, 3));
    EZ_TEST(s.GetClampedPoint(ezVec3(5, 2, 3)) == ezVec3(3, 2, 3));
    EZ_TEST(s.GetClampedPoint(ezVec3(1, 7, 3)) == ezVec3(1, 4, 3));
  }

  EZ_TEST_BLOCK(true, "GetRayIntersection")
  {
    ezBoundingSphere s(ezVec3(1,2,3), 4);

    for (ezUInt32 i = 0; i < 10000; ++i)
    {
      const ezVec3 vDir = ezVec3(ezMath::SinDeg(i * 1.0f), ezMath::CosDeg(i * 3.0f), ezMath::CosDeg(i * 1.0f)).GetNormalized();
      const ezVec3 vTarget = vDir * s.m_fRadius + s.m_vCenter;
      const ezVec3 vSource = vTarget + vDir * 5.0f;

      EZ_TEST_FLOAT((vSource - vTarget).GetLength(), 5.0f, 0.001f);

      float fIntersection;
      ezVec3 vIntersection;
      EZ_TEST(s.GetRayIntersection(vSource, -vDir, &fIntersection, &vIntersection) == true);
      EZ_TEST_FLOAT(fIntersection, (vSource - vTarget).GetLength(), 0.0001f);
      EZ_TEST(vIntersection.IsEqual(vTarget, 0.0001f));

      EZ_TEST(s.GetRayIntersection(vSource, vDir, &fIntersection, &vIntersection) == false);

      EZ_TEST(s.GetRayIntersection(vTarget - vDir, vDir, &fIntersection, &vIntersection) == true);
      EZ_TEST_FLOAT(fIntersection, 1, 0.0001f);
      EZ_TEST(vIntersection.IsEqual(vTarget, 0.0001f));
    }
  }

  EZ_TEST_BLOCK(true, "GetLineSegmentIntersection")
  {
    ezBoundingSphere s(ezVec3(1,2,3), 4);

    for (ezUInt32 i = 0; i < 10000; ++i)
    {
      const ezVec3 vDir = ezVec3(ezMath::SinDeg(i * 1.0f), ezMath::CosDeg(i * 3.0f), ezMath::CosDeg(i * 1.0f)).GetNormalized();
      const ezVec3 vTarget = vDir * s.m_fRadius + s.m_vCenter - vDir;
      const ezVec3 vSource = vTarget + vDir * 5.0f;

      float fIntersection;
      ezVec3 vIntersection;
      EZ_TEST(s.GetLineSegmentIntersection(vSource, vTarget, &fIntersection, &vIntersection) == true);
      EZ_TEST_FLOAT(fIntersection, 4.0f / 5.0f, 0.0001f);
      EZ_TEST(vIntersection.IsEqual(vTarget + vDir, 0.0001f));

      EZ_TEST(s.GetLineSegmentIntersection(vTarget, vSource, &fIntersection, &vIntersection) == true);
      EZ_TEST_FLOAT(fIntersection, 1.0f / 5.0f, 0.0001f);
      EZ_TEST(vIntersection.IsEqual(vTarget + vDir, 0.0001f));
    }
  }

  EZ_TEST_BLOCK(true, "TransformFromOrigin")
  {
    ezBoundingSphere s(ezVec3(1,2,3), 4);
    ezMat4 mTransform;

    mTransform.SetTranslationMatrix(ezVec3 (5,6,7));
    mTransform.SetScalingFactors(ezVec3(4,3,2));

    s.TransformFromOrigin(mTransform);
    
    EZ_TEST(s.m_vCenter == ezVec3(9,12,13));
    EZ_TEST(s.m_fRadius == 16);
  }

  EZ_TEST_BLOCK(true, "TransformFromCenter")
  {
    ezBoundingSphere s(ezVec3(1,2,3), 4);
    ezMat4 mTransform;

    mTransform.SetTranslationMatrix(ezVec3 (5,6,7));
    mTransform.SetScalingFactors(ezVec3(4,3,2));

    s.TransformFromCenter(mTransform);
    
    EZ_TEST(s.m_vCenter == ezVec3(6,8,10));
    EZ_TEST(s.m_fRadius == 16);
  }
}

