#include <FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdBSphere.h>

EZ_CREATE_SIMPLE_TEST(SimdMath, SimdBSphere)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
    ezSimdBSphere s(ezSimdVec4f(1, 2, 3), 4);

    EZ_TEST_BOOL((s.m_CenterAndRadius == ezSimdVec4f(1, 2, 3, 4)).AllSet());

    EZ_TEST_BOOL((s.GetCenter() == ezSimdVec4f(1, 2, 3)).AllSet<3>());
    EZ_TEST_BOOL(s.GetRadius() == 4.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetInvalid / IsValid")
  {
    ezSimdBSphere s(ezSimdVec4f(1, 2, 3), 4);

    EZ_TEST_BOOL(s.IsValid());

    s.SetInvalid();

    EZ_TEST_BOOL(!s.IsValid());
    EZ_TEST_BOOL(!s.IsNaN());

    s = ezSimdBSphere(ezSimdVec4f(1, 2, 3), ezMath::BasicType<float>::GetNaN());
    EZ_TEST_BOOL(s.IsNaN());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExpandToInclude(Point)")
  {
    ezSimdBSphere s(ezSimdVec4f::ZeroVector(), 0.0f);

    s.ExpandToInclude(ezSimdVec4f(3, 0, 0));

    EZ_TEST_BOOL((s.m_CenterAndRadius == ezSimdVec4f(0, 0, 0, 3)).AllSet());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExpandToInclude(array)")
  {
    ezSimdBSphere s(ezSimdVec4f(2, 2, 0), 0.0f);

    ezSimdVec4f p[4] = {ezSimdVec4f(0, 2, 0), ezSimdVec4f(4, 2, 0), ezSimdVec4f(2, 0, 0), ezSimdVec4f(2, 4, 0)};

    s.ExpandToInclude(p, 4);

    EZ_TEST_BOOL((s.m_CenterAndRadius == ezSimdVec4f(2, 2, 0, 2)).AllSet());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExpandToInclude (sphere)")
  {
    ezSimdBSphere s1(ezSimdVec4f(5, 0, 0), 1);
    ezSimdBSphere s2(ezSimdVec4f(6, 0, 0), 1);
    ezSimdBSphere s3(ezSimdVec4f(5, 0, 0), 2);

    s1.ExpandToInclude(s2);
    EZ_TEST_BOOL((s1.m_CenterAndRadius == ezSimdVec4f(5, 0, 0, 2)).AllSet());

    s1.ExpandToInclude(s3);
    EZ_TEST_BOOL((s1.m_CenterAndRadius == ezSimdVec4f(5, 0, 0, 2)).AllSet());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Transform")
  {
    ezSimdBSphere s(ezSimdVec4f(5, 0, 0), 2);

    ezSimdTransform t(ezSimdVec4f(4, 5, 6));
    t.m_Rotation.SetFromAxisAndAngle(ezSimdVec4f(0, 0, 1), ezAngle::Degree(90));
    t.m_Scale = ezSimdVec4f(1, -2, -4);

    s.Transform(t);
    EZ_TEST_BOOL(s.m_CenterAndRadius.IsEqual(ezSimdVec4f(4, 10, 6, 8), ezSimdFloat(ezMath::BasicType<float>::SmallEpsilon())).AllSet());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetDistanceTo (point)")
  {
    ezSimdBSphere s(ezSimdVec4f(5, 0, 0), 2);

    EZ_TEST_BOOL(s.GetDistanceTo(ezSimdVec4f(5, 0, 0)) == -2.0f);
    EZ_TEST_BOOL(s.GetDistanceTo(ezSimdVec4f(7, 0, 0)) == 0.0f);
    EZ_TEST_BOOL(s.GetDistanceTo(ezSimdVec4f(9, 0, 0)) == 2.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetDistanceTo (sphere)")
  {
    ezSimdBSphere s1(ezSimdVec4f(5, 0, 0), 2);
    ezSimdBSphere s2(ezSimdVec4f(10, 0, 0), 3);
    ezSimdBSphere s3(ezSimdVec4f(10, 0, 0), 1);

    EZ_TEST_BOOL(s1.GetDistanceTo(s2) == 0.0f);
    EZ_TEST_BOOL(s1.GetDistanceTo(s3) == 2.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Contains (point)")
  {
    ezSimdBSphere s(ezSimdVec4f(5, 0, 0), 2.0f);

    EZ_TEST_BOOL(s.Contains(ezSimdVec4f(3, 0, 0)));
    EZ_TEST_BOOL(s.Contains(ezSimdVec4f(5, 0, 0)));
    EZ_TEST_BOOL(s.Contains(ezSimdVec4f(6, 0, 0)));
    EZ_TEST_BOOL(s.Contains(ezSimdVec4f(7, 0, 0)));

    EZ_TEST_BOOL(!s.Contains(ezSimdVec4f(2, 0, 0)));
    EZ_TEST_BOOL(!s.Contains(ezSimdVec4f(8, 0, 0)));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Contains (sphere)")
  {
    ezSimdBSphere s1(ezSimdVec4f(5, 0, 0), 2);
    ezSimdBSphere s2(ezSimdVec4f(6, 0, 0), 1);
    ezSimdBSphere s3(ezSimdVec4f(6, 0, 0), 2);

    EZ_TEST_BOOL(s1.Contains(s1));
    EZ_TEST_BOOL(s2.Contains(s2));
    EZ_TEST_BOOL(s3.Contains(s3));

    EZ_TEST_BOOL(s1.Contains(s2));
    EZ_TEST_BOOL(!s1.Contains(s3));

    EZ_TEST_BOOL(!s2.Contains(s3));
    EZ_TEST_BOOL(s3.Contains(s2));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Overlaps (sphere)")
  {
    ezSimdBSphere s1(ezSimdVec4f(5, 0, 0), 2);
    ezSimdBSphere s2(ezSimdVec4f(6, 0, 0), 2);
    ezSimdBSphere s3(ezSimdVec4f(8, 0, 0), 1);

    EZ_TEST_BOOL(s1.Overlaps(s1));
    EZ_TEST_BOOL(s2.Overlaps(s2));
    EZ_TEST_BOOL(s3.Overlaps(s3));

    EZ_TEST_BOOL(s1.Overlaps(s2));
    EZ_TEST_BOOL(!s1.Overlaps(s3));

    EZ_TEST_BOOL(s2.Overlaps(s3));
    EZ_TEST_BOOL(s3.Overlaps(s2));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetClampedPoint")
  {
    ezSimdBSphere s(ezSimdVec4f(1, 2, 3), 2.0f);

    EZ_TEST_BOOL(s.GetClampedPoint(ezSimdVec4f(2, 2, 3)).IsEqual(ezSimdVec4f(2, 2, 3), 0.001f).AllSet<3>());
    EZ_TEST_BOOL(s.GetClampedPoint(ezSimdVec4f(5, 2, 3)).IsEqual(ezSimdVec4f(3, 2, 3), 0.001f).AllSet<3>());
    EZ_TEST_BOOL(s.GetClampedPoint(ezSimdVec4f(1, 7, 3)).IsEqual(ezSimdVec4f(1, 4, 3), 0.001f).AllSet<3>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Comparison")
  {
    ezSimdBSphere s1(ezSimdVec4f(5, 0, 0), 2);
    ezSimdBSphere s2(ezSimdVec4f(6, 0, 0), 1);

    EZ_TEST_BOOL(s1 == ezSimdBSphere(ezSimdVec4f(5, 0, 0), 2));
    EZ_TEST_BOOL(s1 != s2);
  }
}
