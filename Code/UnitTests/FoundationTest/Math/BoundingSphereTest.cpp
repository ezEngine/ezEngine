#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/BoundingSphere.h>

template <typename Type>
void TestBoundingSphere()
{
  using ezBoundingSphereType = ezBoundingSphereTemplate<Type>;
  using ezBoundingBoxType = ezBoundingBoxTemplate<Type>;
  using ezVec3Type = ezVec3Template<Type>;
  using ezMat4Type = ezMat4Template<Type>;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
    ezBoundingSphereType s = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(1, 2, 3), 4);

    EZ_TEST_BOOL(s.m_vCenter == ezVec3Type(1, 2, 3));
    EZ_TEST_BOOL(s.m_fRadius == 4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetInvalid / IsValid")
  {
    ezBoundingSphereType s = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(1, 2, 3), 4);

    EZ_TEST_BOOL(s.IsValid());

    s = ezBoundingSphereType::MakeInvalid();

    EZ_TEST_BOOL(!s.IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeZero / IsZero")
  {
    ezBoundingSphereType s = ezBoundingSphereType::MakeZero();

    EZ_TEST_BOOL(s.IsValid());
    EZ_TEST_BOOL(s.m_vCenter.IsZero());
    EZ_TEST_BOOL(s.m_fRadius == 0);
    EZ_TEST_BOOL(s.IsZero());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetElements")
  {
    ezBoundingSphereType s = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(1, 2, 3), 4);

    EZ_TEST_BOOL(s.m_vCenter == ezVec3Type(1, 2, 3));
    EZ_TEST_BOOL(s.m_fRadius == 4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFromPoints")
  {


    ezVec3Type p[4] = {ezVec3Type(2, 6, 0), ezVec3Type(4, 2, 0), ezVec3Type(2, 0, 0), ezVec3Type(0, 4, 0)};

    ezBoundingSphereType s = ezBoundingSphereType::MakeFromPoints(p, 4);

    EZ_TEST_BOOL(s.m_vCenter == ezVec3Type(2, 3, 0));
    EZ_TEST_BOOL(s.m_fRadius == 3);

    for (int i = 0; i < EZ_ARRAY_SIZE(p); ++i)
    {
      EZ_TEST_BOOL(s.Contains(p[i]));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExpandToInclude(Point)")
  {
    ezBoundingSphereType s = ezBoundingSphereType::MakeZero();

    s.ExpandToInclude(ezVec3Type(3, 0, 0));

    EZ_TEST_BOOL(s.m_vCenter == ezVec3Type(0, 0, 0));
    EZ_TEST_BOOL(s.m_fRadius == 3);

    s = ezBoundingSphereType::MakeInvalid();

    s.ExpandToInclude(ezVec3Type(0.25, 0.0, 0.0));

    EZ_TEST_BOOL(s.m_vCenter == ezVec3Type(0, 0, 0));
    EZ_TEST_BOOL(s.m_fRadius == Type(0.25));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExpandToInclude(array)")
  {
    ezBoundingSphereType s = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(2, 2, 0), 0.0f);

    ezVec3Type p[4] = {ezVec3Type(0, 2, 0), ezVec3Type(4, 2, 0), ezVec3Type(2, 0, 0), ezVec3Type(2, 4, 0)};

    s.ExpandToInclude(p, 4);

    EZ_TEST_BOOL(s.m_vCenter == ezVec3Type(2, 2, 0));
    EZ_TEST_BOOL(s.m_fRadius == 2);

    for (int i = 0; i < EZ_ARRAY_SIZE(p); ++i)
    {
      EZ_TEST_BOOL(s.Contains(p[i]));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExpandToInclude (sphere)")
  {
    ezBoundingSphereType s1, s2, s3;
    s1 = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(5, 0, 0), 1);
    s2 = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(6, 0, 0), 1);
    s3 = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(5, 0, 0), 2);

    s1.ExpandToInclude(s2);
    EZ_TEST_BOOL(s1.m_vCenter == ezVec3Type(5, 0, 0));
    EZ_TEST_BOOL(s1.m_fRadius == 2);

    s1.ExpandToInclude(s3);
    EZ_TEST_BOOL(s1.m_vCenter == ezVec3Type(5, 0, 0));
    EZ_TEST_BOOL(s1.m_fRadius == 2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExpandToInclude (box)")
  {
    ezBoundingSphereType s = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(1, 2, 3), 1);

    ezBoundingBoxType b = ezBoundingBoxType::MakeFromCenterAndHalfExtents(ezVec3Type(1, 2, 3), ezVec3Type(2.0f));

    s.ExpandToInclude(b);

    EZ_TEST_BOOL(s.m_vCenter == ezVec3Type(1, 2, 3));
    EZ_TEST_FLOAT(s.m_fRadius, ezMath::Sqrt(Type(12)), Type(0.000001));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Grow")
  {
    ezBoundingSphereType s = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(1, 2, 3), 4);

    s.Grow(5);

    EZ_TEST_BOOL(s.m_vCenter == ezVec3Type(1, 2, 3));
    EZ_TEST_BOOL(s.m_fRadius == 9);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsIdentical, ==, !=")
  {
    ezBoundingSphereType s1, s2, s3;

    s1 = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(1, 2, 3), 4);
    s2 = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(1, 2, 3), 4);
    s3 = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(1.001f, 2.001f, 3.001f), 4.001f);

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
    ezBoundingSphereType s1, s2, s3;

    s1 = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(1, 2, 3), 4);
    s2 = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(1, 2, 3), 4);
    s3 = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(1.001f, 2.001f, 3.001f), 4.001f);

    EZ_TEST_BOOL(s1.IsEqual(s1));
    EZ_TEST_BOOL(s2.IsEqual(s2));
    EZ_TEST_BOOL(s3.IsEqual(s3));

    EZ_TEST_BOOL(s1.IsEqual(s2));
    EZ_TEST_BOOL(s2.IsEqual(s1));

    EZ_TEST_BOOL(!s1.IsEqual(s3, Type(0.0001)));
    EZ_TEST_BOOL(!s2.IsEqual(s3, Type(0.0001)));
    EZ_TEST_BOOL(!s3.IsEqual(s1, Type(0.0001)));
    EZ_TEST_BOOL(!s3.IsEqual(s2, Type(0.0001)));

    EZ_TEST_BOOL(s1.IsEqual(s3, Type(0.002)));
    EZ_TEST_BOOL(s2.IsEqual(s3, Type(0.002)));
    EZ_TEST_BOOL(s3.IsEqual(s1, Type(0.002)));
    EZ_TEST_BOOL(s3.IsEqual(s2, Type(0.002)));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Translate")
  {
    ezBoundingSphereType s = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(1, 2, 3), 4);

    s.Translate(ezVec3Type(4, 5, 6));

    EZ_TEST_BOOL(s.m_vCenter == ezVec3Type(5, 7, 9));
    EZ_TEST_BOOL(s.m_fRadius == 4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ScaleFromCenter")
  {
    ezBoundingSphereType s = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(1, 2, 3), 4);

    s.ScaleFromCenter(5.0f);

    EZ_TEST_BOOL(s.m_vCenter == ezVec3Type(1, 2, 3));
    EZ_TEST_BOOL(s.m_fRadius == 20);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ScaleFromOrigin")
  {
    ezBoundingSphereType s = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(1, 2, 3), 4);

    s.ScaleFromOrigin(ezVec3Type(2, 3, 4));

    EZ_TEST_BOOL(s.m_vCenter == ezVec3Type(2, 6, 12));
    EZ_TEST_BOOL(s.m_fRadius == 16);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetDistanceTo (point)")
  {
    ezBoundingSphereType s = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(5, 0, 0), 2);

    EZ_TEST_BOOL(s.GetDistanceTo(ezVec3Type(5, 0, 0)) == -2);
    EZ_TEST_BOOL(s.GetDistanceTo(ezVec3Type(7, 0, 0)) == 0);
    EZ_TEST_BOOL(s.GetDistanceTo(ezVec3Type(9, 0, 0)) == 2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetDistanceTo (sphere)")
  {
    ezBoundingSphereType s1, s2, s3;
    s1 = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(5, 0, 0), 2);
    s2 = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(10, 0, 0), 3);
    s3 = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(10, 0, 0), 1);

    EZ_TEST_BOOL(s1.GetDistanceTo(s2) == 0);
    EZ_TEST_BOOL(s1.GetDistanceTo(s3) == 2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetDistanceTo (array)")
  {
    ezBoundingSphereType s = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(0.0f), 0.0f);

    ezVec3Type p[4] = {
      ezVec3Type(5),
      ezVec3Type(10),
      ezVec3Type(15),
      ezVec3Type(7),
    };

    EZ_TEST_FLOAT(s.GetDistanceTo(p, 4), ezVec3Type(5).GetLength(), Type(0.001));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Contains (point)")
  {
    ezBoundingSphereType s = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(5, 0, 0), 2.0f);

    EZ_TEST_BOOL(s.Contains(ezVec3Type(3, 0, 0)));
    EZ_TEST_BOOL(s.Contains(ezVec3Type(5, 0, 0)));
    EZ_TEST_BOOL(s.Contains(ezVec3Type(6, 0, 0)));
    EZ_TEST_BOOL(s.Contains(ezVec3Type(7, 0, 0)));

    EZ_TEST_BOOL(!s.Contains(ezVec3Type(2, 0, 0)));
    EZ_TEST_BOOL(!s.Contains(ezVec3Type(8, 0, 0)));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Contains (array)")
  {
    ezBoundingSphereType s = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(0.0f), 6.0f);

    ezVec3Type p[4] = {
      ezVec3Type(3),
      ezVec3Type(10),
      ezVec3Type(2),
      ezVec3Type(7),
    };

    EZ_TEST_BOOL(s.Contains(p, 2, sizeof(ezVec3Type) * 2));
    EZ_TEST_BOOL(!s.Contains(p + 1, 2, sizeof(ezVec3Type) * 2));
    EZ_TEST_BOOL(!s.Contains(p, 4, sizeof(ezVec3Type)));
  }

  // Disabled because MSVC 2017 has code generation issues in Release builds
  EZ_TEST_BLOCK(ezTestBlock::Disabled, "Contains (sphere)")
  {
    ezBoundingSphereType s1, s2, s3;
    s1 = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(5, 0, 0), 2);
    s2 = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(6, 0, 0), 1);
    s3 = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(6, 0, 0), 2);

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
    ezBoundingSphereType s = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(1, 2, 3), 4);
    ezBoundingBoxType b1 = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(1, 2, 3) - ezVec3Type(1), ezVec3Type(1, 2, 3) + ezVec3Type(1));
    ezBoundingBoxType b2 = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(1, 2, 3) - ezVec3Type(1), ezVec3Type(1, 2, 3) + ezVec3Type(3));

    EZ_TEST_BOOL(s.Contains(b1));
    EZ_TEST_BOOL(!s.Contains(b2));

    ezVec3Type vDir(1, 1, 1);
    vDir.SetLength(3.99f).IgnoreResult();
    ezBoundingBoxType b3 = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(1, 2, 3) - ezVec3Type(1), ezVec3Type(1, 2, 3) + vDir);

    EZ_TEST_BOOL(s.Contains(b3));

    vDir.SetLength(4.01f).IgnoreResult();
    ezBoundingBoxType b4 = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(1, 2, 3) - ezVec3Type(1), ezVec3Type(1, 2, 3) + vDir);

    EZ_TEST_BOOL(!s.Contains(b4));
  }



  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Overlaps (array)")
  {
    ezBoundingSphereType s = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(0.0f), 6.0f);

    ezVec3Type p[4] = {
      ezVec3Type(3),
      ezVec3Type(10),
      ezVec3Type(2),
      ezVec3Type(7),
    };

    EZ_TEST_BOOL(s.Overlaps(p, 2, sizeof(ezVec3Type) * 2));
    EZ_TEST_BOOL(!s.Overlaps(p + 1, 2, sizeof(ezVec3Type) * 2));
    EZ_TEST_BOOL(s.Overlaps(p, 4, sizeof(ezVec3Type)));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Overlaps (sphere)")
  {
    ezBoundingSphereType s1, s2, s3;
    s1 = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(5, 0, 0), 2);
    s2 = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(6, 0, 0), 2);
    s3 = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(8, 0, 0), 1);

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
    ezBoundingSphereType s = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(1, 2, 3), 2);
    ezBoundingBoxType b1 = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(1, 2, 3), ezVec3Type(1, 2, 3) + ezVec3Type(2));
    ezBoundingBoxType b2 = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(1, 2, 3) + ezVec3Type(2), ezVec3Type(1, 2, 3) + ezVec3Type(3));

    EZ_TEST_BOOL(s.Overlaps(b1));
    EZ_TEST_BOOL(!s.Overlaps(b2));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetBoundingBox")
  {
    ezBoundingSphereType s = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(1, 2, 3), 2.0f);

    ezBoundingBoxType b = s.GetBoundingBox();

    EZ_TEST_BOOL(b.m_vMin == ezVec3Type(-1, 0, 1));
    EZ_TEST_BOOL(b.m_vMax == ezVec3Type(3, 4, 5));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetClampedPoint")
  {
    ezBoundingSphereType s = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(1, 2, 3), 2.0f);

    EZ_TEST_VEC3(s.GetClampedPoint(ezVec3Type(2, 2, 3)), ezVec3Type(2, 2, 3), Type(0.001));
    EZ_TEST_VEC3(s.GetClampedPoint(ezVec3Type(5, 2, 3)), ezVec3Type(3, 2, 3), Type(0.001));
    EZ_TEST_VEC3(s.GetClampedPoint(ezVec3Type(1, 7, 3)), ezVec3Type(1, 4, 3), Type(0.001));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetRayIntersection")
  {
    ezBoundingSphereType s = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(1, 2, 3), 4);

    for (ezUInt32 i = 0; i < 10000; ++i)
    {
      const ezVec3Type vDir =
        ezVec3Type(ezMath::Sin(ezAngleTemplate<Type>::MakeFromDegree(i * 1.0f)), ezMath::Cos(ezAngleTemplate<Type>::MakeFromDegree(i * 3.0f)), ezMath::Cos(ezAngleTemplate<Type>::MakeFromDegree(i * 1.0f)))
          .GetNormalized();
      const ezVec3Type vTarget = vDir * s.m_fRadius + s.m_vCenter;
      const ezVec3Type vSource = vTarget + vDir * Type(5);

      EZ_TEST_FLOAT((vSource - vTarget).GetLength(), 5.0f, Type(0.001));

      Type fIntersection;
      ezVec3Type vIntersection;
      EZ_TEST_BOOL(s.GetRayIntersection(vSource, -vDir, &fIntersection, &vIntersection) == true);
      EZ_TEST_FLOAT(fIntersection, (vSource - vTarget).GetLength(), Type(0.0001));
      EZ_TEST_BOOL(vIntersection.IsEqual(vTarget, Type(0.0001)));

      EZ_TEST_BOOL(s.GetRayIntersection(vSource, vDir, &fIntersection, &vIntersection) == false);

      EZ_TEST_BOOL(s.GetRayIntersection(vTarget - vDir, vDir, &fIntersection, &vIntersection) == true);
      EZ_TEST_FLOAT(fIntersection, 1, Type(0.0001));
      EZ_TEST_BOOL(vIntersection.IsEqual(vTarget, Type(0.0001)));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetLineSegmentIntersection")
  {
    ezBoundingSphereType s = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(1, 2, 3), 4);

    for (ezUInt32 i = 0; i < 10000; ++i)
    {
      const ezVec3Type vDir = ezVec3Type(ezMath::Sin(ezAngleTemplate<Type>::MakeFromDegree(i * Type(1))), ezMath::Cos(ezAngleTemplate<Type>::MakeFromDegree(i * Type(3))),
        ezMath::Cos(ezAngleTemplate<Type>::MakeFromDegree(i * Type(1))))
                             .GetNormalized();
      const ezVec3Type vTarget = vDir * s.m_fRadius + s.m_vCenter - vDir;
      const ezVec3Type vSource = vTarget + vDir * Type(5);

      Type fIntersection;
      ezVec3Type vIntersection;
      EZ_TEST_BOOL(s.GetLineSegmentIntersection(vSource, vTarget, &fIntersection, &vIntersection) == true);
      EZ_TEST_FLOAT(fIntersection, 4.0f / 5.0f, Type(0.0001));
      EZ_TEST_BOOL(vIntersection.IsEqual(vTarget + vDir, Type(0.0001)));

      EZ_TEST_BOOL(s.GetLineSegmentIntersection(vTarget, vSource, &fIntersection, &vIntersection) == true);
      EZ_TEST_FLOAT(fIntersection, 1.0f / 5.0f, Type(0.0001));
      EZ_TEST_BOOL(vIntersection.IsEqual(vTarget + vDir, Type(0.0001)));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TransformFromOrigin")
  {
    ezBoundingSphereType s = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(1, 2, 3), 4);
    ezMat4Type mTransform;

    mTransform = ezMat4Type::MakeTranslation(ezVec3Type(5, 6, 7));
    mTransform.SetScalingFactors(ezVec3Type(4, 3, 2)).IgnoreResult();

    s.TransformFromOrigin(mTransform);

    EZ_TEST_BOOL(s.m_vCenter == ezVec3Type(9, 12, 13));
    EZ_TEST_BOOL(s.m_fRadius == 16);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TransformFromCenter")
  {
    ezBoundingSphereType s = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(1, 2, 3), 4);
    ezMat4Type mTransform;

    mTransform = ezMat4Type::MakeTranslation(ezVec3Type(5, 6, 7));
    mTransform.SetScalingFactors(ezVec3Type(4, 3, 2)).IgnoreResult();

    s.TransformFromCenter(mTransform);

    EZ_TEST_BOOL(s.m_vCenter == ezVec3Type(6, 8, 10));
    EZ_TEST_BOOL(s.m_fRadius == 16);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsNaN")
  {
    if (ezMath::SupportsNaN<Type>())
    {
      ezBoundingSphereType s;

      s = ezBoundingSphereType::MakeInvalid();
      EZ_TEST_BOOL(!s.IsNaN());

      s = ezBoundingSphereType::MakeInvalid();
      s.m_fRadius = ezMath::NaN<Type>();
      EZ_TEST_BOOL(s.IsNaN());

      s = ezBoundingSphereType::MakeInvalid();
      s.m_vCenter.x = ezMath::NaN<Type>();
      EZ_TEST_BOOL(s.IsNaN());

      s = ezBoundingSphereType::MakeInvalid();
      s.m_vCenter.y = ezMath::NaN<Type>();
      EZ_TEST_BOOL(s.IsNaN());

      s = ezBoundingSphereType::MakeInvalid();
      s.m_vCenter.z = ezMath::NaN<Type>();
      EZ_TEST_BOOL(s.IsNaN());
    }
  }
}

EZ_CREATE_SIMPLE_TEST(Math, BoundingSpheref)
{
  TestBoundingSphere<float>();
}

EZ_CREATE_SIMPLE_TEST(Math, BoundingSphered)
{
  TestBoundingSphere<double>();
}
