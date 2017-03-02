#include <PCH.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Math/Random.h>
#include <Foundation/SimdMath/SimdBBoxSphere.h>
#include <Foundation/SimdMath/SimdConversion.h>

EZ_CREATE_SIMPLE_TEST(SimdMath, SimdBBoxSphere)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
    ezSimdBBoxSphere b(ezSimdVec4f(-1, -2, -3), ezSimdVec4f(1, 2, 3), 2);

    EZ_TEST_BOOL((b.m_CenterAndRadius == ezSimdVec4f(-1, -2, -3, 2)).AllSet<4>());
    EZ_TEST_BOOL((b.m_BoxHalfExtents == ezSimdVec4f(1, 2, 3)).AllSet<3>());

    ezSimdBBox box(ezSimdVec4f(1, 1, 1), ezSimdVec4f(3, 3, 3));
    ezSimdBSphere sphere(ezSimdVec4f(2, 2, 2), 1);

    b = ezSimdBBoxSphere(box, sphere);

    EZ_TEST_BOOL((b.m_CenterAndRadius == ezSimdVec4f(2, 2, 2, 1)).AllSet<4>());
    EZ_TEST_BOOL((b.m_BoxHalfExtents == ezSimdVec4f(1, 1, 1)).AllSet<3>());
    EZ_TEST_BOOL(b.GetBox() == box);
    EZ_TEST_BOOL(b.GetSphere() == sphere);

    b = ezSimdBBoxSphere(box);

    EZ_TEST_BOOL(b.m_CenterAndRadius.IsEqual(ezSimdVec4f(2, 2, 2, ezMath::Sqrt(3.0f)), 0.00001f).AllSet<4>());
    EZ_TEST_BOOL((b.m_BoxHalfExtents == ezSimdVec4f(1, 1, 1)).AllSet<3>());
    EZ_TEST_BOOL(b.GetBox() == box);

    b = ezSimdBBoxSphere(sphere);

    EZ_TEST_BOOL((b.m_CenterAndRadius == ezSimdVec4f(2, 2, 2, 1)).AllSet<4>());
    EZ_TEST_BOOL((b.m_BoxHalfExtents == ezSimdVec4f(1, 1, 1)).AllSet<3>());
    EZ_TEST_BOOL(b.GetSphere() == sphere);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetInvalid")
  {
    ezSimdBBoxSphere b;
    b.SetInvalid();

    EZ_TEST_BOOL(!b.IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsNaN")
  {
    if (ezMath::BasicType<float>::SupportsNaN())
    {
      ezSimdBBoxSphere b;

      b.SetInvalid();
      EZ_TEST_BOOL(!b.IsNaN());

      b.SetInvalid();
      b.m_CenterAndRadius.SetX(ezMath::BasicType<float>::GetNaN());
      EZ_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_CenterAndRadius.SetY(ezMath::BasicType<float>::GetNaN());
      EZ_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_CenterAndRadius.SetZ(ezMath::BasicType<float>::GetNaN());
      EZ_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_CenterAndRadius.SetW(ezMath::BasicType<float>::GetNaN());
      EZ_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_BoxHalfExtents.SetX(ezMath::BasicType<float>::GetNaN());
      EZ_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_BoxHalfExtents.SetY(ezMath::BasicType<float>::GetNaN());
      EZ_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_BoxHalfExtents.SetZ(ezMath::BasicType<float>::GetNaN());
      EZ_TEST_BOOL(b.IsNaN());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFromPoints")
  {
    ezSimdVec4f p[6] =
    {
      ezSimdVec4f(-4, 0, 0),
      ezSimdVec4f(5, 0, 0),
      ezSimdVec4f(0, -6, 0),
      ezSimdVec4f(0, 7, 0),
      ezSimdVec4f(0, 0, -8),
      ezSimdVec4f(0, 0, 9),
    };

    ezSimdBBoxSphere b;
    b.SetFromPoints(p, 6);

    EZ_TEST_BOOL((b.m_CenterAndRadius == ezSimdVec4f(0.5, 0.5, 0.5)).AllSet<3>());
    EZ_TEST_BOOL((b.m_BoxHalfExtents == ezSimdVec4f(4.5, 6.5, 8.5)).AllSet<3>());
    EZ_TEST_BOOL(b.m_CenterAndRadius.w().IsEqual(ezSimdVec4f(0.5, 0.5, 8.5).GetLength<3>(), 0.00001f));
    EZ_TEST_BOOL(b.m_CenterAndRadius.w() <= b.m_BoxHalfExtents.GetLength<3>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExpandToInclude")
  {
    ezSimdBBoxSphere b1; b1.SetInvalid();
    ezSimdBBoxSphere b2(ezSimdBBox(ezSimdVec4f(2, 2, 2), ezSimdVec4f(4, 4, 4)));

    b1.ExpandToInclude(b2);
    EZ_TEST_BOOL(b1 == b2);

    ezSimdBSphere sphere(ezSimdVec4f(2, 2, 2), 2);
    b2 = ezSimdBBoxSphere(sphere);

    b1.ExpandToInclude(b2);
    EZ_TEST_BOOL(b1 != b2);

    EZ_TEST_BOOL((b1.m_CenterAndRadius == ezSimdVec4f(2, 2, 2)).AllSet<3>());
    EZ_TEST_BOOL((b1.m_BoxHalfExtents == ezSimdVec4f(2, 2, 2)).AllSet<3>());
    EZ_TEST_FLOAT(b1.m_CenterAndRadius.w(), ezMath::Sqrt(3.0f) * 2.0f, 0.00001f);
    EZ_TEST_BOOL(b1.m_CenterAndRadius.w() <= b1.m_BoxHalfExtents.GetLength<3>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Transform")
  {
    ezSimdBBoxSphere b(ezSimdVec4f(1), ezSimdVec4f(5), 5);

    ezSimdTransform t(ezSimdVec4f(1, 1, 1), ezSimdQuat::Identity(), ezSimdVec4f(2, 3, -2));

    b.Transform(t);

    EZ_TEST_BOOL((b.m_CenterAndRadius == ezSimdVec4f(3, 4, -1, 15)).AllSet<4>());
    EZ_TEST_BOOL((b.m_BoxHalfExtents == ezSimdVec4f(10, 15, 10)).AllSet<3>());

    // verification
    ezRandom rnd;
    rnd.Initialize(0x736454);

    ezDynamicArray<ezSimdVec4f, ezAlignedAllocatorWrapper> points;
    points.SetCountUninitialized(10);
    float fSize = 10;

    for (ezUInt32 i = 0; i < points.GetCount(); ++i)
    {
      float x = (float)rnd.DoubleMinMax(-fSize, fSize);
      float y = (float)rnd.DoubleMinMax(-fSize, fSize);
      float z = (float)rnd.DoubleMinMax(-fSize, fSize);
      points[i] = ezSimdVec4f(x, y, z);
    }

    b.SetFromPoints(points.GetData(), points.GetCount());

    t.m_Rotation.SetFromAxisAndAngle(ezSimdVec4f(0, 0, 1), ezAngle::Degree(-30));
    b.Transform(t);

    for (ezUInt32 i = 0; i < points.GetCount(); ++i)
    {
      ezSimdVec4f tp = t.TransformPosition(points[i]);

      ezSimdFloat boxDist = b.GetBox().GetDistanceTo(tp);
      EZ_TEST_BOOL(boxDist < ezMath::BasicType<float>::DefaultEpsilon());

      ezSimdFloat sphereDist = b.GetSphere().GetDistanceTo(tp);
      EZ_TEST_BOOL(sphereDist < ezMath::BasicType<float>::DefaultEpsilon());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Comparison")
  {
    ezSimdBBoxSphere b1(ezSimdBBox(ezSimdVec4f(5, 0, 0), ezSimdVec4f(1, 2, 3)));
    ezSimdBBoxSphere b2(ezSimdBBox(ezSimdVec4f(6, 0, 0), ezSimdVec4f(1, 2, 3)));

    EZ_TEST_BOOL(b1 == ezSimdBBoxSphere(ezSimdBBox(ezSimdVec4f(5, 0, 0), ezSimdVec4f(1, 2, 3))));
    EZ_TEST_BOOL(b1 != b2);
  }
}
