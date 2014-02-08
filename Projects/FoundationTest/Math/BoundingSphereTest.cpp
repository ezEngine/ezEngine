#include <PCH.h>
#include <Foundation/Math/BoundingSphere.h>
#include <Foundation/Math/BoundingBox.h>

EZ_CREATE_SIMPLE_TEST(Math, BoundingSphere)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
    ezBoundingSphereT s(ezVec3T(1,2,3), 4);

    EZ_TEST_BOOL(s.m_vCenter == ezVec3T(1,2,3));
    EZ_TEST_BOOL(s.m_fRadius == 4.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetInvalid / IsValid")
  {
    ezBoundingSphereT s;
    s.SetElements(ezVec3T(1,2,3), 4);

    EZ_TEST_BOOL(s.IsValid());

    s.SetInvalid();

    EZ_TEST_BOOL(!s.IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetZero / IsZero")
  {
    ezBoundingSphereT s;
    s.SetZero();

    EZ_TEST_BOOL(s.IsValid());
    EZ_TEST_BOOL(s.m_vCenter.IsZero());
    EZ_TEST_BOOL(s.m_fRadius == 0.0f);
    EZ_TEST_BOOL(s.IsZero());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetElements")
  {
    ezBoundingSphereT s;
    s.SetElements(ezVec3T(1,2,3), 4);

    EZ_TEST_BOOL(s.m_vCenter == ezVec3T(1,2,3));
    EZ_TEST_BOOL(s.m_fRadius == 4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFromPoints")
  {
    ezBoundingSphereT s;

    ezVec3T p[4] =
    {
      ezVec3T( 0, 2, 0),
      ezVec3T( 4, 2, 0),
      ezVec3T( 2, 0, 0),
      ezVec3T( 2, 4, 0)
    };

    s.SetFromPoints(p, 4);

    EZ_TEST_BOOL(s.m_vCenter == ezVec3T(2, 2, 0));
    EZ_TEST_BOOL(s.m_fRadius == 2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExpandToInclude(Point)")
  {
    ezBoundingSphereT s;
    s.SetZero();

    s.ExpandToInclude(ezVec3T(3, 0, 0));

    EZ_TEST_BOOL(s.m_vCenter == ezVec3T(0, 0, 0));
    EZ_TEST_BOOL(s.m_fRadius == 3);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExpandToInclude(array)")
  {
    ezBoundingSphereT s;
    s.SetElements(ezVec3T(2,2,0), 0.0f);

    ezVec3T p[4] =
    {
      ezVec3T( 0, 2, 0),
      ezVec3T( 4, 2, 0),
      ezVec3T( 2, 0, 0),
      ezVec3T( 2, 4, 0)
    };

    s.ExpandToInclude(p, 4);

    EZ_TEST_BOOL(s.m_vCenter == ezVec3T(2, 2, 0));
    EZ_TEST_BOOL(s.m_fRadius == 2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExpandToInclude (sphere)")
  {
    ezBoundingSphereT s1, s2, s3;
    s1.SetElements(ezVec3T(5, 0, 0), 1);
    s2.SetElements(ezVec3T(6, 0, 0), 1);
    s3.SetElements(ezVec3T(5, 0, 0), 2);

    s1.ExpandToInclude(s2);
    EZ_TEST_BOOL(s1.m_vCenter == ezVec3T(5, 0, 0));
    EZ_TEST_BOOL(s1.m_fRadius == 2);

    s1.ExpandToInclude(s3);
    EZ_TEST_BOOL(s1.m_vCenter == ezVec3T(5, 0, 0));
    EZ_TEST_BOOL(s1.m_fRadius == 2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExpandToInclude (box)")
  {
    ezBoundingSphereT s;
    s.SetElements(ezVec3T(1, 2, 3), 1);

    ezBoundingBoxT b;
    b.SetCenterAndHalfExtents(ezVec3T(1, 2, 3), ezVec3T(2.0f));

    s.ExpandToInclude(b);

    EZ_TEST_BOOL(s.m_vCenter == ezVec3T(1, 2, 3));
    EZ_TEST_FLOAT(s.m_fRadius, ezMath::Sqrt((ezMathTestType) 12), 0.000001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Grow")
  {
    ezBoundingSphereT s;
    s.SetElements(ezVec3T(1, 2, 3), 4);

    s.Grow(5);

    EZ_TEST_BOOL(s.m_vCenter == ezVec3T(1,2,3));
    EZ_TEST_BOOL(s.m_fRadius == 9);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsIdentical, ==, !=")
  {
    ezBoundingSphereT s1, s2, s3;

    s1.SetElements(ezVec3T(1,2,3), 4);
    s2.SetElements(ezVec3T(1,2,3), 4);
    s3.SetElements(ezVec3T(1.001f,2.001f,3.001f), 4.001f);

    EZ_TEST_BOOL(s1 == s1);
    EZ_TEST_BOOL(s2 == s2);
    EZ_TEST_BOOL(s3 == s3);

    EZ_TEST_BOOL(s1 == s2);
    EZ_TEST_BOOL(s2 == s1);

    EZ_TEST_BOOL(s1 != s3);
    EZ_TEST_BOOL(s2 != s3);
    EZ_TEST_BOOL(s3 != s1);
    EZ_TEST_BOOL(s3 != s2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsEqual")
  {
    ezBoundingSphereT s1, s2, s3;

    s1.SetElements(ezVec3T(1,2,3), 4);
    s2.SetElements(ezVec3T(1,2,3), 4);
    s3.SetElements(ezVec3T(1.001f,2.001f,3.001f), 4.001f);

    EZ_TEST_BOOL(s1.IsEqual(s1));
    EZ_TEST_BOOL(s2.IsEqual(s2));
    EZ_TEST_BOOL(s3.IsEqual(s3));

    EZ_TEST_BOOL(s1.IsEqual(s2));
    EZ_TEST_BOOL(s2.IsEqual(s1));

    EZ_TEST_BOOL(!s1.IsEqual(s3, 0.0001f));
    EZ_TEST_BOOL(!s2.IsEqual(s3, 0.0001f));
    EZ_TEST_BOOL(!s3.IsEqual(s1, 0.0001f));
    EZ_TEST_BOOL(!s3.IsEqual(s2, 0.0001f));

    EZ_TEST_BOOL(s1.IsEqual(s3, 0.002f));
    EZ_TEST_BOOL(s2.IsEqual(s3, 0.002f));
    EZ_TEST_BOOL(s3.IsEqual(s1, 0.002f));
    EZ_TEST_BOOL(s3.IsEqual(s2, 0.002f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Translate")
  {
    ezBoundingSphereT s;
    s.SetElements(ezVec3T(1,2,3), 4);

    s.Translate(ezVec3T(4,5,6));

    EZ_TEST_BOOL(s.m_vCenter == ezVec3T(5,7,9));
    EZ_TEST_BOOL(s.m_fRadius == 4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ScaleFromCenter")
  {
    ezBoundingSphereT s;
    s.SetElements(ezVec3T(1,2,3), 4);

    s.ScaleFromCenter(5.0f);

    EZ_TEST_BOOL(s.m_vCenter == ezVec3T(1,2,3));
    EZ_TEST_BOOL(s.m_fRadius == 20);
  }
  
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ScaleFromOrigin")
  {
    ezBoundingSphereT s;
    s.SetElements(ezVec3T(1,2,3), 4);

    s.ScaleFromOrigin(ezVec3T(2,3,4));

    EZ_TEST_BOOL(s.m_vCenter == ezVec3T(2,6,12));
    EZ_TEST_BOOL(s.m_fRadius == 16);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetDistanceTo (point)")
  {
    ezBoundingSphereT s;
    s.SetElements(ezVec3T(5, 0, 0), 2);

    EZ_TEST_BOOL(s.GetDistanceTo(ezVec3T(5, 0, 0)) ==-2.0f);
    EZ_TEST_BOOL(s.GetDistanceTo(ezVec3T(7, 0, 0)) == 0.0f);
    EZ_TEST_BOOL(s.GetDistanceTo(ezVec3T(9, 0, 0)) == 2.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetDistanceTo (sphere)")
  {
    ezBoundingSphereT s1, s2, s3;
    s1.SetElements(ezVec3T(5, 0, 0), 2);
    s2.SetElements(ezVec3T(10, 0, 0), 3);
    s3.SetElements(ezVec3T(10, 0, 0), 1);

    EZ_TEST_BOOL(s1.GetDistanceTo(s2) == 0.0f);
    EZ_TEST_BOOL(s1.GetDistanceTo(s3) == 2.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetDistanceTo (array)")
  {
    ezBoundingSphereT s;
    s.SetElements(ezVec3T(0.0f), 0.0f);

    ezVec3T p[4] =
    {
      ezVec3T(5),
      ezVec3T(10),
      ezVec3T(15),
      ezVec3T(7),
    };

    EZ_TEST_FLOAT(s.GetDistanceTo(p, 4), ezVec3T(5).GetLength(), 0.001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Contains (point)")
  {
    ezBoundingSphereT s;
    s.SetElements(ezVec3T(5, 0, 0), 2.0f);

    EZ_TEST_BOOL(s.Contains(ezVec3T(3, 0, 0)));
    EZ_TEST_BOOL(s.Contains(ezVec3T(5, 0, 0)));
    EZ_TEST_BOOL(s.Contains(ezVec3T(6, 0, 0)));
    EZ_TEST_BOOL(s.Contains(ezVec3T(7, 0, 0)));

    EZ_TEST_BOOL(!s.Contains(ezVec3T(2, 0, 0)));
    EZ_TEST_BOOL(!s.Contains(ezVec3T(8, 0, 0)));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Contains (array)")
  {
    ezBoundingSphereT s(ezVec3T(0.0f), 6.0f);

    ezVec3T p[4] =
    {
      ezVec3T(3),
      ezVec3T(10),
      ezVec3T(2),
      ezVec3T(7),
    };

    EZ_TEST_BOOL(s.Contains(p, 2, sizeof(ezVec3T) * 2));
    EZ_TEST_BOOL(!s.Contains(p+1, 2, sizeof(ezVec3T) * 2));
    EZ_TEST_BOOL(!s.Contains(p, 4, sizeof(ezVec3T)));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Contains (sphere)")
  {
    ezBoundingSphereT s1, s2, s3;
    s1.SetElements(ezVec3T(5, 0, 0), 2);
    s2.SetElements(ezVec3T(6, 0, 0), 1);
    s3.SetElements(ezVec3T(6, 0, 0), 2);

    EZ_TEST_BOOL(s1.Contains(s1));
    EZ_TEST_BOOL(s2.Contains(s2));
    EZ_TEST_BOOL(s3.Contains(s3));

    EZ_TEST_BOOL(s1.Contains(s2));
    EZ_TEST_BOOL(!s1.Contains(s3));

    EZ_TEST_BOOL(!s2.Contains(s3));
    EZ_TEST_BOOL(s3.Contains(s2));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Contains (box)")
  {
    ezBoundingSphereT s(ezVec3T(1,2,3), 4);
    ezBoundingBoxT b1(ezVec3T(1,2,3) - ezVec3T(1), ezVec3T(1,2,3) + ezVec3T(1));
    ezBoundingBoxT b2(ezVec3T(1,2,3) - ezVec3T(1), ezVec3T(1,2,3) + ezVec3T(3));

    EZ_TEST_BOOL(s.Contains(b1));
    EZ_TEST_BOOL(!s.Contains(b2));

    ezVec3T vDir(1, 1, 1);
    vDir.SetLength(3.99f);
    ezBoundingBoxT b3(ezVec3T(1,2,3) - ezVec3T(1), ezVec3T(1,2,3) + vDir);

    EZ_TEST_BOOL(s.Contains(b3));

    vDir.SetLength(4.01f);
    ezBoundingBoxT b4(ezVec3T(1,2,3) - ezVec3T(1), ezVec3T(1,2,3) + vDir);

    EZ_TEST_BOOL(!s.Contains(b4));
  }


  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Overlaps (array)")
  {
    ezBoundingSphereT s(ezVec3T(0.0f), 6.0f);

    ezVec3T p[4] =
    {
      ezVec3T(3),
      ezVec3T(10),
      ezVec3T(2),
      ezVec3T(7),
    };

    EZ_TEST_BOOL(s.Overlaps(p, 2, sizeof(ezVec3T) * 2));
    EZ_TEST_BOOL(!s.Overlaps(p+1, 2, sizeof(ezVec3T) * 2));
    EZ_TEST_BOOL(s.Overlaps(p, 4, sizeof(ezVec3T)));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Overlaps (sphere)")
  {
    ezBoundingSphereT s1, s2, s3;
    s1.SetElements(ezVec3T(5, 0, 0), 2);
    s2.SetElements(ezVec3T(6, 0, 0), 2);
    s3.SetElements(ezVec3T(8, 0, 0), 1);

    EZ_TEST_BOOL(s1.Overlaps(s1));
    EZ_TEST_BOOL(s2.Overlaps(s2));
    EZ_TEST_BOOL(s3.Overlaps(s3));

    EZ_TEST_BOOL(s1.Overlaps(s2));
    EZ_TEST_BOOL(!s1.Overlaps(s3));

    EZ_TEST_BOOL(s2.Overlaps(s3));
    EZ_TEST_BOOL(s3.Overlaps(s2));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Overlaps (box)")
  {
    ezBoundingSphereT s(ezVec3T(1,2,3), 2);
    ezBoundingBoxT b1(ezVec3T(1,2,3), ezVec3T(1,2,3) + ezVec3T(2));
    ezBoundingBoxT b2(ezVec3T(1,2,3) + ezVec3T(2), ezVec3T(1,2,3) + ezVec3T(3));

    EZ_TEST_BOOL(s.Overlaps(b1));
    EZ_TEST_BOOL(!s.Overlaps(b2));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetBoundingBox")
  {
    ezBoundingSphereT s;
    s.SetElements(ezVec3T(1, 2, 3), 2.0f);

    ezBoundingBoxT b = s.GetBoundingBox();

    EZ_TEST_BOOL(b.m_vMin == ezVec3T(-1, 0, 1));
    EZ_TEST_BOOL(b.m_vMax == ezVec3T(3, 4, 5));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetClampedPoint")
  {
    ezBoundingSphereT s(ezVec3T(1, 2, 3), 2.0f);

    EZ_TEST_VEC3(s.GetClampedPoint(ezVec3T(2, 2, 3)), ezVec3T(2, 2, 3), 0.001);
    EZ_TEST_VEC3(s.GetClampedPoint(ezVec3T(5, 2, 3)), ezVec3T(3, 2, 3), 0.001);
    EZ_TEST_VEC3(s.GetClampedPoint(ezVec3T(1, 7, 3)), ezVec3T(1, 4, 3), 0.001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetRayIntersection")
  {
    ezBoundingSphereT s(ezVec3T(1,2,3), 4);

    for (ezUInt32 i = 0; i < 10000; ++i)
    {
      const ezVec3T vDir = ezVec3T(ezMath::Sin(ezAngle::Degree(i * 1.0f)),
                                   ezMath::Cos(ezAngle::Degree(i * 3.0f)),
                                   ezMath::Cos(ezAngle::Degree(i * 1.0f))).GetNormalized();
      const ezVec3T vTarget = vDir * s.m_fRadius + s.m_vCenter;
      const ezVec3T vSource = vTarget + vDir * (ezMathTestType) 5;

      EZ_TEST_FLOAT((vSource - vTarget).GetLength(), 5.0f, 0.001f);

      ezMathTestType fIntersection;
      ezVec3T vIntersection;
      EZ_TEST_BOOL(s.GetRayIntersection(vSource, -vDir, &fIntersection, &vIntersection) == true);
      EZ_TEST_FLOAT(fIntersection, (vSource - vTarget).GetLength(), 0.0001f);
      EZ_TEST_BOOL(vIntersection.IsEqual(vTarget, 0.0001f));

      EZ_TEST_BOOL(s.GetRayIntersection(vSource, vDir, &fIntersection, &vIntersection) == false);

      EZ_TEST_BOOL(s.GetRayIntersection(vTarget - vDir, vDir, &fIntersection, &vIntersection) == true);
      EZ_TEST_FLOAT(fIntersection, 1, 0.0001f);
      EZ_TEST_BOOL(vIntersection.IsEqual(vTarget, 0.0001f));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetLineSegmentIntersection")
  {
    ezBoundingSphereT s(ezVec3T(1,2,3), 4);

    for (ezUInt32 i = 0; i < 10000; ++i)
    {
      const ezVec3T vDir = ezVec3T(ezMath::Sin(ezAngle::Degree(i * (ezMathTestType) 1)), 
                                   ezMath::Cos(ezAngle::Degree(i * (ezMathTestType) 3)),
                                   ezMath::Cos(ezAngle::Degree(i * (ezMathTestType) 1))).GetNormalized();
      const ezVec3T vTarget = vDir * s.m_fRadius + s.m_vCenter - vDir;
      const ezVec3T vSource = vTarget + vDir * (ezMathTestType) 5;

      ezMathTestType fIntersection;
      ezVec3T vIntersection;
      EZ_TEST_BOOL(s.GetLineSegmentIntersection(vSource, vTarget, &fIntersection, &vIntersection) == true);
      EZ_TEST_FLOAT(fIntersection, 4.0f / 5.0f, 0.0001f);
      EZ_TEST_BOOL(vIntersection.IsEqual(vTarget + vDir, 0.0001f));

      EZ_TEST_BOOL(s.GetLineSegmentIntersection(vTarget, vSource, &fIntersection, &vIntersection) == true);
      EZ_TEST_FLOAT(fIntersection, 1.0f / 5.0f, 0.0001f);
      EZ_TEST_BOOL(vIntersection.IsEqual(vTarget + vDir, 0.0001f));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TransformFromOrigin")
  {
    ezBoundingSphereT s(ezVec3T(1,2,3), 4);
    ezMat4T mTransform;

    mTransform.SetTranslationMatrix(ezVec3T (5,6,7));
    mTransform.SetScalingFactors(ezVec3T(4,3,2));

    s.TransformFromOrigin(mTransform);
    
    EZ_TEST_BOOL(s.m_vCenter == ezVec3T(9,12,13));
    EZ_TEST_BOOL(s.m_fRadius == 16);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TransformFromCenter")
  {
    ezBoundingSphereT s(ezVec3T(1,2,3), 4);
    ezMat4T mTransform;

    mTransform.SetTranslationMatrix(ezVec3T (5,6,7));
    mTransform.SetScalingFactors(ezVec3T(4,3,2));

    s.TransformFromCenter(mTransform);
    
    EZ_TEST_BOOL(s.m_vCenter == ezVec3T(6,8,10));
    EZ_TEST_BOOL(s.m_fRadius == 16);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsNaN")
  {
    if (ezMath::BasicType<ezMathTestType>::SupportsNaN())
    {
      ezBoundingSphereT s;

      s.SetInvalid();
      EZ_TEST_BOOL(!s.IsNaN());

      s.SetInvalid();
      s.m_fRadius = ezMath::BasicType<ezMathTestType>::GetNaN();
      EZ_TEST_BOOL(s.IsNaN());

      s.SetInvalid();
      s.m_vCenter.x = ezMath::BasicType<ezMathTestType>::GetNaN();
      EZ_TEST_BOOL(s.IsNaN());

      s.SetInvalid();
      s.m_vCenter.y = ezMath::BasicType<ezMathTestType>::GetNaN();
      EZ_TEST_BOOL(s.IsNaN());

      s.SetInvalid();
      s.m_vCenter.z = ezMath::BasicType<ezMathTestType>::GetNaN();
      EZ_TEST_BOOL(s.IsNaN());
    }
  }

}

