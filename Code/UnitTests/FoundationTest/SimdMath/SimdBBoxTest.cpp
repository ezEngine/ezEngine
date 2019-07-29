#include <FoundationTestPCH.h>

#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/SimdMath/SimdConversion.h>

EZ_CREATE_SIMPLE_TEST(SimdMath, SimdBBox)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
    ezSimdBBox b(ezSimdVec4f(-1, -2, -3), ezSimdVec4f(1, 2, 3));

    EZ_TEST_BOOL((b.m_Min == ezSimdVec4f(-1, -2, -3)).AllSet<3>());
    EZ_TEST_BOOL((b.m_Max == ezSimdVec4f(1, 2, 3)).AllSet<3>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetInvalid")
  {
    ezSimdBBox b;
    b.SetInvalid();

    EZ_TEST_BOOL(!b.IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsNaN")
  {
    ezSimdBBox b;

    b.SetInvalid();
    EZ_TEST_BOOL(!b.IsNaN());

    b.SetInvalid();
    b.m_Min.SetX(ezMath::NaN<ezMathTestType>());
    EZ_TEST_BOOL(b.IsNaN());

    b.SetInvalid();
    b.m_Min.SetY(ezMath::NaN<ezMathTestType>());
    EZ_TEST_BOOL(b.IsNaN());

    b.SetInvalid();
    b.m_Min.SetZ(ezMath::NaN<ezMathTestType>());
    EZ_TEST_BOOL(b.IsNaN());

    b.SetInvalid();
    b.m_Max.SetX(ezMath::NaN<ezMathTestType>());
    EZ_TEST_BOOL(b.IsNaN());

    b.SetInvalid();
    b.m_Max.SetY(ezMath::NaN<ezMathTestType>());
    EZ_TEST_BOOL(b.IsNaN());

    b.SetInvalid();
    b.m_Max.SetZ(ezMath::NaN<ezMathTestType>());
    EZ_TEST_BOOL(b.IsNaN());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetCenterAndHalfExtents")
  {
    ezSimdBBox b;
    b.SetCenterAndHalfExtents(ezSimdVec4f(1, 2, 3), ezSimdVec4f(4, 5, 6));

    EZ_TEST_BOOL((b.m_Min == ezSimdVec4f(-3, -3, -3)).AllSet<3>());
    EZ_TEST_BOOL((b.m_Max == ezSimdVec4f(5, 7, 9)).AllSet<3>());

    EZ_TEST_BOOL((b.GetCenter() == ezSimdVec4f(1, 2, 3)).AllSet<3>());
    EZ_TEST_BOOL((b.GetExtents() == ezSimdVec4f(8, 10, 12)).AllSet<3>());
    EZ_TEST_BOOL((b.GetHalfExtents() == ezSimdVec4f(4, 5, 6)).AllSet<3>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFromPoints")
  {
    ezSimdVec4f p[6] = {
        ezSimdVec4f(-4, 0, 0), ezSimdVec4f(5, 0, 0),  ezSimdVec4f(0, -6, 0),
        ezSimdVec4f(0, 7, 0),  ezSimdVec4f(0, 0, -8), ezSimdVec4f(0, 0, 9),
    };

    ezSimdBBox b;
    b.SetFromPoints(p, 6);

    EZ_TEST_BOOL((b.m_Min == ezSimdVec4f(-4, -6, -8)).AllSet<3>());
    EZ_TEST_BOOL((b.m_Max == ezSimdVec4f(5, 7, 9)).AllSet<3>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExpandToInclude (Point)")
  {
    ezSimdBBox b;
    b.SetInvalid();
    b.ExpandToInclude(ezSimdVec4f(1, 2, 3));

    EZ_TEST_BOOL((b.m_Min == ezSimdVec4f(1, 2, 3)).AllSet<3>());
    EZ_TEST_BOOL((b.m_Max == ezSimdVec4f(1, 2, 3)).AllSet<3>());


    b.ExpandToInclude(ezSimdVec4f(2, 3, 4));

    EZ_TEST_BOOL((b.m_Min == ezSimdVec4f(1, 2, 3)).AllSet<3>());
    EZ_TEST_BOOL((b.m_Max == ezSimdVec4f(2, 3, 4)).AllSet<3>());

    b.ExpandToInclude(ezSimdVec4f(0, 1, 2));

    EZ_TEST_BOOL((b.m_Min == ezSimdVec4f(0, 1, 2)).AllSet<3>());
    EZ_TEST_BOOL((b.m_Max == ezSimdVec4f(2, 3, 4)).AllSet<3>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExpandToInclude (array)")
  {
    ezSimdVec4f v[4] = {ezSimdVec4f(1, 1, 1), ezSimdVec4f(-1, -1, -1), ezSimdVec4f(2, 2, 2), ezSimdVec4f(4, 4, 4)};

    ezSimdBBox b;
    b.SetInvalid();
    b.ExpandToInclude(v, 2, sizeof(ezSimdVec4f) * 2);

    EZ_TEST_BOOL((b.m_Min == ezSimdVec4f(1, 1, 1)).AllSet<3>());
    EZ_TEST_BOOL((b.m_Max == ezSimdVec4f(2, 2, 2)).AllSet<3>());

    b.ExpandToInclude(v, 4);

    EZ_TEST_BOOL((b.m_Min == ezSimdVec4f(-1, -1, -1)).AllSet<3>());
    EZ_TEST_BOOL((b.m_Max == ezSimdVec4f(4, 4, 4)).AllSet<3>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExpandToInclude (Box)")
  {
    ezSimdBBox b1(ezSimdVec4f(-1, -2, -3), ezSimdVec4f(1, 2, 3));
    ezSimdBBox b2(ezSimdVec4f(0), ezSimdVec4f(4, 5, 6));

    b1.ExpandToInclude(b2);

    EZ_TEST_BOOL((b1.m_Min == ezSimdVec4f(-1, -2, -3)).AllSet<3>());
    EZ_TEST_BOOL((b1.m_Max == ezSimdVec4f(4, 5, 6)).AllSet<3>());

    ezSimdBBox b3;
    b3.SetInvalid();
    b3.ExpandToInclude(b1);
    EZ_TEST_BOOL(b3 == b1);

    b2.m_Min = ezSimdVec4f(-4, -5, -6);
    b2.m_Max.SetZero();

    b1.ExpandToInclude(b2);

    EZ_TEST_BOOL((b1.m_Min == ezSimdVec4f(-4, -5, -6)).AllSet<3>());
    EZ_TEST_BOOL((b1.m_Max == ezSimdVec4f(4, 5, 6)).AllSet<3>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExpandToCube")
  {
    ezSimdBBox b;
    b.SetCenterAndHalfExtents(ezSimdVec4f(1, 2, 3), ezSimdVec4f(4, 5, 6));

    b.ExpandToCube();

    EZ_TEST_BOOL((b.GetCenter() == ezSimdVec4f(1, 2, 3)).AllSet<3>());
    EZ_TEST_BOOL((b.GetHalfExtents() == ezSimdVec4f(6, 6, 6)).AllSet<3>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Contains (Point)")
  {
    ezSimdBBox b(ezSimdVec4f(0), ezSimdVec4f(0));

    EZ_TEST_BOOL(b.Contains(ezSimdVec4f(0)));
    EZ_TEST_BOOL(!b.Contains(ezSimdVec4f(1, 0, 0)));
    EZ_TEST_BOOL(!b.Contains(ezSimdVec4f(-1, 0, 0)));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Contains (Box)")
  {
    ezSimdBBox b1(ezSimdVec4f(-3), ezSimdVec4f(3));
    ezSimdBBox b2(ezSimdVec4f(-1), ezSimdVec4f(1));
    ezSimdBBox b3(ezSimdVec4f(-1), ezSimdVec4f(4));

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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Contains (Sphere)")
  {
    ezSimdBBox b(ezSimdVec4f(1), ezSimdVec4f(5));

    EZ_TEST_BOOL(b.Contains(ezSimdBSphere(ezSimdVec4f(3), 2)));
    EZ_TEST_BOOL(!b.Contains(ezSimdBSphere(ezSimdVec4f(3), 2.1f)));
    EZ_TEST_BOOL(!b.Contains(ezSimdBSphere(ezSimdVec4f(8), 2)));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Overlaps (box)")
  {
    ezSimdBBox b1(ezSimdVec4f(-3), ezSimdVec4f(3));
    ezSimdBBox b2(ezSimdVec4f(-1), ezSimdVec4f(1));
    ezSimdBBox b3(ezSimdVec4f(1), ezSimdVec4f(4));
    ezSimdBBox b4(ezSimdVec4f(-4, 1, 1), ezSimdVec4f(4, 2, 2));

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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Overlaps (Sphere)")
  {
    ezSimdBBox b(ezSimdVec4f(1), ezSimdVec4f(5));

    EZ_TEST_BOOL(b.Overlaps(ezSimdBSphere(ezSimdVec4f(3), 2)));
    EZ_TEST_BOOL(b.Overlaps(ezSimdBSphere(ezSimdVec4f(3), 2.1f)));
    EZ_TEST_BOOL(!b.Overlaps(ezSimdBSphere(ezSimdVec4f(8), 2)));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Grow")
  {
    ezSimdBBox b(ezSimdVec4f(1, 2, 3), ezSimdVec4f(4, 5, 6));
    b.Grow(ezSimdVec4f(2, 4, 6));

    EZ_TEST_BOOL((b.m_Min == ezSimdVec4f(-1, -2, -3)).AllSet<3>());
    EZ_TEST_BOOL((b.m_Max == ezSimdVec4f(6, 9, 12)).AllSet<3>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Transform")
  {
    ezSimdBBox b(ezSimdVec4f(3), ezSimdVec4f(5));

    ezSimdTransform t(ezSimdVec4f(4, 5, 6));
    t.m_Rotation.SetFromAxisAndAngle(ezSimdVec4f(0, 0, 1), ezAngle::Degree(90));
    t.m_Scale = ezSimdVec4f(1, -2, -4);

    b.Transform(t);

    EZ_TEST_BOOL(b.m_Min.IsEqual(ezSimdVec4f(10, 8, -14), 0.00001f).AllSet<3>());
    EZ_TEST_BOOL(b.m_Max.IsEqual(ezSimdVec4f(14, 10, -6), 0.00001f).AllSet<3>());

    t.m_Rotation.SetFromAxisAndAngle(ezSimdVec4f(0, 0, 1), ezAngle::Degree(-30));

    b.m_Min = ezSimdVec4f(3);
    b.m_Max = ezSimdVec4f(5);
    b.Transform(t);

    // reference
    ezBoundingBox referenceBox(ezVec3(3), ezVec3(5));
    {
      ezQuat q;
      q.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(-30));

      ezTransform referenceTransform(ezVec3(4, 5, 6), q, ezVec3(1, -2, -4));

      referenceBox.TransformFromOrigin(referenceTransform.GetAsMat4());
    }

    EZ_TEST_BOOL(b.m_Min.IsEqual(ezSimdConversion::ToVec3(referenceBox.m_vMin), 0.00001f).AllSet<3>());
    EZ_TEST_BOOL(b.m_Max.IsEqual(ezSimdConversion::ToVec3(referenceBox.m_vMax), 0.00001f).AllSet<3>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetClampedPoint")
  {
    ezSimdBBox b(ezSimdVec4f(-1, -2, -3), ezSimdVec4f(1, 2, 3));

    EZ_TEST_BOOL((b.GetClampedPoint(ezSimdVec4f(-2, 0, 0)) == ezSimdVec4f(-1, 0, 0)).AllSet<3>());
    EZ_TEST_BOOL((b.GetClampedPoint(ezSimdVec4f(2, 0, 0)) == ezSimdVec4f(1, 0, 0)).AllSet<3>());

    EZ_TEST_BOOL((b.GetClampedPoint(ezSimdVec4f(0, -3, 0)) == ezSimdVec4f(0, -2, 0)).AllSet<3>());
    EZ_TEST_BOOL((b.GetClampedPoint(ezSimdVec4f(0, 3, 0)) == ezSimdVec4f(0, 2, 0)).AllSet<3>());

    EZ_TEST_BOOL((b.GetClampedPoint(ezSimdVec4f(0, 0, -4)) == ezSimdVec4f(0, 0, -3)).AllSet<3>());
    EZ_TEST_BOOL((b.GetClampedPoint(ezSimdVec4f(0, 0, 4)) == ezSimdVec4f(0, 0, 3)).AllSet<3>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetDistanceSquaredTo (point)")
  {
    ezSimdBBox b(ezSimdVec4f(-1, -2, -3), ezSimdVec4f(1, 2, 3));

    EZ_TEST_BOOL(b.GetDistanceSquaredTo(ezSimdVec4f(-2, 0, 0)) == 1.0f);
    EZ_TEST_BOOL(b.GetDistanceSquaredTo(ezSimdVec4f(2, 0, 0)) == 1.0f);

    EZ_TEST_BOOL(b.GetDistanceSquaredTo(ezSimdVec4f(0, -4, 0)) == 4.0f);
    EZ_TEST_BOOL(b.GetDistanceSquaredTo(ezSimdVec4f(0, 4, 0)) == 4.0f);

    EZ_TEST_BOOL(b.GetDistanceSquaredTo(ezSimdVec4f(0, 0, -6)) == 9.0f);
    EZ_TEST_BOOL(b.GetDistanceSquaredTo(ezSimdVec4f(0, 0, 6)) == 9.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetDistanceTo (point)")
  {
    ezSimdBBox b(ezSimdVec4f(-1, -2, -3), ezSimdVec4f(1, 2, 3));

    EZ_TEST_BOOL(b.GetDistanceTo(ezSimdVec4f(-2, 0, 0)) == 1.0f);
    EZ_TEST_BOOL(b.GetDistanceTo(ezSimdVec4f(2, 0, 0)) == 1.0f);

    EZ_TEST_BOOL(b.GetDistanceTo(ezSimdVec4f(0, -4, 0)) == 2.0f);
    EZ_TEST_BOOL(b.GetDistanceTo(ezSimdVec4f(0, 4, 0)) == 2.0f);

    EZ_TEST_BOOL(b.GetDistanceTo(ezSimdVec4f(0, 0, -6)) == 3.0f);
    EZ_TEST_BOOL(b.GetDistanceTo(ezSimdVec4f(0, 0, 6)) == 3.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Comparison")
  {
    ezSimdBBox b1(ezSimdVec4f(5, 0, 0), ezSimdVec4f(1, 2, 3));
    ezSimdBBox b2(ezSimdVec4f(6, 0, 0), ezSimdVec4f(1, 2, 3));

    EZ_TEST_BOOL(b1 == ezSimdBBox(ezSimdVec4f(5, 0, 0), ezSimdVec4f(1, 2, 3)));
    EZ_TEST_BOOL(b1 != b2);
  }
}
