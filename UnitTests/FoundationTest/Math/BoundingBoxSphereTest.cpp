#include <PCH.h>
#include <Foundation/Math/BoundingBoxSphere.h>

EZ_CREATE_SIMPLE_TEST(Math, BoundingBoxSphere)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
    ezBoundingBoxSphereT b(ezVec3T(-1,-2,-3), ezVec3T(1, 2, 3), 2);

    EZ_TEST_BOOL(b.m_vCenter == ezVec3T(-1,-2,-3));
    EZ_TEST_BOOL(b.m_vBoxHalfExtends == ezVec3T( 1, 2, 3));
    EZ_TEST_BOOL(b.m_fSphereRadius == 2);

    ezBoundingBoxT box(ezVec3T(1, 1, 1), ezVec3T(3, 3, 3));
    ezBoundingSphereT sphere(ezVec3T(2, 2, 2), 1);

    b = ezBoundingBoxSphereT(box, sphere);

    EZ_TEST_BOOL(b.m_vCenter == ezVec3T(2, 2, 2));
    EZ_TEST_BOOL(b.m_vBoxHalfExtends == ezVec3T(1, 1, 1));
    EZ_TEST_BOOL(b.m_fSphereRadius == 1);
    EZ_TEST_BOOL(b.GetBox() == box);
    EZ_TEST_BOOL(b.GetSphere() == sphere);

    b = ezBoundingBoxSphereT(box);

    EZ_TEST_BOOL(b.m_vCenter == ezVec3T(2, 2, 2));
    EZ_TEST_BOOL(b.m_vBoxHalfExtends == ezVec3T(1, 1, 1));
    EZ_TEST_FLOAT(b.m_fSphereRadius, ezMath::Sqrt(ezMathTestType(3)), 0.00001f);
    EZ_TEST_BOOL(b.GetBox() == box);

    b = ezBoundingBoxSphereT(sphere);

    EZ_TEST_BOOL(b.m_vCenter == ezVec3T(2, 2, 2));
    EZ_TEST_BOOL(b.m_vBoxHalfExtends == ezVec3T(1, 1, 1));
    EZ_TEST_BOOL(b.m_fSphereRadius == 1);
    EZ_TEST_BOOL(b.GetSphere() == sphere);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFromPoints")
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

    ezBoundingBoxSphereT b;
    b.SetFromPoints(p, 6);

    EZ_TEST_BOOL(b.m_vCenter == ezVec3T(0.5, 0.5, 0.5));
    EZ_TEST_BOOL(b.m_vBoxHalfExtends == ezVec3T(4.5, 6.5, 8.5));
    EZ_TEST_FLOAT(b.m_fSphereRadius, ezVec3T(0.5, 0.5, 8.5).GetLength(), 0.00001f);
    EZ_TEST_BOOL(b.m_fSphereRadius <= b.m_vBoxHalfExtends.GetLength());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetInvalid")
  {
    ezBoundingBoxSphereT b;
    b.SetInvalid();

    EZ_TEST_BOOL(!b.IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExpandToInclude")
  {
    ezBoundingBoxSphereT b1; b1.SetInvalid();
    ezBoundingBoxSphereT b2(ezVec3T(0, 0, 0), ezVec3T(2, 2, 2), 2);

    b1.ExpandToInclude(b2);
    EZ_TEST_BOOL(b1 == b2);

    ezBoundingSphereT sphere(ezVec3T(2, 2, 2), 2);
    b2 = ezBoundingBoxSphereT(sphere);

    b1.ExpandToInclude(b2);
    EZ_TEST_BOOL(b1 != b2);

    EZ_TEST_BOOL(b1.m_vCenter == ezVec3T(1, 1, 1));
    EZ_TEST_BOOL(b1.m_vBoxHalfExtends == ezVec3T(3, 3, 3));
    EZ_TEST_FLOAT(b1.m_fSphereRadius, ezMath::Sqrt(ezMathTestType(3)) + 2, 0.00001f);
    EZ_TEST_BOOL(b1.m_fSphereRadius <= b1.m_vBoxHalfExtends.GetLength());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Transform")
  {
    ezBoundingBoxSphereT b(ezVec3T(1), ezVec3T(5), 5);

    ezMat4T m;
    m.SetScalingMatrix(ezVec3T(2, 3, -2));
    m.SetTranslationVector(ezVec3T(1, 1, 1));

    b.Transform(m);

    EZ_TEST_BOOL(b.m_vCenter == ezVec3T(3, 4, -1));
    EZ_TEST_BOOL(b.m_vBoxHalfExtends == ezVec3T(10, 15, 10));
    EZ_TEST_BOOL(b.m_fSphereRadius == 15);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsNaN")
  {
    if (ezMath::BasicType<ezMathTestType>::SupportsNaN())
    {
      ezBoundingBoxSphereT b;

      b.SetInvalid();
      EZ_TEST_BOOL(!b.IsNaN());

      b.SetInvalid();
      b.m_vCenter.x = ezMath::BasicType<ezMathTestType>::GetNaN();
      EZ_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_vCenter.y = ezMath::BasicType<ezMathTestType>::GetNaN();
      EZ_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_vCenter.z = ezMath::BasicType<ezMathTestType>::GetNaN();
      EZ_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_vBoxHalfExtends.x = ezMath::BasicType<ezMathTestType>::GetNaN();
      EZ_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_vBoxHalfExtends.y = ezMath::BasicType<ezMathTestType>::GetNaN();
      EZ_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_vBoxHalfExtends.z = ezMath::BasicType<ezMathTestType>::GetNaN();
      EZ_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_fSphereRadius = ezMath::BasicType<ezMathTestType>::GetNaN();
      EZ_TEST_BOOL(b.IsNaN());
    }
  }
}