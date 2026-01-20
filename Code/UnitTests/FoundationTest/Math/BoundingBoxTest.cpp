#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/BoundingBox.h>

template <typename Type>
void TestBoundingBox()
{
  using ezBoundingBoxType = ezBoundingBoxTemplate<Type>;
  using ezBoundingSphereType = ezBoundingSphereTemplate<Type>;
  using ezVec3Type = ezVec3Template<Type>;
  using ezMat4Type = ezMat4Template<Type>;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeFromMinMax")
  {
    ezBoundingBoxType b = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(-1, -2, -3), ezVec3Type(1, 2, 3));

    EZ_TEST_BOOL(b.m_vMin == ezVec3Type(-1, -2, -3));
    EZ_TEST_BOOL(b.m_vMax == ezVec3Type(1, 2, 3));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeFromMinMax")
  {
    ezBoundingBoxType b = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(-1, -2, -3), ezVec3Type(1, 2, 3));

    EZ_TEST_BOOL(b.m_vMin == ezVec3Type(-1, -2, -3));
    EZ_TEST_BOOL(b.m_vMax == ezVec3Type(1, 2, 3));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeFromPoints")
  {
    ezVec3Type p[6] = {
      ezVec3Type(-4, 0, 0),
      ezVec3Type(5, 0, 0),
      ezVec3Type(0, -6, 0),
      ezVec3Type(0, 7, 0),
      ezVec3Type(0, 0, -8),
      ezVec3Type(0, 0, 9),
    };

    ezBoundingBoxType b = ezBoundingBoxType::MakeFromPoints(p, 6);

    EZ_TEST_BOOL(b.m_vMin == ezVec3Type(-4, -6, -8));
    EZ_TEST_BOOL(b.m_vMax == ezVec3Type(5, 7, 9));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeInvalid")
  {
    ezBoundingBoxType b;
    b = ezBoundingBoxType::MakeInvalid();

    EZ_TEST_BOOL(!b.IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeFromCenterAndHalfExtents")
  {
    ezBoundingBoxType b = ezBoundingBoxType::MakeFromCenterAndHalfExtents(ezVec3Type(1, 2, 3), ezVec3Type(4, 5, 6));

    EZ_TEST_BOOL(b.m_vMin == ezVec3Type(-3, -3, -3));
    EZ_TEST_BOOL(b.m_vMax == ezVec3Type(5, 7, 9));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetCorners")
  {
    ezBoundingBoxType b = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(-1, -2, -3), ezVec3Type(1, 2, 3));

    ezVec3Type c[8];
    b.GetCorners(c);

    EZ_TEST_BOOL(c[0] == ezVec3Type(-1, -2, -3));
    EZ_TEST_BOOL(c[1] == ezVec3Type(-1, -2, 3));
    EZ_TEST_BOOL(c[2] == ezVec3Type(-1, 2, -3));
    EZ_TEST_BOOL(c[3] == ezVec3Type(-1, 2, 3));
    EZ_TEST_BOOL(c[4] == ezVec3Type(1, -2, -3));
    EZ_TEST_BOOL(c[5] == ezVec3Type(1, -2, 3));
    EZ_TEST_BOOL(c[6] == ezVec3Type(1, 2, -3));
    EZ_TEST_BOOL(c[7] == ezVec3Type(1, 2, 3));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExpandToInclue (Point)")
  {
    ezBoundingBoxType b;
    b = ezBoundingBoxType::MakeInvalid();
    b.ExpandToInclude(ezVec3Type(1, 2, 3));

    EZ_TEST_BOOL(b.m_vMin == ezVec3Type(1, 2, 3));
    EZ_TEST_BOOL(b.m_vMax == ezVec3Type(1, 2, 3));


    b.ExpandToInclude(ezVec3Type(2, 3, 4));

    EZ_TEST_BOOL(b.m_vMin == ezVec3Type(1, 2, 3));
    EZ_TEST_BOOL(b.m_vMax == ezVec3Type(2, 3, 4));

    b.ExpandToInclude(ezVec3Type(0, 1, 2));

    EZ_TEST_BOOL(b.m_vMin == ezVec3Type(0, 1, 2));
    EZ_TEST_BOOL(b.m_vMax == ezVec3Type(2, 3, 4));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExpandToInclude (Box)")
  {
    ezBoundingBoxType b1, b2;

    b1 = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(-1, -2, -3), ezVec3Type(1, 2, 3));
    b2 = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(0), ezVec3Type(4, 5, 6));

    b1.ExpandToInclude(b2);

    EZ_TEST_BOOL(b1.m_vMin == ezVec3Type(-1, -2, -3));
    EZ_TEST_BOOL(b1.m_vMax == ezVec3Type(4, 5, 6));

    b2 = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(-4, -5, -6), ezVec3Type(0));

    b1.ExpandToInclude(b2);

    EZ_TEST_BOOL(b1.m_vMin == ezVec3Type(-4, -5, -6));
    EZ_TEST_BOOL(b1.m_vMax == ezVec3Type(4, 5, 6));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExpandToInclude (array)")
  {
    ezVec3Type v[4] = {ezVec3Type(1, 1, 1), ezVec3Type(-1, -1, -1), ezVec3Type(2, 2, 2), ezVec3Type(4, 4, 4)};

    ezBoundingBoxType b;
    b = ezBoundingBoxType::MakeInvalid();
    b.ExpandToInclude(v, 2, sizeof(ezVec3Type) * 2);

    EZ_TEST_BOOL(b.m_vMin == ezVec3Type(1, 1, 1));
    EZ_TEST_BOOL(b.m_vMax == ezVec3Type(2, 2, 2));

    b.ExpandToInclude(v, 4, sizeof(ezVec3Type));

    EZ_TEST_BOOL(b.m_vMin == ezVec3Type(-1, -1, -1));
    EZ_TEST_BOOL(b.m_vMax == ezVec3Type(4, 4, 4));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExpandToCube")
  {
    ezBoundingBoxType b = ezBoundingBoxType::MakeFromCenterAndHalfExtents(ezVec3Type(1, 2, 3), ezVec3Type(4, 5, 6));

    b.ExpandToCube();

    EZ_TEST_VEC3(b.GetCenter(), ezVec3Type(1, 2, 3), ezMath::DefaultEpsilon<Type>());
    EZ_TEST_VEC3(b.GetHalfExtents(), ezVec3Type(6, 6, 6), ezMath::DefaultEpsilon<Type>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Grow")
  {
    ezBoundingBoxType b = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(1, 2, 3), ezVec3Type(4, 5, 6));
    b.Grow(ezVec3Type(2, 4, 6));

    EZ_TEST_BOOL(b.m_vMin == ezVec3Type(-1, -2, -3));
    EZ_TEST_BOOL(b.m_vMax == ezVec3Type(6, 9, 12));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Contains (Point)")
  {
    ezBoundingBoxType b = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(0), ezVec3Type(0));

    EZ_TEST_BOOL(b.Contains(ezVec3Type(0)));
    EZ_TEST_BOOL(!b.Contains(ezVec3Type(1, 0, 0)));
    EZ_TEST_BOOL(!b.Contains(ezVec3Type(-1, 0, 0)));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Contains (Box)")
  {
    ezBoundingBoxType b1 = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(-3), ezVec3Type(3));
    ezBoundingBoxType b2 = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(-1), ezVec3Type(1));
    ezBoundingBoxType b3 = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(-1), ezVec3Type(4));

    EZ_TEST_BOOL(b1.Contains(b1));
    EZ_TEST_BOOL(b2.Contains(b2));
    EZ_TEST_BOOL(b3.Contains(b3));

    EZ_TEST_BOOL(b1.Contains(b2));
    EZ_TEST_BOOL(!b1.Contains(b3));

    EZ_TEST_BOOL(!b2.Contains(b1));
    EZ_TEST_BOOL(!b2.Contains(b3));

    EZ_TEST_BOOL(!b3.Contains(b1));
    EZ_TEST_BOOL(b3.Contains(b2));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Contains (Array)")
  {
    ezBoundingBoxType b = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(1), ezVec3Type(5));

    ezVec3Type v[4] = {ezVec3Type(0), ezVec3Type(1), ezVec3Type(5), ezVec3Type(6)};

    EZ_TEST_BOOL(!b.Contains(&v[0], 4, sizeof(ezVec3Type)));
    EZ_TEST_BOOL(b.Contains(&v[1], 2, sizeof(ezVec3Type)));
    EZ_TEST_BOOL(b.Contains(&v[2], 1, sizeof(ezVec3Type)));

    EZ_TEST_BOOL(!b.Contains(&v[1], 2, sizeof(ezVec3Type) * 2));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Contains (Sphere)")
  {
    ezBoundingBoxType b = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(1), ezVec3Type(5));

    EZ_TEST_BOOL(b.Contains(ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(3), 2)));
    EZ_TEST_BOOL(!b.Contains(ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(3), Type(2.1))));
    EZ_TEST_BOOL(!b.Contains(ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(8), 2)));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Overlaps (box)")
  {
    ezBoundingBoxType b1 = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(-3), ezVec3Type(3));
    ezBoundingBoxType b2 = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(-1), ezVec3Type(1));
    ezBoundingBoxType b3 = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(1), ezVec3Type(4));
    ezBoundingBoxType b4 = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(-4, 1, 1), ezVec3Type(4, 2, 2));

    EZ_TEST_BOOL(b1.Overlaps(b1));
    EZ_TEST_BOOL(b2.Overlaps(b2));
    EZ_TEST_BOOL(b3.Overlaps(b3));
    EZ_TEST_BOOL(b4.Overlaps(b4));

    EZ_TEST_BOOL(b1.Overlaps(b2));
    EZ_TEST_BOOL(b1.Overlaps(b3));
    EZ_TEST_BOOL(b1.Overlaps(b4));

    EZ_TEST_BOOL(!b2.Overlaps(b3));
    EZ_TEST_BOOL(!b2.Overlaps(b4));

    EZ_TEST_BOOL(b3.Overlaps(b4));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Overlaps (Array)")
  {
    ezBoundingBoxType b = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(1), ezVec3Type(5));

    ezVec3Type v[4] = {ezVec3Type(0), ezVec3Type(1), ezVec3Type(5), ezVec3Type(6)};

    EZ_TEST_BOOL(!b.Overlaps(&v[0], 1, sizeof(ezVec3Type)));
    EZ_TEST_BOOL(!b.Overlaps(&v[3], 1, sizeof(ezVec3Type)));

    EZ_TEST_BOOL(b.Overlaps(&v[0], 4, sizeof(ezVec3Type)));
    EZ_TEST_BOOL(b.Overlaps(&v[1], 2, sizeof(ezVec3Type)));
    EZ_TEST_BOOL(b.Overlaps(&v[2], 1, sizeof(ezVec3Type)));

    EZ_TEST_BOOL(b.Overlaps(&v[1], 2, sizeof(ezVec3Type) * 2));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Overlaps (Sphere)")
  {
    ezBoundingBoxType b = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(1), ezVec3Type(5));

    EZ_TEST_BOOL(b.Overlaps(ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(3), 2)));
    EZ_TEST_BOOL(b.Overlaps(ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(3), Type(2.1))));
    EZ_TEST_BOOL(!b.Overlaps(ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(8), 2)));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsIdentical, ==, !=")
  {
    ezBoundingBoxType b1, b2, b3;

    b1 = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(1), ezVec3Type(2));
    b2 = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(1), ezVec3Type(2));
    b3 = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(1), ezVec3Type(Type(2.01)));

    EZ_TEST_BOOL(b1.IsIdentical(b1));
    EZ_TEST_BOOL(b2.IsIdentical(b2));
    EZ_TEST_BOOL(b3.IsIdentical(b3));

    EZ_TEST_BOOL(b1 == b1);
    EZ_TEST_BOOL(b2 == b2);
    EZ_TEST_BOOL(b3 == b3);

    EZ_TEST_BOOL(b1.IsIdentical(b2));
    EZ_TEST_BOOL(b2.IsIdentical(b1));

    EZ_TEST_BOOL(!b1.IsIdentical(b3));
    EZ_TEST_BOOL(!b2.IsIdentical(b3));
    EZ_TEST_BOOL(!b3.IsIdentical(b1));
    EZ_TEST_BOOL(!b3.IsIdentical(b1));

    EZ_TEST_BOOL(b1 == b2);
    EZ_TEST_BOOL(b2 == b1);

    EZ_TEST_BOOL(b1 != b3);
    EZ_TEST_BOOL(b2 != b3);
    EZ_TEST_BOOL(b3 != b1);
    EZ_TEST_BOOL(b3 != b1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsEqual")
  {
    ezBoundingBoxType b1, b2;
    b1 = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(-1), ezVec3Type(1));
    b2 = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(-1), ezVec3Type(2));

    EZ_TEST_BOOL(!b1.IsEqual(b2));
    EZ_TEST_BOOL(!b1.IsEqual(b2, Type(0.5)));
    EZ_TEST_BOOL(b1.IsEqual(b2, 1));
    EZ_TEST_BOOL(b1.IsEqual(b2, 2));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetCenter")
  {
    ezBoundingBoxType b = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(3), ezVec3Type(7));

    EZ_TEST_BOOL(b.GetCenter() == ezVec3Type(5));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetExtents")
  {
    ezBoundingBoxType b = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(3), ezVec3Type(7));

    EZ_TEST_BOOL(b.GetExtents() == ezVec3Type(4));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetHalfExtents")
  {
    ezBoundingBoxType b = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(3), ezVec3Type(7));

    EZ_TEST_BOOL(b.GetHalfExtents() == ezVec3Type(2));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Translate")
  {
    ezBoundingBoxType b = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(3), ezVec3Type(5));

    b.Translate(ezVec3Type(1, 2, 3));

    EZ_TEST_BOOL(b.m_vMin == ezVec3Type(4, 5, 6));
    EZ_TEST_BOOL(b.m_vMax == ezVec3Type(6, 7, 8));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ScaleFromCenter")
  {
    {
      ezBoundingBoxType b = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(3), ezVec3Type(5));

      b.ScaleFromCenter(ezVec3Type(1, 2, 3));

      EZ_TEST_BOOL(b.m_vMin == ezVec3Type(3, 2, 1));
      EZ_TEST_BOOL(b.m_vMax == ezVec3Type(5, 6, 7));
    }
    {
      ezBoundingBoxType b = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(3), ezVec3Type(5));

      b.ScaleFromCenter(ezVec3Type(-1, -2, -3));

      EZ_TEST_BOOL(b.m_vMin == ezVec3Type(3, 2, 1));
      EZ_TEST_BOOL(b.m_vMax == ezVec3Type(5, 6, 7));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ScaleFromOrigin")
  {
    {
      ezBoundingBoxType b = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(3), ezVec3Type(5));

      b.ScaleFromOrigin(ezVec3Type(1, 2, 3));

      EZ_TEST_BOOL(b.m_vMin == ezVec3Type(3, 6, 9));
      EZ_TEST_BOOL(b.m_vMax == ezVec3Type(5, 10, 15));
    }
    {
      ezBoundingBoxType b = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(3), ezVec3Type(5));

      b.ScaleFromOrigin(ezVec3Type(-1, -2, -3));

      EZ_TEST_BOOL(b.m_vMin == ezVec3Type(-5, -10, -15));
      EZ_TEST_BOOL(b.m_vMax == ezVec3Type(-3, -6, -9));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TransformFromOrigin")
  {
    ezBoundingBoxType b = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(3), ezVec3Type(5));

    ezMat4Type m = ezMat4Type::MakeScaling(ezVec3Type(2));

    b.TransformFromOrigin(m);

    EZ_TEST_BOOL(b.m_vMin == ezVec3Type(6, 6, 6));
    EZ_TEST_BOOL(b.m_vMax == ezVec3Type(10, 10, 10));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TransformFromCenter")
  {
    ezBoundingBoxType b = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(3), ezVec3Type(5));

    ezMat4Type m = ezMat4Type::MakeScaling(ezVec3Type(2));

    b.TransformFromCenter(m);

    EZ_TEST_BOOL(b.m_vMin == ezVec3Type(2, 2, 2));
    EZ_TEST_BOOL(b.m_vMax == ezVec3Type(6, 6, 6));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetClampedPoint")
  {
    ezBoundingBoxType b = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(-1, -2, -3), ezVec3Type(1, 2, 3));

    EZ_TEST_BOOL(b.GetClampedPoint(ezVec3Type(-2, 0, 0)) == ezVec3Type(-1, 0, 0));
    EZ_TEST_BOOL(b.GetClampedPoint(ezVec3Type(2, 0, 0)) == ezVec3Type(1, 0, 0));

    EZ_TEST_BOOL(b.GetClampedPoint(ezVec3Type(0, -3, 0)) == ezVec3Type(0, -2, 0));
    EZ_TEST_BOOL(b.GetClampedPoint(ezVec3Type(0, 3, 0)) == ezVec3Type(0, 2, 0));

    EZ_TEST_BOOL(b.GetClampedPoint(ezVec3Type(0, 0, -4)) == ezVec3Type(0, 0, -3));
    EZ_TEST_BOOL(b.GetClampedPoint(ezVec3Type(0, 0, 4)) == ezVec3Type(0, 0, 3));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetDistanceTo (point)")
  {
    ezBoundingBoxType b = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(-1, -2, -3), ezVec3Type(1, 2, 3));

    EZ_TEST_BOOL(b.GetDistanceTo(ezVec3Type(-2, 0, 0)) == 1);
    EZ_TEST_BOOL(b.GetDistanceTo(ezVec3Type(2, 0, 0)) == 1);

    EZ_TEST_BOOL(b.GetDistanceTo(ezVec3Type(0, -4, 0)) == 2);
    EZ_TEST_BOOL(b.GetDistanceTo(ezVec3Type(0, 4, 0)) == 2);

    EZ_TEST_BOOL(b.GetDistanceTo(ezVec3Type(0, 0, -6)) == 3);
    EZ_TEST_BOOL(b.GetDistanceTo(ezVec3Type(0, 0, 6)) == 3);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetDistanceTo (Sphere)")
  {
    ezBoundingBoxType b = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(1), ezVec3Type(5));

    EZ_TEST_BOOL(b.GetDistanceTo(ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(3), 2)) < 0);
    EZ_TEST_BOOL(b.GetDistanceTo(ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(5), 1)) < 0);
    EZ_TEST_FLOAT(b.GetDistanceTo(ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(8, 2, 2), 2)), 1, Type(0.001));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetDistanceTo (box)")
  {
    ezBoundingBoxType b = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(1), ezVec3Type(5));

    ezBoundingBoxType b1, b2, b3;
    b1 = ezBoundingBoxType::MakeFromCenterAndHalfExtents(ezVec3Type(3), ezVec3Type(2));
    b2 = ezBoundingBoxType::MakeFromCenterAndHalfExtents(ezVec3Type(5), ezVec3Type(1));
    b3 = ezBoundingBoxType::MakeFromCenterAndHalfExtents(ezVec3Type(9, 2, 2), ezVec3Type(2));

    auto test1 = b.GetDistanceTo(b1);
    auto test2 = b.GetDistanceTo(b2);
    EZ_TEST_BOOL(b.GetDistanceTo(b1) <= 0);
    EZ_TEST_BOOL(b.GetDistanceTo(b2) <= 0);
    EZ_TEST_FLOAT(b.GetDistanceTo(b3), 2, Type(0.001));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetDistanceSquaredTo (point)")
  {
    ezBoundingBoxType b = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(-1, -2, -3), ezVec3Type(1, 2, 3));

    EZ_TEST_BOOL(b.GetDistanceSquaredTo(ezVec3Type(-2, 0, 0)) == 1);
    EZ_TEST_BOOL(b.GetDistanceSquaredTo(ezVec3Type(2, 0, 0)) == 1);

    EZ_TEST_BOOL(b.GetDistanceSquaredTo(ezVec3Type(0, -4, 0)) == 4);
    EZ_TEST_BOOL(b.GetDistanceSquaredTo(ezVec3Type(0, 4, 0)) == 4);

    EZ_TEST_BOOL(b.GetDistanceSquaredTo(ezVec3Type(0, 0, -6)) == 9);
    EZ_TEST_BOOL(b.GetDistanceSquaredTo(ezVec3Type(0, 0, 6)) == 9);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetDistanceSquaredTo (box)")
  {
    ezBoundingBoxType b = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(1), ezVec3Type(5));

    ezBoundingBoxType b1, b2, b3;
    b1 = ezBoundingBoxType::MakeFromCenterAndHalfExtents(ezVec3Type(3), ezVec3Type(2));
    b2 = ezBoundingBoxType::MakeFromCenterAndHalfExtents(ezVec3Type(5), ezVec3Type(1));
    b3 = ezBoundingBoxType::MakeFromCenterAndHalfExtents(ezVec3Type(9, 2, 2), ezVec3Type(2));

    EZ_TEST_BOOL(b.GetDistanceSquaredTo(b1) <= 0);
    EZ_TEST_BOOL(b.GetDistanceSquaredTo(b2) <= 0);
    EZ_TEST_FLOAT(b.GetDistanceSquaredTo(b3), 4, Type(0.001));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetBoundingSphere")
  {
    ezBoundingBoxType b;
    b = ezBoundingBoxType::MakeFromCenterAndHalfExtents(ezVec3Type(5, 4, 2), ezVec3Type(3));

    ezBoundingSphereType s = b.GetBoundingSphere();

    EZ_TEST_BOOL(s.m_vCenter == ezVec3Type(5, 4, 2));
    EZ_TEST_FLOAT(s.m_fRadius, ezVec3Type(3).GetLength(), Type(0.001));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetRayIntersection")
  {
    if (ezMath::SupportsInfinity<Type>())
    {
      const ezVec3Type c = ezVec3Type(10);

      ezBoundingBoxType b;
      b = ezBoundingBoxType::MakeFromCenterAndHalfExtents(c, ezVec3Type(2, 4, 8));

      for (Type x = b.m_vMin.x - Type(1); x < b.m_vMax.x + Type(1); x += Type(0.2))
      {
        for (Type y = b.m_vMin.y - Type(1); y < b.m_vMax.y + Type(1); y += Type(0.2))
        {
          for (Type z = b.m_vMin.z - Type(1); z < b.m_vMax.z + Type(1); z += Type(0.2))
          {
            const ezVec3Type v(x, y, z);

            if (b.Contains(v))
              continue;

            const ezVec3Type vTarget = b.GetClampedPoint(v);

            const ezVec3Type vDir = (vTarget - c).GetNormalized();

            const ezVec3Type vSource = vTarget + vDir * Type(3);

            Type f;
            ezVec3Type vi;
            EZ_TEST_BOOL(b.GetRayIntersection(vSource, -vDir, &f, &vi) == true);
            EZ_TEST_FLOAT(f, 3, Type(0.001));
            EZ_TEST_BOOL(vi.IsEqual(vTarget, Type(0.0001)));

            EZ_TEST_BOOL(b.GetRayIntersection(vSource, vDir, &f, &vi) == false);
            EZ_TEST_BOOL(b.GetRayIntersection(vTarget, vDir, &f, &vi) == false);
          }
        }
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetLineSegmentIntersection")
  {
    if (ezMath::SupportsInfinity<Type>())
    {
      const ezVec3Type c = ezVec3Type(10);

      ezBoundingBoxType b;
      b = ezBoundingBoxType::MakeFromCenterAndHalfExtents(c, ezVec3Type(2, 4, 8));

      for (Type x = b.m_vMin.x - Type(1); x < b.m_vMax.x + Type(1); x += Type(0.2))
      {
        for (Type y = b.m_vMin.y - Type(1); y < b.m_vMax.y + Type(1); y += Type(0.2))
        {
          for (Type z = b.m_vMin.z - Type(1); z < b.m_vMax.z + Type(1); z += Type(0.2))
          {
            const ezVec3Type v(x, y, z);

            if (b.Contains(v))
              continue;

            const ezVec3Type vTarget0 = b.GetClampedPoint(v);

            const ezVec3Type vDir = (vTarget0 - c).GetNormalized();

            const ezVec3Type vTarget = vTarget0 - vDir * Type(1);
            const ezVec3Type vSource = vTarget0 + vDir * Type(3);

            Type f;
            ezVec3Type vi;
            EZ_TEST_BOOL(b.GetLineSegmentIntersection(vSource, vTarget, &f, &vi) == true);
            EZ_TEST_FLOAT(f, Type(0.75), Type(0.001));
            EZ_TEST_BOOL(vi.IsEqual(vTarget0, Type(0.0001)));
          }
        }
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsNaN")
  {
    if (ezMath::SupportsNaN<Type>())
    {
      ezBoundingBoxType b;

      b = ezBoundingBoxType::MakeInvalid();
      EZ_TEST_BOOL(!b.IsNaN());

      b = ezBoundingBoxType::MakeInvalid();
      b.m_vMin.x = ezMath::NaN<Type>();
      EZ_TEST_BOOL(b.IsNaN());

      b = ezBoundingBoxType::MakeInvalid();
      b.m_vMin.y = ezMath::NaN<Type>();
      EZ_TEST_BOOL(b.IsNaN());

      b = ezBoundingBoxType::MakeInvalid();
      b.m_vMin.z = ezMath::NaN<Type>();
      EZ_TEST_BOOL(b.IsNaN());

      b = ezBoundingBoxType::MakeInvalid();
      b.m_vMax.x = ezMath::NaN<Type>();
      EZ_TEST_BOOL(b.IsNaN());

      b = ezBoundingBoxType::MakeInvalid();
      b.m_vMax.y = ezMath::NaN<Type>();
      EZ_TEST_BOOL(b.IsNaN());

      b = ezBoundingBoxType::MakeInvalid();
      b.m_vMax.z = ezMath::NaN<Type>();
      EZ_TEST_BOOL(b.IsNaN());
    }
  }
}

EZ_CREATE_SIMPLE_TEST(Math, BoundingBoxf)
{
  TestBoundingBox<float>();
}

EZ_CREATE_SIMPLE_TEST(Math, BoundingBoxd)
{
  TestBoundingBox<double>();
}
