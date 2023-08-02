#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/BoundingSphere.h>
#include <Foundation/Math/Plane.h>
#include <Foundation/Math/Random.h>

EZ_CREATE_SIMPLE_TEST(Math, Plane)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Default Constructor")
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    if (ezMath::SupportsNaN<ezMat3T::ComponentType>())
    {
      // In debug the default constructor initializes everything with NaN.
      ezPlaneT p;
      EZ_TEST_BOOL(ezMath::IsNaN(p.m_vNormal.x) && ezMath::IsNaN(p.m_vNormal.y) && ezMath::IsNaN(p.m_vNormal.z) && ezMath::IsNaN(p.m_fNegDistance));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    ezPlaneT::ComponentType testBlock[4] = {(ezPlaneT::ComponentType)1, (ezPlaneT::ComponentType)2, (ezPlaneT::ComponentType)3, (ezPlaneT::ComponentType)4};
    ezPlaneT* p = ::new ((void*)&testBlock[0]) ezPlaneT;
    EZ_TEST_BOOL(p->m_vNormal.x == (ezPlaneT::ComponentType)1 && p->m_vNormal.y == (ezPlaneT::ComponentType)2 && p->m_vNormal.z == (ezPlaneT::ComponentType)3 && p->m_fNegDistance == (ezPlaneT::ComponentType)4);
#endif
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor(Normal, Point)")
  {
    ezPlaneT p = ezPlane::MakeFromNormalAndPoint(ezVec3T(1, 0, 0), ezVec3T(5, 3, 1));

    EZ_TEST_BOOL(p.m_vNormal == ezVec3T(1, 0, 0));
    EZ_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor(Point, Point, Point)")
  {
    ezPlaneT p = ezPlaneT::MakeFromPoints(ezVec3T(-1, 5, 1), ezVec3T(1, 5, 1), ezVec3T(0, 5, -5));

    EZ_TEST_VEC3(p.m_vNormal, ezVec3T(0, 1, 0), 0.0001f);
    EZ_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor(Points)")
  {
    ezVec3T v[3] = {ezVec3T(-1, 5, 1), ezVec3T(1, 5, 1), ezVec3T(0, 5, -5)};

    ezPlaneT p;
    p.SetFromPoints(v).AssertSuccess();

    EZ_TEST_VEC3(p.m_vNormal, ezVec3T(0, 1, 0), 0.0001f);
    EZ_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor(Points, numpoints)")
  {
    ezVec3T v[6] = {ezVec3T(-1, 5, 1), ezVec3T(-1, 5, 1), ezVec3T(1, 5, 1), ezVec3T(1, 5, 1), ezVec3T(0, 5, -5), ezVec3T(0, 5, -5)};

    ezPlaneT p;
    p.SetFromPoints(v, 6).AssertSuccess();

    EZ_TEST_VEC3(p.m_vNormal, ezVec3T(0, 1, 0), 0.0001f);
    EZ_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFromNormalAndPoint")
  {
    ezPlaneT p;
    p = ezPlane::MakeFromNormalAndPoint(ezVec3T(1, 0, 0), ezVec3T(5, 3, 1));

    EZ_TEST_BOOL(p.m_vNormal == ezVec3T(1, 0, 0));
    EZ_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFromPoints")
  {
    ezPlaneT p;
    p.SetFromPoints(ezVec3T(-1, 5, 1), ezVec3T(1, 5, 1), ezVec3T(0, 5, -5)).IgnoreResult();

    EZ_TEST_VEC3(p.m_vNormal, ezVec3T(0, 1, 0), 0.0001f);
    EZ_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFromPoints")
  {
    ezVec3T v[3] = {ezVec3T(-1, 5, 1), ezVec3T(1, 5, 1), ezVec3T(0, 5, -5)};

    ezPlaneT p;
    p.SetFromPoints(v).IgnoreResult();

    EZ_TEST_VEC3(p.m_vNormal, ezVec3T(0, 1, 0), 0.0001f);
    EZ_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFromPoints")
  {
    ezVec3T v[6] = {ezVec3T(-1, 5, 1), ezVec3T(-1, 5, 1), ezVec3T(1, 5, 1), ezVec3T(1, 5, 1), ezVec3T(0, 5, -5), ezVec3T(0, 5, -5)};

    ezPlaneT p;
    p.SetFromPoints(v, 6).IgnoreResult();

    EZ_TEST_VEC3(p.m_vNormal, ezVec3T(0, 1, 0), 0.0001f);
    EZ_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFromDirections")
  {
    ezPlaneT p;
    p.SetFromDirections(ezVec3T(1, 0, 0), ezVec3T(1, 0, -1), ezVec3T(3, 5, 9)).IgnoreResult();

    EZ_TEST_VEC3(p.m_vNormal, ezVec3T(0, 1, 0), 0.0001f);
    EZ_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetInvalid")
  {
    ezPlaneT p;
    p.SetFromDirections(ezVec3T(1, 0, 0), ezVec3T(1, 0, -1), ezVec3T(3, 5, 9)).IgnoreResult();

    p = ezPlaneT::MakeInvalid();

    EZ_TEST_VEC3(p.m_vNormal, ezVec3T(0, 0, 0), 0.0001f);
    EZ_TEST_FLOAT(p.m_fNegDistance, 0.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetDistanceTo")
  {
    ezPlaneT p = ezPlane::MakeFromNormalAndPoint(ezVec3T(1, 0, 0), ezVec3T(5, 0, 0));

    EZ_TEST_FLOAT(p.GetDistanceTo(ezVec3T(10, 3, 5)), 5.0f, 0.0001f);
    EZ_TEST_FLOAT(p.GetDistanceTo(ezVec3T(0, 7, 123)), -5.0f, 0.0001f);
    EZ_TEST_FLOAT(p.GetDistanceTo(ezVec3T(5, 12, 23)), 0.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetMinimumDistanceTo")
  {
    ezVec3T v1[3] = {ezVec3T(15, 3, 5), ezVec3T(6, 7, 123), ezVec3T(10, 12, 23)};
    ezVec3T v2[3] = {ezVec3T(3, 3, 5), ezVec3T(5, 7, 123), ezVec3T(10, 12, 23)};

    ezPlaneT p = ezPlane::MakeFromNormalAndPoint(ezVec3T(1, 0, 0), ezVec3T(5, 0, 0));

    EZ_TEST_FLOAT(p.GetMinimumDistanceTo(v1, 3), 1.0f, 0.0001f);
    EZ_TEST_FLOAT(p.GetMinimumDistanceTo(v2, 3), -2.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetMinMaxDistanceTo")
  {
    ezVec3T v1[3] = {ezVec3T(15, 3, 5), ezVec3T(5, 7, 123), ezVec3T(0, 12, 23)};
    ezVec3T v2[3] = {ezVec3T(8, 3, 5), ezVec3T(6, 7, 123), ezVec3T(10, 12, 23)};

    ezPlaneT p = ezPlane::MakeFromNormalAndPoint(ezVec3T(1, 0, 0), ezVec3T(5, 0, 0));

    ezMathTestType fmin, fmax;

    p.GetMinMaxDistanceTo(fmin, fmax, v1, 3);
    EZ_TEST_FLOAT(fmin, -5.0f, 0.0001f);
    EZ_TEST_FLOAT(fmax, 10.0f, 0.0001f);

    p.GetMinMaxDistanceTo(fmin, fmax, v2, 3);
    EZ_TEST_FLOAT(fmin, 1, 0.0001f);
    EZ_TEST_FLOAT(fmax, 5, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetPointPosition")
  {
    ezPlaneT p = ezPlane::MakeFromNormalAndPoint(ezVec3T(0, 1, 0), ezVec3T(0, 10, 0));

    EZ_TEST_BOOL(p.GetPointPosition(ezVec3T(0, 15, 0)) == ezPositionOnPlane::Front);
    EZ_TEST_BOOL(p.GetPointPosition(ezVec3T(0, 5, 0)) == ezPositionOnPlane::Back);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetPointPosition(planewidth)")
  {
    ezPlaneT p = ezPlane::MakeFromNormalAndPoint(ezVec3T(0, 1, 0), ezVec3T(0, 10, 0));

    EZ_TEST_BOOL(p.GetPointPosition(ezVec3T(0, 15, 0), 0.01f) == ezPositionOnPlane::Front);
    EZ_TEST_BOOL(p.GetPointPosition(ezVec3T(0, 5, 0), 0.01f) == ezPositionOnPlane::Back);
    EZ_TEST_BOOL(p.GetPointPosition(ezVec3T(0, 10, 0), 0.01f) == ezPositionOnPlane::OnPlane);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetObjectPosition")
  {
    ezPlaneT p = ezPlane::MakeFromNormalAndPoint(ezVec3T(1, 0, 0), ezVec3T(10, 0, 0));

    ezVec3T v0[3] = {ezVec3T(12, 0, 0), ezVec3T(15, 0, 0), ezVec3T(20, 0, 0)};
    ezVec3T v1[3] = {ezVec3T(8, 0, 0), ezVec3T(6, 0, 0), ezVec3T(4, 0, 0)};
    ezVec3T v2[3] = {ezVec3T(12, 0, 0), ezVec3T(6, 0, 0), ezVec3T(4, 0, 0)};

    EZ_TEST_BOOL(p.GetObjectPosition(v0, 3) == ezPositionOnPlane::Front);
    EZ_TEST_BOOL(p.GetObjectPosition(v1, 3) == ezPositionOnPlane::Back);
    EZ_TEST_BOOL(p.GetObjectPosition(v2, 3) == ezPositionOnPlane::Spanning);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetObjectPosition(fPlaneHalfWidth)")
  {
    ezPlaneT p = ezPlane::MakeFromNormalAndPoint(ezVec3T(1, 0, 0), ezVec3T(10, 0, 0));

    ezVec3T v0[3] = {ezVec3T(12, 0, 0), ezVec3T(15, 0, 0), ezVec3T(20, 0, 0)};
    ezVec3T v1[3] = {ezVec3T(8, 0, 0), ezVec3T(6, 0, 0), ezVec3T(4, 0, 0)};
    ezVec3T v2[3] = {ezVec3T(12, 0, 0), ezVec3T(6, 0, 0), ezVec3T(4, 0, 0)};
    ezVec3T v3[3] = {ezVec3T(10, 1, 0), ezVec3T(10, 5, 7), ezVec3T(10, 3, -5)};

    EZ_TEST_BOOL(p.GetObjectPosition(v0, 3, 0.001f) == ezPositionOnPlane::Front);
    EZ_TEST_BOOL(p.GetObjectPosition(v1, 3, 0.001f) == ezPositionOnPlane::Back);
    EZ_TEST_BOOL(p.GetObjectPosition(v2, 3, 0.001f) == ezPositionOnPlane::Spanning);
    EZ_TEST_BOOL(p.GetObjectPosition(v3, 3, 0.001f) == ezPositionOnPlane::OnPlane);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetObjectPosition(sphere)")
  {
    ezPlaneT p = ezPlane::MakeFromNormalAndPoint(ezVec3T(1, 0, 0), ezVec3T(10, 0, 0));

    EZ_TEST_BOOL(p.GetObjectPosition(ezBoundingSphereT::MakeFromCenterAndRadius(ezVec3T(15, 2, 3), 3.0f)) == ezPositionOnPlane::Front);
    EZ_TEST_BOOL(p.GetObjectPosition(ezBoundingSphereT::MakeFromCenterAndRadius(ezVec3T(5, 2, 3), 3.0f)) == ezPositionOnPlane::Back);
    EZ_TEST_BOOL(p.GetObjectPosition(ezBoundingSphereT::MakeFromCenterAndRadius(ezVec3T(15, 2, 4.999f), 3.0f)) == ezPositionOnPlane::Front);
    EZ_TEST_BOOL(p.GetObjectPosition(ezBoundingSphereT::MakeFromCenterAndRadius(ezVec3T(5, 2, 3), 4.999f)) == ezPositionOnPlane::Back);
    EZ_TEST_BOOL(p.GetObjectPosition(ezBoundingSphereT::MakeFromCenterAndRadius(ezVec3T(8, 2, 3), 3.0f)) == ezPositionOnPlane::Spanning);
    EZ_TEST_BOOL(p.GetObjectPosition(ezBoundingSphereT::MakeFromCenterAndRadius(ezVec3T(12, 2, 3), 3.0f)) == ezPositionOnPlane::Spanning);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetObjectPosition(box)")
  {
    {
      ezPlaneT p = ezPlane::MakeFromNormalAndPoint(ezVec3T(1, 0, 0), ezVec3T(10, 0, 0));
      EZ_TEST_BOOL(p.GetObjectPosition(ezBoundingBoxT::MakeFromMinMax(ezVec3T(10.1f), ezVec3T(15))) == ezPositionOnPlane::Front);
      EZ_TEST_BOOL(p.GetObjectPosition(ezBoundingBoxT::MakeFromMinMax(ezVec3T(7), ezVec3T(9.9f))) == ezPositionOnPlane::Back);
      EZ_TEST_BOOL(p.GetObjectPosition(ezBoundingBoxT::MakeFromMinMax(ezVec3T(7), ezVec3T(15))) == ezPositionOnPlane::Spanning);
    }
    {
      ezPlaneT p = ezPlane::MakeFromNormalAndPoint(ezVec3T(0, 1, 0), ezVec3T(0, 10, 0));
      EZ_TEST_BOOL(p.GetObjectPosition(ezBoundingBoxT::MakeFromMinMax(ezVec3T(10.1f), ezVec3T(15))) == ezPositionOnPlane::Front);
      EZ_TEST_BOOL(p.GetObjectPosition(ezBoundingBoxT::MakeFromMinMax(ezVec3T(7), ezVec3T(9.9f))) == ezPositionOnPlane::Back);
      EZ_TEST_BOOL(p.GetObjectPosition(ezBoundingBoxT::MakeFromMinMax(ezVec3T(7), ezVec3T(15))) == ezPositionOnPlane::Spanning);
    }
    {
      ezPlaneT p = ezPlane::MakeFromNormalAndPoint(ezVec3T(0, 0, 1), ezVec3T(0, 0, 10));
      EZ_TEST_BOOL(p.GetObjectPosition(ezBoundingBoxT::MakeFromMinMax(ezVec3T(10.1f), ezVec3T(15))) == ezPositionOnPlane::Front);
      EZ_TEST_BOOL(p.GetObjectPosition(ezBoundingBoxT::MakeFromMinMax(ezVec3T(7), ezVec3T(9.9f))) == ezPositionOnPlane::Back);
      EZ_TEST_BOOL(p.GetObjectPosition(ezBoundingBoxT::MakeFromMinMax(ezVec3T(7), ezVec3T(15))) == ezPositionOnPlane::Spanning);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ProjectOntoPlane")
  {
    ezPlaneT p = ezPlane::MakeFromNormalAndPoint(ezVec3T(0, 1, 0), ezVec3T(0, 10, 0));

    EZ_TEST_VEC3(p.ProjectOntoPlane(ezVec3T(3, 15, 2)), ezVec3T(3, 10, 2), 0.001f);
    EZ_TEST_VEC3(p.ProjectOntoPlane(ezVec3T(-1, 5, -5)), ezVec3T(-1, 10, -5), 0.001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Mirror")
  {
    ezPlaneT p = ezPlane::MakeFromNormalAndPoint(ezVec3T(0, 1, 0), ezVec3T(0, 10, 0));

    EZ_TEST_VEC3(p.Mirror(ezVec3T(3, 15, 2)), ezVec3T(3, 5, 2), 0.001f);
    EZ_TEST_VEC3(p.Mirror(ezVec3T(-1, 5, -5)), ezVec3T(-1, 15, -5), 0.001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetCoplanarDirection")
  {
    ezPlaneT p = ezPlane::MakeFromNormalAndPoint(ezVec3T(0, 1, 0), ezVec3T(0, 10, 0));

    EZ_TEST_VEC3(p.GetCoplanarDirection(ezVec3T(0, 1, 0)), ezVec3T(0, 0, 0), 0.001f);
    EZ_TEST_VEC3(p.GetCoplanarDirection(ezVec3T(1, 1, 0)).GetNormalized(), ezVec3T(1, 0, 0), 0.001f);
    EZ_TEST_VEC3(p.GetCoplanarDirection(ezVec3T(-1, 1, 0)).GetNormalized(), ezVec3T(-1, 0, 0), 0.001f);
    EZ_TEST_VEC3(p.GetCoplanarDirection(ezVec3T(0, 1, 1)).GetNormalized(), ezVec3T(0, 0, 1), 0.001f);
    EZ_TEST_VEC3(p.GetCoplanarDirection(ezVec3T(0, 1, -1)).GetNormalized(), ezVec3T(0, 0, -1), 0.001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsIdentical / operator== / operator!=")
  {
    ezPlaneT p1 = ezPlane::MakeFromNormalAndPoint(ezVec3T(0, 1, 0), ezVec3T(0, 10, 0));
    ezPlaneT p2 = ezPlane::MakeFromNormalAndPoint(ezVec3T(0, 1, 0), ezVec3T(0, 10, 0));
    ezPlaneT p3 = ezPlane::MakeFromNormalAndPoint(ezVec3T(0, 1, 0), ezVec3T(0, 10.00001f, 0));

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
    ezPlaneT p1 = ezPlane::MakeFromNormalAndPoint(ezVec3T(0, 1, 0), ezVec3T(0, 10, 0));
    ezPlaneT p2 = ezPlane::MakeFromNormalAndPoint(ezVec3T(0, 1, 0), ezVec3T(0, 10, 0));
    ezPlaneT p3 = ezPlane::MakeFromNormalAndPoint(ezVec3T(0, 1, 0), ezVec3T(0, 10.00001f, 0));

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
    ezPlaneT p1 = ezPlane::MakeFromNormalAndPoint(ezVec3T(0, 1, 0), ezVec3T(0, 10, 0));

    EZ_TEST_BOOL(p1.IsValid());

    p1 = ezPlaneT::MakeInvalid();
    EZ_TEST_BOOL(!p1.IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Transform(Mat3)")
  {
    ezPlaneT p = ezPlane::MakeFromNormalAndPoint(ezVec3T(0, 1, 0), ezVec3T(0, 10, 0));

    ezMat3T m;
    m = ezMat3::MakeRotationX(ezAngle::MakeFromDegree(90));

    p.Transform(m);

    EZ_TEST_VEC3(p.m_vNormal, ezVec3T(0, 0, 1), 0.0001f);
    EZ_TEST_FLOAT(p.m_fNegDistance, -10.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Transform(Mat4)")
  {
    {
      ezPlaneT p = ezPlane::MakeFromNormalAndPoint(ezVec3T(0, 1, 0), ezVec3T(0, 10, 0));

      ezMat4T m;
      m = ezMat4::MakeRotationX(ezAngle::MakeFromDegree(90));
      m.SetTranslationVector(ezVec3T(0, 5, 0));

      p.Transform(m);

      EZ_TEST_VEC3(p.m_vNormal, ezVec3T(0, 0, 1), 0.0001f);
      EZ_TEST_FLOAT(p.m_fNegDistance, -10.0f, 0.0001f);
    }

    {
      ezPlaneT p = ezPlane::MakeFromNormalAndPoint(ezVec3T(0, 1, 0), ezVec3T(0, 10, 0));

      ezMat4T m;
      m = ezMat4::MakeRotationX(ezAngle::MakeFromDegree(90));
      m.SetTranslationVector(ezVec3T(0, 0, 5));

      p.Transform(m);

      EZ_TEST_VEC3(p.m_vNormal, ezVec3T(0, 0, 1), 0.0001f);
      EZ_TEST_FLOAT(p.m_fNegDistance, -15.0f, 0.0001f);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Flip")
  {
    ezPlaneT p = ezPlane::MakeFromNormalAndPoint(ezVec3T(0, 1, 0), ezVec3T(0, 10, 0));

    EZ_TEST_VEC3(p.m_vNormal, ezVec3T(0, 1, 0), 0.0001f);
    EZ_TEST_FLOAT(p.m_fNegDistance, -10.0f, 0.0001f);

    p.Flip();

    EZ_TEST_VEC3(p.m_vNormal, ezVec3T(0, -1, 0), 0.0001f);
    EZ_TEST_FLOAT(p.m_fNegDistance, 10.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FlipIfNecessary")
  {
    {
      ezPlaneT p = ezPlane::MakeFromNormalAndPoint(ezVec3T(0, 1, 0), ezVec3T(0, 10, 0));

      EZ_TEST_VEC3(p.m_vNormal, ezVec3T(0, 1, 0), 0.0001f);
      EZ_TEST_FLOAT(p.m_fNegDistance, -10.0f, 0.0001f);

      EZ_TEST_BOOL(p.FlipIfNecessary(ezVec3T(0, 11, 0), true) == false);

      EZ_TEST_VEC3(p.m_vNormal, ezVec3T(0, 1, 0), 0.0001f);
      EZ_TEST_FLOAT(p.m_fNegDistance, -10.0f, 0.0001f);
    }

    {
      ezPlaneT p = ezPlane::MakeFromNormalAndPoint(ezVec3T(0, 1, 0), ezVec3T(0, 10, 0));

      EZ_TEST_VEC3(p.m_vNormal, ezVec3T(0, 1, 0), 0.0001f);
      EZ_TEST_FLOAT(p.m_fNegDistance, -10.0f, 0.0001f);

      EZ_TEST_BOOL(p.FlipIfNecessary(ezVec3T(0, 11, 0), false) == true);

      EZ_TEST_VEC3(p.m_vNormal, ezVec3T(0, -1, 0), 0.0001f);
      EZ_TEST_FLOAT(p.m_fNegDistance, 10.0f, 0.0001f);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetRayIntersection")
  {
    ezPlaneT p = ezPlane::MakeFromNormalAndPoint(ezVec3T(0, 1, 0), ezVec3T(0, 10, 0));

    ezMathTestType f;
    ezVec3T v;

    EZ_TEST_BOOL(p.GetRayIntersection(ezVec3T(3, 1, 7), ezVec3T(0, 1, 0), &f, &v));
    EZ_TEST_FLOAT(f, 9, 0.0001f);
    EZ_TEST_VEC3(v, ezVec3T(3, 10, 7), 0.0001f);

    EZ_TEST_BOOL(p.GetRayIntersection(ezVec3T(3, 20, 7), ezVec3T(0, -1, 0), &f, &v));
    EZ_TEST_FLOAT(f, 10, 0.0001f);
    EZ_TEST_VEC3(v, ezVec3T(3, 10, 7), 0.0001f);

    EZ_TEST_BOOL(!p.GetRayIntersection(ezVec3T(3, 1, 7), ezVec3T(1, 0, 0), &f, &v));
    EZ_TEST_BOOL(!p.GetRayIntersection(ezVec3T(3, 1, 7), ezVec3T(0, -1, 0), &f, &v));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetRayIntersectionBiDirectional")
  {
    ezPlaneT p = ezPlane::MakeFromNormalAndPoint(ezVec3T(0, 1, 0), ezVec3T(0, 10, 0));

    ezMathTestType f;
    ezVec3T v;

    EZ_TEST_BOOL(p.GetRayIntersectionBiDirectional(ezVec3T(3, 1, 7), ezVec3T(0, 1, 0), &f, &v));
    EZ_TEST_FLOAT(f, 9, 0.0001f);
    EZ_TEST_VEC3(v, ezVec3T(3, 10, 7), 0.0001f);

    EZ_TEST_BOOL(!p.GetRayIntersectionBiDirectional(ezVec3T(3, 1, 7), ezVec3T(1, 0, 0), &f, &v));

    EZ_TEST_BOOL(p.GetRayIntersectionBiDirectional(ezVec3T(3, 1, 7), ezVec3T(0, -1, 0), &f, &v));
    EZ_TEST_FLOAT(f, -9, 0.0001f);
    EZ_TEST_VEC3(v, ezVec3T(3, 10, 7), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetLineSegmentIntersection")
  {
    ezPlaneT p = ezPlane::MakeFromNormalAndPoint(ezVec3T(0, 1, 0), ezVec3T(0, 10, 0));

    ezMathTestType f;
    ezVec3T v;

    EZ_TEST_BOOL(p.GetLineSegmentIntersection(ezVec3T(3, 5, 7), ezVec3T(3, 15, 7), &f, &v));
    EZ_TEST_FLOAT(f, 0.5f, 0.0001f);
    EZ_TEST_VEC3(v, ezVec3T(3, 10, 7), 0.0001f);

    EZ_TEST_BOOL(!p.GetLineSegmentIntersection(ezVec3T(3, 5, 7), ezVec3T(13, 5, 7), &f, &v));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetPlanesIntersectionPoint")
  {
    ezPlaneT p1 = ezPlane::MakeFromNormalAndPoint(ezVec3T(1, 0, 0), ezVec3T(0, 10, 0));
    ezPlaneT p2 = ezPlane::MakeFromNormalAndPoint(ezVec3T(0, 1, 0), ezVec3T(0, 10, 0));
    ezPlaneT p3 = ezPlane::MakeFromNormalAndPoint(ezVec3T(0, 0, 1), ezVec3T(0, 10, 0));

    ezVec3T r;

    EZ_TEST_BOOL(ezPlaneT::GetPlanesIntersectionPoint(p1, p2, p3, r) == EZ_SUCCESS);
    EZ_TEST_VEC3(r, ezVec3T(0, 10, 0), 0.0001f);

    EZ_TEST_BOOL(ezPlaneT::GetPlanesIntersectionPoint(p1, p1, p3, r) == EZ_FAILURE);
    EZ_TEST_BOOL(ezPlaneT::GetPlanesIntersectionPoint(p1, p2, p2, r) == EZ_FAILURE);
    EZ_TEST_BOOL(ezPlaneT::GetPlanesIntersectionPoint(p3, p2, p3, r) == EZ_FAILURE);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FindSupportPoints")
  {
    ezVec3T v[6] = {ezVec3T(-1, 5, 1), ezVec3T(-1, 5, 1), ezVec3T(1, 5, 1), ezVec3T(1, 5, 1), ezVec3T(0, 5, -5), ezVec3T(0, 5, -5)};

    ezInt32 i1, i2, i3;

    ezPlaneT::FindSupportPoints(v, 6, i1, i2, i3).IgnoreResult();

    EZ_TEST_INT(i1, 0);
    EZ_TEST_INT(i2, 2);
    EZ_TEST_INT(i3, 4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsNaN")
  {
    if (ezMath::SupportsNaN<ezMathTestType>())
    {
      ezPlaneT p;

      p = ezPlaneT::MakeInvalid();
      EZ_TEST_BOOL(!p.IsNaN());

      p = ezPlaneT::MakeInvalid();
      p.m_fNegDistance = ezMath::NaN<ezMathTestType>();
      EZ_TEST_BOOL(p.IsNaN());

      p = ezPlaneT::MakeInvalid();
      p.m_vNormal.x = ezMath::NaN<ezMathTestType>();
      EZ_TEST_BOOL(p.IsNaN());

      p = ezPlaneT::MakeInvalid();
      p.m_vNormal.y = ezMath::NaN<ezMathTestType>();
      EZ_TEST_BOOL(p.IsNaN());

      p = ezPlaneT::MakeInvalid();
      p.m_vNormal.z = ezMath::NaN<ezMathTestType>();
      EZ_TEST_BOOL(p.IsNaN());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsFinite")
  {
    if (ezMath::SupportsInfinity<ezMathTestType>())
    {
      ezPlaneT p;

      p.m_vNormal = ezVec3(1, 2, 3).GetNormalized();
      p.m_fNegDistance = 42;
      EZ_TEST_BOOL(p.IsValid());
      EZ_TEST_BOOL(p.IsFinite());

      p = ezPlaneT::MakeInvalid();
      p.m_vNormal = ezVec3(1, 2, 3).GetNormalized();
      p.m_fNegDistance = ezMath::Infinity<ezMathTestType>();
      EZ_TEST_BOOL(p.IsValid());
      EZ_TEST_BOOL(!p.IsFinite());

      p = ezPlaneT::MakeInvalid();
      p.m_vNormal.x = ezMath::NaN<ezMathTestType>();
      p.m_fNegDistance = ezMath::Infinity<ezMathTestType>();
      EZ_TEST_BOOL(!p.IsValid());
      EZ_TEST_BOOL(!p.IsFinite());

      p = ezPlaneT::MakeInvalid();
      p.m_vNormal = ezVec3(1, 2, 3);
      p.m_fNegDistance = 42;
      EZ_TEST_BOOL(!p.IsValid());
      EZ_TEST_BOOL(p.IsFinite());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetMinimumDistanceTo/GetMaximumDistanceTo")
  {
    const ezUInt32 numTestLoops = 1000 * 1000;

    ezRandom randomGenerator;
    randomGenerator.Initialize(0x83482343);

    const auto randomNonZeroVec3T = [&randomGenerator]() -> ezVec3T
    {
      const float extent = 1000.f;
      const ezVec3T v(randomGenerator.FloatMinMax(-extent, extent), randomGenerator.FloatMinMax(-extent, extent), randomGenerator.FloatMinMax(-extent, extent));
      return v.GetLength() > 0.001f ? v : ezVec3T::MakeAxisX();
    };

    for (ezUInt32 loopIndex = 0; loopIndex < numTestLoops; ++loopIndex)
    {
      const ezPlaneT plane = ezPlane::MakeFromNormalAndPoint(randomNonZeroVec3T().GetNormalized(), randomNonZeroVec3T());

      ezVec3T boxCorners[8];
      ezBoundingBoxT box;
      {
        const ezVec3T boxPoint0 = randomNonZeroVec3T();
        const ezVec3T boxPoint1 = randomNonZeroVec3T();
        const ezVec3T boxMins(ezMath::Min(boxPoint0.x, boxPoint1.x), ezMath::Min(boxPoint0.y, boxPoint1.y), ezMath::Min(boxPoint0.z, boxPoint1.z));
        const ezVec3T boxMaxs(ezMath::Max(boxPoint0.x, boxPoint1.x), ezMath::Max(boxPoint0.y, boxPoint1.y), ezMath::Max(boxPoint0.z, boxPoint1.z));
        box = ezBoundingBoxT::MakeFromMinMax(boxMins, boxMaxs);
        box.GetCorners(boxCorners);
      }

      float distanceMin;
      float distanceMax;
      {
        distanceMin = plane.GetMinimumDistanceTo(box);
        distanceMax = plane.GetMaximumDistanceTo(box);
      }

      float referenceDistanceMin = FLT_MAX;
      float referenceDistanceMax = -FLT_MAX;
      {
        for (ezUInt32 cornerIndex = 0; cornerIndex < EZ_ARRAY_SIZE(boxCorners); ++cornerIndex)
        {
          const float cornerDist = plane.GetDistanceTo(boxCorners[cornerIndex]);
          referenceDistanceMin = ezMath::Min(referenceDistanceMin, cornerDist);
          referenceDistanceMax = ezMath::Max(referenceDistanceMax, cornerDist);
        }
      }

      // Break at first error to not spam the log with other potential error (the loop here is very long)
      {
        bool currIterSucceeded = true;
        currIterSucceeded = currIterSucceeded && EZ_TEST_FLOAT(distanceMin, referenceDistanceMin, 0.0001f);
        currIterSucceeded = currIterSucceeded && EZ_TEST_FLOAT(distanceMax, referenceDistanceMax, 0.0001f);
        if (!currIterSucceeded)
        {
          break;
        }
      }
    }
  }
}
