#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/BoundingBoxSphere.h>

template <typename Type>
void TestBoundingBoxSphere()
{
  using ezBoundingBoxSphereType = ezBoundingBoxSphereTemplate<Type>;
  using ezBoundingBoxType = ezBoundingBoxTemplate<Type>;
  using ezBoundingSphereType = ezBoundingSphereTemplate<Type>;
  using ezVec3Type = ezVec3Template<Type>;
  using ezMat4Type = ezMat4Template<Type>;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
    ezBoundingBoxSphereType b = ezBoundingBoxSphereType::MakeFromCenterExtents(ezVec3Type(-1, -2, -3), ezVec3Type(1, 2, 3), 2);

    EZ_TEST_BOOL(b.m_vCenter == ezVec3Type(-1, -2, -3));
    EZ_TEST_BOOL(b.m_vBoxHalfExtents == ezVec3Type(1, 2, 3));
    EZ_TEST_BOOL(b.m_fSphereRadius == 2);

    ezBoundingBoxType box = ezBoundingBoxType::MakeFromMinMax(ezVec3Type(1, 1, 1), ezVec3Type(3, 3, 3));
    ezBoundingSphereType sphere = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(2, 2, 2), 1);

    b = ezBoundingBoxSphereType::MakeFromBoxAndSphere(box, sphere);

    EZ_TEST_BOOL(b.m_vCenter == ezVec3Type(2, 2, 2));
    EZ_TEST_BOOL(b.m_vBoxHalfExtents == ezVec3Type(1, 1, 1));
    EZ_TEST_BOOL(b.m_fSphereRadius == 1);
    EZ_TEST_BOOL(b.GetBox() == box);
    EZ_TEST_BOOL(b.GetSphere() == sphere);

    b = ezBoundingBoxSphereType::MakeFromBox(box);

    EZ_TEST_BOOL(b.m_vCenter == ezVec3Type(2, 2, 2));
    EZ_TEST_BOOL(b.m_vBoxHalfExtents == ezVec3Type(1, 1, 1));
    EZ_TEST_FLOAT(b.m_fSphereRadius, ezMath::Sqrt(Type(3)), ezMath::DefaultEpsilon<Type>());
    EZ_TEST_BOOL(b.GetBox() == box);

    b = ezBoundingBoxSphereType::MakeFromSphere(sphere);

    EZ_TEST_BOOL(b.m_vCenter == ezVec3Type(2, 2, 2));
    EZ_TEST_BOOL(b.m_vBoxHalfExtents == ezVec3Type(1, 1, 1));
    EZ_TEST_BOOL(b.m_fSphereRadius == 1);
    EZ_TEST_BOOL(b.GetSphere() == sphere);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFromPoints")
  {
    ezVec3Type p[6] = {
      ezVec3Type(-4, 0, 0),
      ezVec3Type(5, 0, 0),
      ezVec3Type(0, -6, 0),
      ezVec3Type(0, 7, 0),
      ezVec3Type(0, 0, -8),
      ezVec3Type(0, 0, 9),
    };

    ezBoundingBoxSphereType b = ezBoundingBoxSphereType::MakeFromPoints(p, 6);

    EZ_TEST_BOOL(b.m_vCenter == ezVec3Type(0.5, 0.5, 0.5));
    EZ_TEST_BOOL(b.m_vBoxHalfExtents == ezVec3Type(4.5, 6.5, 8.5));
    EZ_TEST_FLOAT(b.m_fSphereRadius, ezVec3Type(0.5, 0.5, 8.5).GetLength(), ezMath::DefaultEpsilon<Type>());
    EZ_TEST_BOOL(b.m_fSphereRadius <= b.m_vBoxHalfExtents.GetLength());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetInvalid")
  {
    ezBoundingBoxSphereType b = ezBoundingBoxSphereType::MakeInvalid();

    EZ_TEST_BOOL(!b.IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExpandToInclude")
  {
    ezBoundingBoxSphereType b1 = ezBoundingBoxSphereType::MakeInvalid();
    ezBoundingBoxSphereType b2 = ezBoundingBoxSphereType::MakeFromBox(ezBoundingBoxType::MakeFromMinMax(ezVec3Type(2, 2, 2), ezVec3Type(4, 4, 4)));

    b1.ExpandToInclude(b2);
    EZ_TEST_BOOL(b1 == b2);

    ezBoundingSphereType sphere = ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(2, 2, 2), 2);
    b2 = ezBoundingBoxSphereType::MakeFromSphere(sphere);

    b1.ExpandToInclude(b2);
    EZ_TEST_BOOL(b1 != b2);

    EZ_TEST_BOOL(b1.m_vCenter == ezVec3Type(2, 2, 2));
    EZ_TEST_BOOL(b1.m_vBoxHalfExtents == ezVec3Type(2, 2, 2));
    EZ_TEST_FLOAT(b1.m_fSphereRadius, ezMath::Sqrt(Type(3)) * 2, ezMath::DefaultEpsilon<Type>());
    EZ_TEST_BOOL(b1.m_fSphereRadius <= b1.m_vBoxHalfExtents.GetLength());

    b1 = ezBoundingBoxSphereType::MakeInvalid();
    b2 = ezBoundingBoxSphereType::MakeFromBox(ezBoundingBoxType::MakeFromMinMax(ezVec3Type(0.25, 0.25, 0.25), ezVec3Type(0.5, 0.5, 0.5)));

    b1.ExpandToInclude(b2);
    EZ_TEST_BOOL(b1 == b2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Transform")
  {
    ezBoundingBoxSphereType b = ezBoundingBoxSphereType::MakeFromCenterExtents(ezVec3Type(1), ezVec3Type(5), 5);

    ezMat4Type m;
    m = ezMat4Type::MakeScaling(ezVec3Type(-2, -3, -2));
    m.SetTranslationVector(ezVec3Type(1, 1, 1));

    b.Transform(m);

    EZ_TEST_BOOL(b.m_vCenter == ezVec3Type(-1, -2, -1));
    EZ_TEST_BOOL(b.m_vBoxHalfExtents == ezVec3Type(10, 15, 10));
    EZ_TEST_BOOL(b.m_fSphereRadius == 15);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsNaN")
  {
    if (ezMath::SupportsNaN<Type>())
    {
      ezBoundingBoxSphereType b;

      b = ezBoundingBoxSphereType::MakeInvalid();
      EZ_TEST_BOOL(!b.IsNaN());

      b = ezBoundingBoxSphereType::MakeInvalid();
      b.m_vCenter.x = ezMath::NaN<Type>();
      EZ_TEST_BOOL(b.IsNaN());

      b = ezBoundingBoxSphereType::MakeInvalid();
      b.m_vCenter.y = ezMath::NaN<Type>();
      EZ_TEST_BOOL(b.IsNaN());

      b = ezBoundingBoxSphereType::MakeInvalid();
      b.m_vCenter.z = ezMath::NaN<Type>();
      EZ_TEST_BOOL(b.IsNaN());

      b = ezBoundingBoxSphereType::MakeInvalid();
      b.m_vBoxHalfExtents.x = ezMath::NaN<Type>();
      EZ_TEST_BOOL(b.IsNaN());

      b = ezBoundingBoxSphereType::MakeInvalid();
      b.m_vBoxHalfExtents.y = ezMath::NaN<Type>();
      EZ_TEST_BOOL(b.IsNaN());

      b = ezBoundingBoxSphereType::MakeInvalid();
      b.m_vBoxHalfExtents.z = ezMath::NaN<Type>();
      EZ_TEST_BOOL(b.IsNaN());

      b = ezBoundingBoxSphereType::MakeInvalid();
      b.m_fSphereRadius = ezMath::NaN<Type>();
      EZ_TEST_BOOL(b.IsNaN());
    }
  }
}

EZ_CREATE_SIMPLE_TEST(Math, BoundingBoxSpheref)
{
  TestBoundingBoxSphere<float>();
}

EZ_CREATE_SIMPLE_TEST(Math, BoundingBoxSphered)
{
  TestBoundingBoxSphere<double>();
}
