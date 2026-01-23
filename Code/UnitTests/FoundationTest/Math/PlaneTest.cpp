#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/BoundingSphere.h>
#include <Foundation/Math/Plane.h>
#include <Foundation/Math/Random.h>

template <typename Type>
void TestPlane()
{
  using ezPlaneType = ezPlaneTemplate<Type>;
  using ezVec3Type = ezVec3Template<Type>;
  using ezMat3Type = ezMat3Template<Type>;
  using ezMat4Type = ezMat4Template<Type>;
  using ezBoundingBoxType = ezBoundingBoxTemplate<Type>;
  using ezBoundingSphereType = ezBoundingSphereTemplate<Type>;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Default Constructor")
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    if (ezMath::SupportsNaN<Type>())
    {
      // In debug the default constructor initializes everything with NaN.
      ezPlaneType p;
      EZ_TEST_BOOL(ezMath::IsNaN(p.m_vNormal.x) && ezMath::IsNaN(p.m_vNormal.y) && ezMath::IsNaN(p.m_vNormal.z) && ezMath::IsNaN(p.m_fNegDistance));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    Type testBlock[4] = {(Type)1, (Type)2, (Type)3, (Type)4};
    ezPlaneType* p = ::new ((void*)&testBlock[0]) ezPlaneType;
    EZ_TEST_BOOL(p->m_vNormal.x == (Type)1 && p->m_vNormal.y == (Type)2 && p->m_vNormal.z == (Type)3 && p->m_fNegDistance == (Type)4);
#endif
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor(Normal, Point)")
  {
    ezPlaneType p = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(1, 0, 0), ezVec3Type(5, 3, 1));

    EZ_TEST_BOOL(p.m_vNormal == ezVec3Type(1, 0, 0));
    EZ_TEST_FLOAT(p.m_fNegDistance, (Type)-5.0, (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor(Point, Point, Point)")
  {
    ezPlaneType p = ezPlaneType::MakeFromPoints(ezVec3Type(-1, 5, 1), ezVec3Type(1, 5, 1), ezVec3Type(0, 5, -5));

    EZ_TEST_VEC3(p.m_vNormal, ezVec3Type(0, 1, 0), (Type)0.0001);
    EZ_TEST_FLOAT(p.m_fNegDistance, (Type)-5.0, (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor(Points)")
  {
    ezVec3Type v[3] = {ezVec3Type(-1, 5, 1), ezVec3Type(1, 5, 1), ezVec3Type(0, 5, -5)};

    ezPlaneType p;
    p.SetFromPoints(v).AssertSuccess();

    EZ_TEST_VEC3(p.m_vNormal, ezVec3Type(0, 1, 0), (Type)0.0001);
    EZ_TEST_FLOAT(p.m_fNegDistance, (Type)-5.0, (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor(Points, numpoints)")
  {
    ezVec3Type v[6] = {ezVec3Type(-1, 5, 1), ezVec3Type(-1, 5, 1), ezVec3Type(1, 5, 1), ezVec3Type(1, 5, 1), ezVec3Type(0, 5, -5), ezVec3Type(0, 5, -5)};

    ezPlaneType p;
    p.SetFromPoints(v, 6).AssertSuccess();

    EZ_TEST_VEC3(p.m_vNormal, ezVec3Type(0, 1, 0), (Type)0.0001);
    EZ_TEST_FLOAT(p.m_fNegDistance, (Type)-5.0, (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFromNormalAndPoint")
  {
    ezPlaneType p;
    p = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(1, 0, 0), ezVec3Type(5, 3, 1));

    EZ_TEST_BOOL(p.m_vNormal == ezVec3Type(1, 0, 0));
    EZ_TEST_FLOAT(p.m_fNegDistance, (Type)-5.0, (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFromPoints")
  {
    ezPlaneType p;
    p.SetFromPoints(ezVec3Type(-1, 5, 1), ezVec3Type(1, 5, 1), ezVec3Type(0, 5, -5)).IgnoreResult();

    EZ_TEST_VEC3(p.m_vNormal, ezVec3Type(0, 1, 0), (Type)0.0001);
    EZ_TEST_FLOAT(p.m_fNegDistance, (Type)-5.0, (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFromPoints")
  {
    ezVec3Type v[3] = {ezVec3Type(-1, 5, 1), ezVec3Type(1, 5, 1), ezVec3Type(0, 5, -5)};

    ezPlaneType p;
    p.SetFromPoints(v).IgnoreResult();

    EZ_TEST_VEC3(p.m_vNormal, ezVec3Type(0, 1, 0), (Type)0.0001);
    EZ_TEST_FLOAT(p.m_fNegDistance, (Type)-5.0, (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFromPoints")
  {
    ezVec3Type v[6] = {ezVec3Type(-1, 5, 1), ezVec3Type(-1, 5, 1), ezVec3Type(1, 5, 1), ezVec3Type(1, 5, 1), ezVec3Type(0, 5, -5), ezVec3Type(0, 5, -5)};

    ezPlaneType p;
    p.SetFromPoints(v, 6).IgnoreResult();

    EZ_TEST_VEC3(p.m_vNormal, ezVec3Type(0, 1, 0), (Type)0.0001);
    EZ_TEST_FLOAT(p.m_fNegDistance, (Type)-5.0, (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFromDirections")
  {
    ezPlaneType p;
    p.SetFromDirections(ezVec3Type(1, 0, 0), ezVec3Type(1, 0, -1), ezVec3Type(3, 5, 9)).IgnoreResult();

    EZ_TEST_VEC3(p.m_vNormal, ezVec3Type(0, 1, 0), (Type)0.0001);
    EZ_TEST_FLOAT(p.m_fNegDistance, (Type)-5.0, (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetInvalid")
  {
    ezPlaneType p;
    p.SetFromDirections(ezVec3Type(1, 0, 0), ezVec3Type(1, 0, -1), ezVec3Type(3, 5, 9)).IgnoreResult();

    p = ezPlaneType::MakeInvalid();

    EZ_TEST_VEC3(p.m_vNormal, ezVec3Type(0, 0, 0), (Type)0.0001);
    EZ_TEST_FLOAT(p.m_fNegDistance, (Type)0.0, (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetDistanceTo")
  {
    ezPlaneType p = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(1, 0, 0), ezVec3Type(5, 0, 0));

    EZ_TEST_FLOAT(p.GetDistanceTo(ezVec3Type(10, 3, 5)), (Type)5.0, (Type)0.0001);
    EZ_TEST_FLOAT(p.GetDistanceTo(ezVec3Type(0, 7, 123)), (Type)-5.0, (Type)0.0001);
    EZ_TEST_FLOAT(p.GetDistanceTo(ezVec3Type(5, 12, 23)), (Type)0.0, (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetMinimumDistanceTo")
  {
    ezVec3Type v1[3] = {ezVec3Type(15, 3, 5), ezVec3Type(6, 7, 123), ezVec3Type(10, 12, 23)};
    ezVec3Type v2[3] = {ezVec3Type(3, 3, 5), ezVec3Type(5, 7, 123), ezVec3Type(10, 12, 23)};

    ezPlaneType p = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(1, 0, 0), ezVec3Type(5, 0, 0));

    EZ_TEST_FLOAT(p.GetMinimumDistanceTo(v1, 3), (Type)1.0, (Type)0.0001);
    EZ_TEST_FLOAT(p.GetMinimumDistanceTo(v2, 3), (Type)-2.0, (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetMinMaxDistanceTo")
  {
    ezVec3Type v1[3] = {ezVec3Type(15, 3, 5), ezVec3Type(5, 7, 123), ezVec3Type(0, 12, 23)};
    ezVec3Type v2[3] = {ezVec3Type(8, 3, 5), ezVec3Type(6, 7, 123), ezVec3Type(10, 12, 23)};

    ezPlaneType p = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(1, 0, 0), ezVec3Type(5, 0, 0));

    Type fmin, fmax;

    p.GetMinMaxDistanceTo(fmin, fmax, v1, 3);
    EZ_TEST_FLOAT(fmin, (Type)-5.0, (Type)0.0001);
    EZ_TEST_FLOAT(fmax, (Type)10.0, (Type)0.0001);

    p.GetMinMaxDistanceTo(fmin, fmax, v2, 3);
    EZ_TEST_FLOAT(fmin, (Type)1, (Type)0.0001);
    EZ_TEST_FLOAT(fmax, (Type)5, (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetPointPosition")
  {
    ezPlaneType p = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(0, 1, 0), ezVec3Type(0, 10, 0));

    EZ_TEST_BOOL(p.GetPointPosition(ezVec3Type(0, 15, 0)) == ezPositionOnPlane::Front);
    EZ_TEST_BOOL(p.GetPointPosition(ezVec3Type(0, 5, 0)) == ezPositionOnPlane::Back);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetPointPosition(planewidth)")
  {
    ezPlaneType p = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(0, 1, 0), ezVec3Type(0, 10, 0));

    EZ_TEST_BOOL(p.GetPointPosition(ezVec3Type(0, 15, 0), (Type)0.01) == ezPositionOnPlane::Front);
    EZ_TEST_BOOL(p.GetPointPosition(ezVec3Type(0, 5, 0), (Type)0.01) == ezPositionOnPlane::Back);
    EZ_TEST_BOOL(p.GetPointPosition(ezVec3Type(0, 10, 0), (Type)0.01) == ezPositionOnPlane::OnPlane);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetObjectPosition")
  {
    ezPlaneType p = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(1, 0, 0), ezVec3Type(10, 0, 0));

    ezVec3Type v0[3] = {ezVec3Type(12, 0, 0), ezVec3Type(15, 0, 0), ezVec3Type(20, 0, 0)};
    ezVec3Type v1[3] = {ezVec3Type(8, 0, 0), ezVec3Type(6, 0, 0), ezVec3Type(4, 0, 0)};
    ezVec3Type v2[3] = {ezVec3Type(12, 0, 0), ezVec3Type(6, 0, 0), ezVec3Type(4, 0, 0)};

    EZ_TEST_BOOL(p.GetObjectPosition(v0, 3) == ezPositionOnPlane::Front);
    EZ_TEST_BOOL(p.GetObjectPosition(v1, 3) == ezPositionOnPlane::Back);
    EZ_TEST_BOOL(p.GetObjectPosition(v2, 3) == ezPositionOnPlane::Spanning);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetObjectPosition(fPlaneHalfWidth)")
  {
    ezPlaneType p = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(1, 0, 0), ezVec3Type(10, 0, 0));

    ezVec3Type v0[3] = {ezVec3Type(12, 0, 0), ezVec3Type(15, 0, 0), ezVec3Type(20, 0, 0)};
    ezVec3Type v1[3] = {ezVec3Type(8, 0, 0), ezVec3Type(6, 0, 0), ezVec3Type(4, 0, 0)};
    ezVec3Type v2[3] = {ezVec3Type(12, 0, 0), ezVec3Type(6, 0, 0), ezVec3Type(4, 0, 0)};
    ezVec3Type v3[3] = {ezVec3Type(10, 1, 0), ezVec3Type(10, 5, 7), ezVec3Type(10, 3, -5)};

    EZ_TEST_BOOL(p.GetObjectPosition(v0, 3, (Type)0.001) == ezPositionOnPlane::Front);
    EZ_TEST_BOOL(p.GetObjectPosition(v1, 3, (Type)0.001) == ezPositionOnPlane::Back);
    EZ_TEST_BOOL(p.GetObjectPosition(v2, 3, (Type)0.001) == ezPositionOnPlane::Spanning);
    EZ_TEST_BOOL(p.GetObjectPosition(v3, 3, (Type)0.001) == ezPositionOnPlane::OnPlane);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetObjectPosition(sphere)")
  {
    ezPlaneType p = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(1, 0, 0), ezVec3Type(10, 0, 0));

    EZ_TEST_BOOL(p.GetObjectPosition(ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(15, 2, 3), (Type)3.0)) == ezPositionOnPlane::Front);
    EZ_TEST_BOOL(p.GetObjectPosition(ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(5, 2, 3), (Type)3.0)) == ezPositionOnPlane::Back);
    EZ_TEST_BOOL(p.GetObjectPosition(ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type((Type)15, (Type)2, (Type)4.999), (Type)3.0)) == ezPositionOnPlane::Front);
    EZ_TEST_BOOL(p.GetObjectPosition(ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(5, 2, 3), (Type)4.999)) == ezPositionOnPlane::Back);
    EZ_TEST_BOOL(p.GetObjectPosition(ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(8, 2, 3), (Type)3.0)) == ezPositionOnPlane::Spanning);
    EZ_TEST_BOOL(p.GetObjectPosition(ezBoundingSphereType::MakeFromCenterAndRadius(ezVec3Type(12, 2, 3), (Type)3.0)) == ezPositionOnPlane::Spanning);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetObjectPosition(box)")
  {
    {
      ezPlaneType p = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(1, 0, 0), ezVec3Type(10, 0, 0));
      EZ_TEST_BOOL(p.GetObjectPosition(ezBoundingBoxType::MakeFromMinMax(ezVec3Type((Type)10.1), ezVec3Type(15))) == ezPositionOnPlane::Front);
      EZ_TEST_BOOL(p.GetObjectPosition(ezBoundingBoxType::MakeFromMinMax(ezVec3Type(7), ezVec3Type((Type)9.9))) == ezPositionOnPlane::Back);
      EZ_TEST_BOOL(p.GetObjectPosition(ezBoundingBoxType::MakeFromMinMax(ezVec3Type(7), ezVec3Type(15))) == ezPositionOnPlane::Spanning);
    }
    {
      ezPlaneType p = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(0, 1, 0), ezVec3Type(0, 10, 0));
      EZ_TEST_BOOL(p.GetObjectPosition(ezBoundingBoxType::MakeFromMinMax(ezVec3Type((Type)10.1), ezVec3Type(15))) == ezPositionOnPlane::Front);
      EZ_TEST_BOOL(p.GetObjectPosition(ezBoundingBoxType::MakeFromMinMax(ezVec3Type(7), ezVec3Type((Type)9.9))) == ezPositionOnPlane::Back);
      EZ_TEST_BOOL(p.GetObjectPosition(ezBoundingBoxType::MakeFromMinMax(ezVec3Type(7), ezVec3Type(15))) == ezPositionOnPlane::Spanning);
    }
    {
      ezPlaneType p = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(0, 0, 1), ezVec3Type(0, 0, 10));
      EZ_TEST_BOOL(p.GetObjectPosition(ezBoundingBoxType::MakeFromMinMax(ezVec3Type((Type)10.1), ezVec3Type(15))) == ezPositionOnPlane::Front);
      EZ_TEST_BOOL(p.GetObjectPosition(ezBoundingBoxType::MakeFromMinMax(ezVec3Type(7), ezVec3Type((Type)9.9))) == ezPositionOnPlane::Back);
      EZ_TEST_BOOL(p.GetObjectPosition(ezBoundingBoxType::MakeFromMinMax(ezVec3Type(7), ezVec3Type(15))) == ezPositionOnPlane::Spanning);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ProjectOntoPlane")
  {
    ezPlaneType p = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(0, 1, 0), ezVec3Type(0, 10, 0));

    EZ_TEST_VEC3(p.ProjectOntoPlane(ezVec3Type(3, 15, 2)), ezVec3Type(3, 10, 2), (Type)0.001);
    EZ_TEST_VEC3(p.ProjectOntoPlane(ezVec3Type(-1, 5, -5)), ezVec3Type(-1, 10, -5), (Type)0.001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Mirror")
  {
    ezPlaneType p = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(0, 1, 0), ezVec3Type(0, 10, 0));

    EZ_TEST_VEC3(p.Mirror(ezVec3Type(3, 15, 2)), ezVec3Type(3, 5, 2), (Type)0.001);
    EZ_TEST_VEC3(p.Mirror(ezVec3Type(-1, 5, -5)), ezVec3Type(-1, 15, -5), (Type)0.001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetCoplanarDirection")
  {
    ezPlaneType p = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(0, 1, 0), ezVec3Type(0, 10, 0));

    EZ_TEST_VEC3(p.GetCoplanarDirection(ezVec3Type(0, 1, 0)), ezVec3Type(0, 0, 0), (Type)0.001);
    EZ_TEST_VEC3(p.GetCoplanarDirection(ezVec3Type(1, 1, 0)).GetNormalized(), ezVec3Type(1, 0, 0), (Type)0.001);
    EZ_TEST_VEC3(p.GetCoplanarDirection(ezVec3Type(-1, 1, 0)).GetNormalized(), ezVec3Type(-1, 0, 0), (Type)0.001);
    EZ_TEST_VEC3(p.GetCoplanarDirection(ezVec3Type(0, 1, 1)).GetNormalized(), ezVec3Type(0, 0, 1), (Type)0.001);
    EZ_TEST_VEC3(p.GetCoplanarDirection(ezVec3Type(0, 1, -1)).GetNormalized(), ezVec3Type(0, 0, -1), (Type)0.001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsIdentical / operator== / operator!=")
  {
    ezPlaneType p1 = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(0, 1, 0), ezVec3Type(0, 10, 0));
    ezPlaneType p2 = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(0, 1, 0), ezVec3Type(0, 10, 0));
    ezPlaneType p3 = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(0, 1, 0), ezVec3Type(0, 10 + ezMath::DefaultEpsilon<Type>(), 0));

    EZ_TEST_BOOL(p1.IsIdentical(p1));
    EZ_TEST_BOOL(p2.IsIdentical(p2));
    EZ_TEST_BOOL(p3.IsIdentical(p3));

    EZ_TEST_BOOL(p1.IsIdentical(p2));
    EZ_TEST_BOOL(p2.IsIdentical(p1));

    EZ_TEST_BOOL(!p1.IsIdentical(p3));
    EZ_TEST_BOOL(!p2.IsIdentical(p3));


    EZ_TEST_BOOL(p1 == p2);
    EZ_TEST_BOOL(p2 == p1);

    EZ_TEST_BOOL(p1 != p3);
    EZ_TEST_BOOL(p2 != p3);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsEqual")
  {
    ezPlaneType p1 = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(0, 1, 0), ezVec3Type(0, 10, 0));
    ezPlaneType p2 = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(0, 1, 0), ezVec3Type(0, 10, 0));
    ezPlaneType p3 = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(0, 1, 0), ezVec3Type(0, 10 + ezMath::DefaultEpsilon<Type>(), 0));

    EZ_TEST_BOOL(p1.IsEqual(p1));
    EZ_TEST_BOOL(p2.IsEqual(p2));
    EZ_TEST_BOOL(p3.IsEqual(p3));

    EZ_TEST_BOOL(p1.IsEqual(p2));
    EZ_TEST_BOOL(p2.IsEqual(p1));

    EZ_TEST_BOOL(p1.IsEqual(p3));
    EZ_TEST_BOOL(p2.IsEqual(p3));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsValid")
  {
    ezPlaneType p1 = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(0, 1, 0), ezVec3Type(0, 10, 0));

    EZ_TEST_BOOL(p1.IsValid());

    p1 = ezPlaneType::MakeInvalid();
    EZ_TEST_BOOL(!p1.IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Transform(Mat3)")
  {
    const Type matrixScale[] = {(Type)1, (Type)2, (Type)99};

    for (ezUInt32 loopIndex = 0; loopIndex < EZ_ARRAY_SIZE(matrixScale); ++loopIndex)
    {
      ezPlaneType p = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(0, 1, 0), ezVec3Type(0, 10, 0));

      ezMat3Type m;
      {
        m = ezMat3Type::MakeRotationX(ezAngleTemplate<Type>::MakeFromDegree((Type)90));

        ezMat3Type rot = ezMat3Type::MakeScaling(ezVec3Type(1) * matrixScale[loopIndex]);
        m = m * rot;
      }

      p.Transform(m);

      EZ_TEST_VEC3(p.m_vNormal, ezVec3Type(0, 0, 1), (Type)0.0001);
      EZ_TEST_FLOAT(p.m_fNegDistance, (Type)-10.0 * matrixScale[loopIndex], (Type)0.0001);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Transform(Mat4)")
  {
    const Type matrixScale[] = {(Type)1, (Type)2, (Type)99};

    for (ezUInt32 loopIndex = 0; loopIndex < EZ_ARRAY_SIZE(matrixScale); ++loopIndex)
    {
      {
        ezPlaneType p = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(0, 1, 0), ezVec3Type(0, 10, 0));

        ezMat4Type m;
        {
          m = ezMat4Type::MakeRotationX(ezAngleTemplate<Type>::MakeFromDegree((Type)90));
          m.SetTranslationVector(ezVec3Type(0, 5, 0));

          ezMat4Type rot = ezMat4Type::MakeScaling(ezVec3Type(1) * matrixScale[loopIndex]);
          m = m * rot;
        }

        p.Transform(m);

        EZ_TEST_VEC3(p.m_vNormal, ezVec3Type(0, 0, 1), (Type)0.0001);
        EZ_TEST_FLOAT(p.m_fNegDistance, (Type)-10.0 * matrixScale[loopIndex], (Type)0.0001);
      }

      {
        ezPlaneType p = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(0, 1, 0), ezVec3Type(0, 10, 0));

        ezMat4Type m;
        {
          m = ezMat4Type::MakeRotationX(ezAngleTemplate<Type>::MakeFromDegree((Type)90));
          m.SetTranslationVector(ezVec3Type(0, 0, 5));

          ezMat4Type rot = ezMat4Type::MakeScaling(ezVec3Type(1) * matrixScale[loopIndex]);
          m = m * rot;
        }

        p.Transform(m);

        EZ_TEST_VEC3(p.m_vNormal, ezVec3Type(0, 0, 1), (Type)0.0001);
        EZ_TEST_FLOAT(p.m_fNegDistance, (Type)-10.0 * matrixScale[loopIndex] - (Type)5.0, (Type)0.0001);
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Flip")
  {
    ezPlaneType p = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(0, 1, 0), ezVec3Type(0, 10, 0));

    EZ_TEST_VEC3(p.m_vNormal, ezVec3Type(0, 1, 0), (Type)0.0001);
    EZ_TEST_FLOAT(p.m_fNegDistance, (Type)-10.0, (Type)0.0001);

    p.Flip();

    EZ_TEST_VEC3(p.m_vNormal, ezVec3Type(0, -1, 0), (Type)0.0001);
    EZ_TEST_FLOAT(p.m_fNegDistance, (Type)10.0, (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FlipIfNecessary")
  {
    {
      ezPlaneType p = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(0, 1, 0), ezVec3Type(0, 10, 0));

      EZ_TEST_VEC3(p.m_vNormal, ezVec3Type(0, 1, 0), (Type)0.0001);
      EZ_TEST_FLOAT(p.m_fNegDistance, (Type)-10.0, (Type)0.0001);

      EZ_TEST_BOOL(p.FlipIfNecessary(ezVec3Type(0, 11, 0), true) == false);

      EZ_TEST_VEC3(p.m_vNormal, ezVec3Type(0, 1, 0), (Type)0.0001);
      EZ_TEST_FLOAT(p.m_fNegDistance, (Type)-10.0, (Type)0.0001);
    }

    {
      ezPlaneType p = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(0, 1, 0), ezVec3Type(0, 10, 0));

      EZ_TEST_VEC3(p.m_vNormal, ezVec3Type(0, 1, 0), (Type)0.0001);
      EZ_TEST_FLOAT(p.m_fNegDistance, (Type)-10.0, (Type)0.0001);

      EZ_TEST_BOOL(p.FlipIfNecessary(ezVec3Type(0, 11, 0), false) == true);

      EZ_TEST_VEC3(p.m_vNormal, ezVec3Type(0, -1, 0), (Type)0.0001);
      EZ_TEST_FLOAT(p.m_fNegDistance, (Type)10.0, (Type)0.0001);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetRayIntersection")
  {
    ezPlaneType p = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(0, 1, 0), ezVec3Type(0, 10, 0));

    Type f;
    ezVec3Type v;

    EZ_TEST_BOOL(p.GetRayIntersection(ezVec3Type(3, 1, 7), ezVec3Type(0, 1, 0), &f, &v));
    EZ_TEST_FLOAT(f, (Type)9, (Type)0.0001);
    EZ_TEST_VEC3(v, ezVec3Type(3, 10, 7), (Type)0.0001);

    EZ_TEST_BOOL(p.GetRayIntersection(ezVec3Type(3, 20, 7), ezVec3Type(0, -1, 0), &f, &v));
    EZ_TEST_FLOAT(f, (Type)10, (Type)0.0001);
    EZ_TEST_VEC3(v, ezVec3Type(3, 10, 7), (Type)0.0001);

    EZ_TEST_BOOL(!p.GetRayIntersection(ezVec3Type(3, 1, 7), ezVec3Type(1, 0, 0), &f, &v));
    EZ_TEST_BOOL(!p.GetRayIntersection(ezVec3Type(3, 1, 7), ezVec3Type(0, -1, 0), &f, &v));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetRayIntersectionBiDirectional")
  {
    ezPlaneType p = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(0, 1, 0), ezVec3Type(0, 10, 0));

    Type f;
    ezVec3Type v;

    EZ_TEST_BOOL(p.GetRayIntersectionBiDirectional(ezVec3Type(3, 1, 7), ezVec3Type(0, 1, 0), &f, &v));
    EZ_TEST_FLOAT(f, (Type)9, (Type)0.0001);
    EZ_TEST_VEC3(v, ezVec3Type(3, 10, 7), (Type)0.0001);

    EZ_TEST_BOOL(!p.GetRayIntersectionBiDirectional(ezVec3Type(3, 1, 7), ezVec3Type(1, 0, 0), &f, &v));

    EZ_TEST_BOOL(p.GetRayIntersectionBiDirectional(ezVec3Type(3, 1, 7), ezVec3Type(0, -1, 0), &f, &v));
    EZ_TEST_FLOAT(f, (Type)-9, (Type)0.0001);
    EZ_TEST_VEC3(v, ezVec3Type(3, 10, 7), (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetLineSegmentIntersection")
  {
    ezPlaneType p = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(0, 1, 0), ezVec3Type(0, 10, 0));

    Type f;
    ezVec3Type v;

    EZ_TEST_BOOL(p.GetLineSegmentIntersection(ezVec3Type(3, 5, 7), ezVec3Type(3, 15, 7), &f, &v));
    EZ_TEST_FLOAT(f, (Type)0.5, (Type)0.0001);
    EZ_TEST_VEC3(v, ezVec3Type(3, 10, 7), (Type)0.0001);

    EZ_TEST_BOOL(!p.GetLineSegmentIntersection(ezVec3Type(3, 5, 7), ezVec3Type(13, 5, 7), &f, &v));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetPlanesIntersectionPoint")
  {
    ezPlaneType p1 = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(1, 0, 0), ezVec3Type(0, 10, 0));
    ezPlaneType p2 = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(0, 1, 0), ezVec3Type(0, 10, 0));
    ezPlaneType p3 = ezPlaneType::MakeFromNormalAndPoint(ezVec3Type(0, 0, 1), ezVec3Type(0, 10, 0));

    ezVec3Type r;

    EZ_TEST_BOOL(ezPlaneType::GetPlanesIntersectionPoint(p1, p2, p3, r) == EZ_SUCCESS);
    EZ_TEST_VEC3(r, ezVec3Type(0, 10, 0), (Type)0.0001);

    EZ_TEST_BOOL(ezPlaneType::GetPlanesIntersectionPoint(p1, p1, p3, r) == EZ_FAILURE);
    EZ_TEST_BOOL(ezPlaneType::GetPlanesIntersectionPoint(p1, p2, p2, r) == EZ_FAILURE);
    EZ_TEST_BOOL(ezPlaneType::GetPlanesIntersectionPoint(p3, p2, p3, r) == EZ_FAILURE);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FindSupportPoints")
  {
    ezVec3Type v[6] = {ezVec3Type(-1, 5, 1), ezVec3Type(-1, 5, 1), ezVec3Type(1, 5, 1), ezVec3Type(1, 5, 1), ezVec3Type(0, 5, -5), ezVec3Type(0, 5, -5)};

    ezInt32 i1, i2, i3;

    ezPlaneType::FindSupportPoints(v, 6, i1, i2, i3).IgnoreResult();

    EZ_TEST_INT(i1, 0);
    EZ_TEST_INT(i2, 2);
    EZ_TEST_INT(i3, 4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsNaN")
  {
    if (ezMath::SupportsNaN<Type>())
    {
      ezPlaneType p;

      p = ezPlaneType::MakeInvalid();
      EZ_TEST_BOOL(!p.IsNaN());

      p = ezPlaneType::MakeInvalid();
      p.m_fNegDistance = ezMath::NaN<Type>();
      EZ_TEST_BOOL(p.IsNaN());

      p = ezPlaneType::MakeInvalid();
      p.m_vNormal.x = ezMath::NaN<Type>();
      EZ_TEST_BOOL(p.IsNaN());

      p = ezPlaneType::MakeInvalid();
      p.m_vNormal.y = ezMath::NaN<Type>();
      EZ_TEST_BOOL(p.IsNaN());

      p = ezPlaneType::MakeInvalid();
      p.m_vNormal.z = ezMath::NaN<Type>();
      EZ_TEST_BOOL(p.IsNaN());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsFinite")
  {
    if (ezMath::SupportsInfinity<Type>())
    {
      ezPlaneType p;

      p.m_vNormal = ezVec3Type(1, 2, 3).GetNormalized();
      p.m_fNegDistance = (Type)42;
      EZ_TEST_BOOL(p.IsValid());
      EZ_TEST_BOOL(p.IsFinite());

      p = ezPlaneType::MakeInvalid();
      p.m_vNormal = ezVec3Type(1, 2, 3).GetNormalized();
      p.m_fNegDistance = ezMath::Infinity<Type>();
      EZ_TEST_BOOL(p.IsValid());
      EZ_TEST_BOOL(!p.IsFinite());

      p = ezPlaneType::MakeInvalid();
      p.m_vNormal.x = ezMath::NaN<Type>();
      p.m_fNegDistance = ezMath::Infinity<Type>();
      EZ_TEST_BOOL(!p.IsValid());
      EZ_TEST_BOOL(!p.IsFinite());

      p = ezPlaneType::MakeInvalid();
      p.m_vNormal = ezVec3Type(1, 2, 3);
      p.m_fNegDistance = (Type)42;
      EZ_TEST_BOOL(!p.IsValid());
      EZ_TEST_BOOL(p.IsFinite());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetMinimumDistanceTo/GetMaximumDistanceTo")
  {
    const ezUInt32 numTestLoops = 1000 * 1000;

    ezRandom randomGenerator;
    randomGenerator.Initialize(0x83482343);

    const auto randomNonZeroVec3T = [&randomGenerator]() -> ezVec3Type
    {
      const Type extent = (Type)1000.0;
      const ezVec3Type v(randomGenerator.FloatMinMax(-extent, extent), randomGenerator.FloatMinMax(-extent, extent), randomGenerator.FloatMinMax(-extent, extent));
      return v.GetLength() > (Type)0.001 ? v : ezVec3Type::MakeAxisX();
    };

    for (ezUInt32 loopIndex = 0; loopIndex < numTestLoops; ++loopIndex)
    {
      const ezPlaneType plane = ezPlaneType::MakeFromNormalAndPoint(randomNonZeroVec3T().GetNormalized(), randomNonZeroVec3T());

      ezVec3Type boxCorners[8];
      ezBoundingBoxType box;
      {
        const ezVec3Type boxPoint0 = randomNonZeroVec3T();
        const ezVec3Type boxPoint1 = randomNonZeroVec3T();
        const ezVec3Type boxMins(ezMath::Min(boxPoint0.x, boxPoint1.x), ezMath::Min(boxPoint0.y, boxPoint1.y), ezMath::Min(boxPoint0.z, boxPoint1.z));
        const ezVec3Type boxMaxs(ezMath::Max(boxPoint0.x, boxPoint1.x), ezMath::Max(boxPoint0.y, boxPoint1.y), ezMath::Max(boxPoint0.z, boxPoint1.z));
        box = ezBoundingBoxType::MakeFromMinMax(boxMins, boxMaxs);
        box.GetCorners(boxCorners);
      }

      Type distanceMin;
      Type distanceMax;
      {
        distanceMin = plane.GetMinimumDistanceTo(box);
        distanceMax = plane.GetMaximumDistanceTo(box);
      }

      Type referenceDistanceMin = ezMath::MaxValue<Type>();
      Type referenceDistanceMax = -ezMath::MaxValue<Type>();
      {
        for (ezUInt32 cornerIndex = 0; cornerIndex < EZ_ARRAY_SIZE(boxCorners); ++cornerIndex)
        {
          const Type cornerDist = plane.GetDistanceTo(boxCorners[cornerIndex]);
          referenceDistanceMin = ezMath::Min(referenceDistanceMin, cornerDist);
          referenceDistanceMax = ezMath::Max(referenceDistanceMax, cornerDist);
        }
      }

      // Break at first error to not spam the log with other potential error (the loop here is very long)
      {
        bool currIterSucceeded = true;
        currIterSucceeded = currIterSucceeded && EZ_TEST_FLOAT(distanceMin, referenceDistanceMin, (Type)0.0001);
        currIterSucceeded = currIterSucceeded && EZ_TEST_FLOAT(distanceMax, referenceDistanceMax, (Type)0.0001);
        if (!currIterSucceeded)
        {
          break;
        }
      }
    }
  }
}

EZ_CREATE_SIMPLE_TEST(Math, Planef)
{
  TestPlane<float>();
}
EZ_CREATE_SIMPLE_TEST(Math, Planed)
{
  TestPlane<double>();
}
