#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/BoundingBoxSphere.h>

EZ_CREATE_SIMPLE_TEST(Math, BoundingBoxSphere)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
    ezBoundingBoxSphereT b = ezBoundingBoxSphereT::MakeFromCenterExtents(ezVec3T(-1, -2, -3), ezVec3T(1, 2, 3), 2);

    EZ_TEST_BOOL(b.m_vCenter == ezVec3T(-1, -2, -3));
    EZ_TEST_BOOL(b.m_vBoxHalfExtents == ezVec3T(1, 2, 3));
    EZ_TEST_BOOL(b.m_fSphereRadius == 2);

    ezBoundingBoxT box = ezBoundingBoxT::MakeFromMinMax(ezVec3T(1, 1, 1), ezVec3T(3, 3, 3));
    ezBoundingSphereT sphere = ezBoundingSphereT::MakeFromCenterAndRadius(ezVec3T(2, 2, 2), 1);

    b = ezBoundingBoxSphereT::MakeFromBoxAndSphere(box, sphere);

    EZ_TEST_BOOL(b.m_vCenter == ezVec3T(2, 2, 2));
    EZ_TEST_BOOL(b.m_vBoxHalfExtents == ezVec3T(1, 1, 1));
    EZ_TEST_BOOL(b.m_fSphereRadius == 1);
    EZ_TEST_BOOL(b.GetBox() == box);
    EZ_TEST_BOOL(b.GetSphere() == sphere);

    b = ezBoundingBoxSphereT::MakeFromBox(box);

    EZ_TEST_BOOL(b.m_vCenter == ezVec3T(2, 2, 2));
    EZ_TEST_BOOL(b.m_vBoxHalfExtents == ezVec3T(1, 1, 1));
    EZ_TEST_FLOAT(b.m_fSphereRadius, ezMath::Sqrt(ezMathTestType(3)), 0.00001f);
    EZ_TEST_BOOL(b.GetBox() == box);

    b = ezBoundingBoxSphereT::MakeFromSphere(sphere);

    EZ_TEST_BOOL(b.m_vCenter == ezVec3T(2, 2, 2));
    EZ_TEST_BOOL(b.m_vBoxHalfExtents == ezVec3T(1, 1, 1));
    EZ_TEST_BOOL(b.m_fSphereRadius == 1);
    EZ_TEST_BOOL(b.GetSphere() == sphere);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFromPoints")
  {
    ezVec3T p[6] = {
      ezVec3T(-4, 0, 0),
      ezVec3T(5, 0, 0),
      ezVec3T(0, -6, 0),
      ezVec3T(0, 7, 0),
      ezVec3T(0, 0, -8),
      ezVec3T(0, 0, 9),
    };

    ezBoundingBoxSphereT b = ezBoundingBoxSphereT::MakeFromPoints(p, 6);

    EZ_TEST_BOOL(b.m_vCenter == ezVec3T(0.5, 0.5, 0.5));
    EZ_TEST_BOOL(b.m_vBoxHalfExtents == ezVec3T(4.5, 6.5, 8.5));
    EZ_TEST_FLOAT(b.m_fSphereRadius, ezVec3T(0.5, 0.5, 8.5).GetLength(), 0.00001f);
    EZ_TEST_BOOL(b.m_fSphereRadius <= b.m_vBoxHalfExtents.GetLength());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetInvalid")
  {
    ezBoundingBoxSphereT b = ezBoundingBoxSphereT::MakeInvalid();

    EZ_TEST_BOOL(!b.IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExpandToInclude")
  {
    ezBoundingBoxSphereT b1 = ezBoundingBoxSphereT::MakeInvalid();
    ezBoundingBoxSphereT b2 = ezBoundingBoxSphereT::MakeFromBox(ezBoundingBoxT::MakeFromMinMax(ezVec3T(2, 2, 2), ezVec3T(4, 4, 4)));

    b1.ExpandToInclude(b2);
    EZ_TEST_BOOL(b1 == b2);

    ezBoundingSphereT sphere = ezBoundingSphereT::MakeFromCenterAndRadius(ezVec3T(2, 2, 2), 2);
    b2 = ezBoundingBoxSphereT::MakeFromSphere(sphere);

    b1.ExpandToInclude(b2);
    EZ_TEST_BOOL(b1 != b2);

    EZ_TEST_BOOL(b1.m_vCenter == ezVec3T(2, 2, 2));
    EZ_TEST_BOOL(b1.m_vBoxHalfExtents == ezVec3T(2, 2, 2));
    EZ_TEST_FLOAT(b1.m_fSphereRadius, ezMath::Sqrt(ezMathTestType(3)) * 2, 0.00001f);
    EZ_TEST_BOOL(b1.m_fSphereRadius <= b1.m_vBoxHalfExtents.GetLength());

    b1 = ezBoundingBoxSphereT::MakeInvalid();
    b2 = ezBoundingBoxSphereT::MakeFromBox(ezBoundingBoxT::MakeFromMinMax(ezVec3T(0.25, 0.25, 0.25), ezVec3T(0.5, 0.5, 0.5)));

    b1.ExpandToInclude(b2);
    EZ_TEST_BOOL(b1 == b2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Transform")
  {
    ezBoundingBoxSphereT b = ezBoundingBoxSphereT::MakeFromCenterExtents(ezVec3T(1), ezVec3T(5), 5);

    ezMat4T m;
    m = ezMat4::MakeScaling(ezVec3T(-2, -3, -2));
    m.SetTranslationVector(ezVec3T(1, 1, 1));

    b.Transform(m);

    EZ_TEST_BOOL(b.m_vCenter == ezVec3T(-1, -2, -1));
    EZ_TEST_BOOL(b.m_vBoxHalfExtents == ezVec3T(10, 15, 10));
    EZ_TEST_BOOL(b.m_fSphereRadius == 15);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsNaN")
  {
    if (ezMath::SupportsNaN<ezMathTestType>())
    {
      ezBoundingBoxSphereT b;

      b = ezBoundingBoxSphereT::MakeInvalid();
      EZ_TEST_BOOL(!b.IsNaN());

      b = ezBoundingBoxSphereT::MakeInvalid();
      b.m_vCenter.x = ezMath::NaN<ezMathTestType>();
      EZ_TEST_BOOL(b.IsNaN());

      b = ezBoundingBoxSphereT::MakeInvalid();
      b.m_vCenter.y = ezMath::NaN<ezMathTestType>();
      EZ_TEST_BOOL(b.IsNaN());

      b = ezBoundingBoxSphereT::MakeInvalid();
      b.m_vCenter.z = ezMath::NaN<ezMathTestType>();
      EZ_TEST_BOOL(b.IsNaN());

      b = ezBoundingBoxSphereT::MakeInvalid();
      b.m_vBoxHalfExtents.x = ezMath::NaN<ezMathTestType>();
      EZ_TEST_BOOL(b.IsNaN());

      b = ezBoundingBoxSphereT::MakeInvalid();
      b.m_vBoxHalfExtents.y = ezMath::NaN<ezMathTestType>();
      EZ_TEST_BOOL(b.IsNaN());

      b = ezBoundingBoxSphereT::MakeInvalid();
      b.m_vBoxHalfExtents.z = ezMath::NaN<ezMathTestType>();
      EZ_TEST_BOOL(b.IsNaN());

      b = ezBoundingBoxSphereT::MakeInvalid();
      b.m_fSphereRadius = ezMath::NaN<ezMathTestType>();
      EZ_TEST_BOOL(b.IsNaN());
    }
  }
}
