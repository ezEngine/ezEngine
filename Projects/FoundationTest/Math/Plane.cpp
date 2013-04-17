#include <PCH.h>

EZ_CREATE_SIMPLE_TEST(Math, Plane)
{
  EZ_TEST_BLOCK(true, "Default Constructor")
  {
    #if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
        // In debug the default constructor initializes everything with NaN.
        ezPlane p;
        EZ_TEST(ezMath::IsNaN(p.m_vNormal.x) && ezMath::IsNaN(p.m_vNormal.y) && ezMath::IsNaN(p.m_vNormal.z) && ezMath::IsNaN(p.m_fNegDistance));
    #else
        // Placement new of the default constructor should not have any effect on the previous data.
        float testBlock[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
        ezPlane* p = ::new ((void*) &testBlock[0]) ezPlane;
        EZ_TEST(p->m_vNormal.x == 1.0f && p->m_vNormal.y == 2.0f && p->m_vNormal.z == 3.0f && p->m_fNegDistance == 4.0f);
    #endif
  }

  EZ_TEST_BLOCK(true, "Constructor(Normal, Point)")
  {
    ezPlane p (ezVec3(1, 0, 0), ezVec3(5, 3, 1));

    EZ_TEST(p.m_vNormal == ezVec3(1, 0, 0));
    EZ_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(true, "Constructor(Point, Point, Point)")
  {
    ezPlane p (ezVec3(-1, 5, 1), ezVec3(1, 5, 1), ezVec3(0, 5, -5));

    EZ_TEST_VEC3(p.m_vNormal, ezVec3(0, 1, 0), 0.0001f);
    EZ_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(true, "Constructor(Points)")
  {
    ezVec3 v[3] = { ezVec3(-1, 5, 1), ezVec3(1, 5, 1), ezVec3(0, 5, -5) };

    ezPlane p (v);

    EZ_TEST_VEC3(p.m_vNormal, ezVec3(0, 1, 0), 0.0001f);
    EZ_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(true, "Constructor(Points, numpoints)")
  {
    ezVec3 v[6] = { ezVec3(-1, 5, 1), ezVec3(-1, 5, 1), ezVec3(1, 5, 1), ezVec3(1, 5, 1), ezVec3(0, 5, -5), ezVec3(0, 5, -5) };

    ezPlane p (v, 6);

    EZ_TEST_VEC3(p.m_vNormal, ezVec3(0, 1, 0), 0.0001f);
    EZ_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(true, "SetFromNormalAndPoint")
  {
    ezPlane p;
    p.SetFromNormalAndPoint(ezVec3(1, 0, 0), ezVec3(5, 3, 1));

    EZ_TEST(p.m_vNormal == ezVec3(1, 0, 0));
    EZ_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(true, "SetFromPoints")
  {
    ezPlane p;
    p.SetFromPoints(ezVec3(-1, 5, 1), ezVec3(1, 5, 1), ezVec3(0, 5, -5));

    EZ_TEST_VEC3(p.m_vNormal, ezVec3(0, 1, 0), 0.0001f);
    EZ_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(true, "SetFromPoints")
  {
    ezVec3 v[3] = { ezVec3(-1, 5, 1), ezVec3(1, 5, 1), ezVec3(0, 5, -5) };

    ezPlane p;
    p.SetFromPoints(v);

    EZ_TEST_VEC3(p.m_vNormal, ezVec3(0, 1, 0), 0.0001f);
    EZ_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(true, "SetFromPoints")
  {
    ezVec3 v[6] = { ezVec3(-1, 5, 1), ezVec3(-1, 5, 1), ezVec3(1, 5, 1), ezVec3(1, 5, 1), ezVec3(0, 5, -5), ezVec3(0, 5, -5) };

    ezPlane p;
    p.SetFromPoints(v, 6);

    EZ_TEST_VEC3(p.m_vNormal, ezVec3(0, 1, 0), 0.0001f);
    EZ_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(true, "SetFromDirections")
  {
    ezPlane p;
    p.SetFromDirections(ezVec3(1, 0, 0), ezVec3(1, 0, -1), ezVec3(3, 5, 9));

    EZ_TEST_VEC3(p.m_vNormal, ezVec3(0, 1, 0), 0.0001f);
    EZ_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(true, "SetInvalid")
  {
    ezPlane p;
    p.SetFromDirections(ezVec3(1, 0, 0), ezVec3(1, 0, -1), ezVec3(3, 5, 9));

    p.SetInvalid();

    EZ_TEST_VEC3(p.m_vNormal, ezVec3(0, 0, 0), 0.0001f);
    EZ_TEST_FLOAT(p.m_fNegDistance, 0.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(true, "GetDistanceTo")
  {
    ezPlane p(ezVec3(1, 0, 0), ezVec3(5, 0, 0));

    EZ_TEST_FLOAT(p.GetDistanceTo(ezVec3(10, 3, 5)), 5.0f, 0.0001f);
    EZ_TEST_FLOAT(p.GetDistanceTo(ezVec3(0, 7, 123)), -5.0f, 0.0001f);
    EZ_TEST_FLOAT(p.GetDistanceTo(ezVec3(5, 12, 23)), 0.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(true, "GetMinimumDistanceTo")
  {
    ezVec3 v1[3] = { ezVec3(15, 3, 5), ezVec3(6, 7, 123), ezVec3(10, 12, 23)};
    ezVec3 v2[3] = { ezVec3(3, 3, 5), ezVec3(5, 7, 123), ezVec3(10, 12, 23)};

    ezPlane p(ezVec3(1, 0, 0), ezVec3(5, 0, 0));

    EZ_TEST_FLOAT(p.GetMinimumDistanceTo(v1, 3), 1.0f, 0.0001f);
    EZ_TEST_FLOAT(p.GetMinimumDistanceTo(v2, 3), -2.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(true, "GetMinMaxDistanceTo")
  {
    ezVec3 v1[3] = { ezVec3(15, 3, 5), ezVec3(5, 7, 123), ezVec3(0, 12, 23)};
    ezVec3 v2[3] = { ezVec3(8, 3, 5), ezVec3(6, 7, 123), ezVec3(10, 12, 23)};

    ezPlane p(ezVec3(1, 0, 0), ezVec3(5, 0, 0));

    float fmin, fmax;

    p.GetMinMaxDistanceTo(fmin, fmax, v1, 3);
    EZ_TEST_FLOAT(fmin, -5.0f, 0.0001f);
    EZ_TEST_FLOAT(fmax, 10.0f, 0.0001f);

    p.GetMinMaxDistanceTo(fmin, fmax, v2, 3);
    EZ_TEST_FLOAT(fmin, 1, 0.0001f);
    EZ_TEST_FLOAT(fmax, 5, 0.0001f);
  }

  EZ_TEST_BLOCK(true, "GetPointPosition")
  {
    ezPlane p(ezVec3(0, 1, 0), ezVec3(0, 10, 0));

    EZ_TEST(p.GetPointPosition(ezVec3(0, 15, 0)) == ezPositionOnPlane::Front);
    EZ_TEST(p.GetPointPosition(ezVec3(0, 5, 0)) == ezPositionOnPlane::Back);
  }

  EZ_TEST_BLOCK(true, "GetPointPosition(planewidth)")
  {
    ezPlane p(ezVec3(0, 1, 0), ezVec3(0, 10, 0));

    EZ_TEST(p.GetPointPosition(ezVec3(0, 15, 0), 0.01f) == ezPositionOnPlane::Front);
    EZ_TEST(p.GetPointPosition(ezVec3(0, 5, 0), 0.01f) == ezPositionOnPlane::Back);
    EZ_TEST(p.GetPointPosition(ezVec3(0, 10, 0), 0.01f) == ezPositionOnPlane::OnPlane);
  }

  EZ_TEST_BLOCK(true, "GetObjectPosition")
  {
    ezPlane p(ezVec3(1, 0, 0), ezVec3(10, 0, 0));

    ezVec3 v0[3] = { ezVec3(12, 0, 0), ezVec3(15, 0, 0), ezVec3(20, 0, 0) };
    ezVec3 v1[3] = { ezVec3(8, 0, 0), ezVec3(6, 0, 0), ezVec3(4, 0, 0) };
    ezVec3 v2[3] = { ezVec3(12, 0, 0), ezVec3(6, 0, 0), ezVec3(4, 0, 0) };

    EZ_TEST(p.GetObjectPosition(v0, 3) == ezPositionOnPlane::Front);
    EZ_TEST(p.GetObjectPosition(v1, 3) == ezPositionOnPlane::Back);
    EZ_TEST(p.GetObjectPosition(v2, 3) == ezPositionOnPlane::Spanning);
  }

  EZ_TEST_BLOCK(true, "GetObjectPosition(fPlaneHalfWidth)")
  {
    ezPlane p(ezVec3(1, 0, 0), ezVec3(10, 0, 0));

    ezVec3 v0[3] = { ezVec3(12, 0, 0), ezVec3(15, 0, 0), ezVec3(20, 0, 0) };
    ezVec3 v1[3] = { ezVec3(8, 0, 0), ezVec3(6, 0, 0), ezVec3(4, 0, 0) };
    ezVec3 v2[3] = { ezVec3(12, 0, 0), ezVec3(6, 0, 0), ezVec3(4, 0, 0) };
    ezVec3 v3[3] = { ezVec3(10, 1, 0), ezVec3(10, 5, 7), ezVec3(10, 3, -5) };

    EZ_TEST(p.GetObjectPosition(v0, 3, 0.001f) == ezPositionOnPlane::Front);
    EZ_TEST(p.GetObjectPosition(v1, 3, 0.001f) == ezPositionOnPlane::Back);
    EZ_TEST(p.GetObjectPosition(v2, 3, 0.001f) == ezPositionOnPlane::Spanning);
    EZ_TEST(p.GetObjectPosition(v3, 3, 0.001f) == ezPositionOnPlane::OnPlane);
  }

  EZ_TEST_BLOCK(true, "ProjectOntoPlane")
  {
    ezPlane p(ezVec3(0, 1, 0), ezVec3(0, 10, 0));

    EZ_TEST_VEC3(p.ProjectOntoPlane(ezVec3(3, 15, 2)), ezVec3(3, 10, 2), 0.001f);
    EZ_TEST_VEC3(p.ProjectOntoPlane(ezVec3(-1, 5, -5)), ezVec3(-1, 10, -5), 0.001f);
  }

  EZ_TEST_BLOCK(true, "Mirror")
  {
    ezPlane p(ezVec3(0, 1, 0), ezVec3(0, 10, 0));

    EZ_TEST_VEC3(p.Mirror(ezVec3(3, 15, 2)), ezVec3(3, 5, 2), 0.001f);
    EZ_TEST_VEC3(p.Mirror(ezVec3(-1, 5, -5)), ezVec3(-1, 15, -5), 0.001f);
  }

  EZ_TEST_BLOCK(true, "GetCoplanarDirection")
  {
    ezPlane p(ezVec3(0, 1, 0), ezVec3(0, 10, 0));

    EZ_TEST_VEC3(p.GetCoplanarDirection(ezVec3(0, 1, 0)), ezVec3(0, 0, 0), 0.001f);
    EZ_TEST_VEC3(p.GetCoplanarDirection(ezVec3(1, 1, 0)).GetNormalized(), ezVec3(1, 0, 0), 0.001f);
    EZ_TEST_VEC3(p.GetCoplanarDirection(ezVec3(-1, 1, 0)).GetNormalized(), ezVec3(-1, 0, 0), 0.001f);
    EZ_TEST_VEC3(p.GetCoplanarDirection(ezVec3(0, 1, 1)).GetNormalized(), ezVec3(0, 0, 1), 0.001f);
    EZ_TEST_VEC3(p.GetCoplanarDirection(ezVec3(0, 1, -1)).GetNormalized(), ezVec3(0, 0, -1), 0.001f);
  }

  EZ_TEST_BLOCK(true, "IsIdentical / operator== / operator!=")
  {
    ezPlane p1(ezVec3(0, 1, 0), ezVec3(0, 10, 0));
    ezPlane p2(ezVec3(0, 1, 0), ezVec3(0, 10, 0));
    ezPlane p3(ezVec3(0, 1, 0), ezVec3(0, 10.00001f, 0));

    EZ_TEST(p1.IsIdentical(p1));
    EZ_TEST(p2.IsIdentical(p2));
    EZ_TEST(p3.IsIdentical(p3));

    EZ_TEST(p1.IsIdentical(p2));
    EZ_TEST(p2.IsIdentical(p1));

    EZ_TEST(!p1.IsIdentical(p3));
    EZ_TEST(!p2.IsIdentical(p3));


    EZ_TEST(p1 == p2);
    EZ_TEST(p2 == p1);

    EZ_TEST(p1 != p3);
    EZ_TEST(p2 != p3);
  }

  EZ_TEST_BLOCK(true, "IsEqual")
  {
    ezPlane p1(ezVec3(0, 1, 0), ezVec3(0, 10, 0));
    ezPlane p2(ezVec3(0, 1, 0), ezVec3(0, 10, 0));
    ezPlane p3(ezVec3(0, 1, 0), ezVec3(0, 10.00001f, 0));

    EZ_TEST(p1.IsEqual(p1));
    EZ_TEST(p2.IsEqual(p2));
    EZ_TEST(p3.IsEqual(p3));

    EZ_TEST(p1.IsEqual(p2));
    EZ_TEST(p2.IsEqual(p1));

    EZ_TEST(p1.IsEqual(p3));
    EZ_TEST(p2.IsEqual(p3));
  }

  EZ_TEST_BLOCK(true, "IsValid")
  {
    ezPlane p1(ezVec3(0, 1, 0), ezVec3(0, 10, 0));

    EZ_TEST(p1.IsValid());

    p1.SetInvalid();
    EZ_TEST(!p1.IsValid());
  }

  EZ_TEST_BLOCK(true, "Transform(Mat3)")
  {
    ezPlane p(ezVec3(0, 1, 0), ezVec3(0, 10, 0));

    ezMat3 m;
    m.SetRotationMatrixX(90);

    p.Transform(m);

    EZ_TEST_VEC3(p.m_vNormal, ezVec3(0, 0, 1), 0.0001f);
    EZ_TEST_FLOAT(p.m_fNegDistance, -10.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(true, "Transform(Mat4)")
  {
    {
      ezPlane p(ezVec3(0, 1, 0), ezVec3(0, 10, 0));

      ezMat4 m;
      m.SetRotationMatrixX(90);
      m.SetTranslationVector(ezVec3(0, 5, 0));

      p.Transform(m);

      EZ_TEST_VEC3(p.m_vNormal, ezVec3(0, 0, 1), 0.0001f);
      EZ_TEST_FLOAT(p.m_fNegDistance, -10.0f, 0.0001f);
    }

    {
      ezPlane p(ezVec3(0, 1, 0), ezVec3(0, 10, 0));

      ezMat4 m;
      m.SetRotationMatrixX(90);
      m.SetTranslationVector(ezVec3(0, 0, 5));

      p.Transform(m);

      EZ_TEST_VEC3(p.m_vNormal, ezVec3(0, 0, 1), 0.0001f);
      EZ_TEST_FLOAT(p.m_fNegDistance, -15.0f, 0.0001f);
    }
  }

  EZ_TEST_BLOCK(true, "Flip")
  {
    ezPlane p(ezVec3(0, 1, 0), ezVec3(0, 10, 0));

    EZ_TEST_VEC3(p.m_vNormal, ezVec3(0, 1, 0), 0.0001f);
    EZ_TEST_FLOAT(p.m_fNegDistance, -10.0f, 0.0001f);

    p.Flip();

    EZ_TEST_VEC3(p.m_vNormal, ezVec3(0, -1, 0), 0.0001f);
    EZ_TEST_FLOAT(p.m_fNegDistance, 10.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(true, "FlipIfNecessary")
  {
    {
      ezPlane p(ezVec3(0, 1, 0), ezVec3(0, 10, 0));

      EZ_TEST_VEC3(p.m_vNormal, ezVec3(0, 1, 0), 0.0001f);
      EZ_TEST_FLOAT(p.m_fNegDistance, -10.0f, 0.0001f);

      EZ_TEST(p.FlipIfNecessary(ezVec3(0, 11, 0), true) == false);

      EZ_TEST_VEC3(p.m_vNormal, ezVec3(0, 1, 0), 0.0001f);
      EZ_TEST_FLOAT(p.m_fNegDistance, -10.0f, 0.0001f);
    }

    {
      ezPlane p(ezVec3(0, 1, 0), ezVec3(0, 10, 0));

      EZ_TEST_VEC3(p.m_vNormal, ezVec3(0, 1, 0), 0.0001f);
      EZ_TEST_FLOAT(p.m_fNegDistance, -10.0f, 0.0001f);

      EZ_TEST(p.FlipIfNecessary(ezVec3(0, 11, 0), false) == true);

      EZ_TEST_VEC3(p.m_vNormal, ezVec3(0, -1, 0), 0.0001f);
      EZ_TEST_FLOAT(p.m_fNegDistance, 10.0f, 0.0001f);
    }
  }

  EZ_TEST_BLOCK(true, "GetRayIntersection")
  {
    ezPlane p(ezVec3(0, 1, 0), ezVec3(0, 10, 0));

    float f;
    ezVec3 v;

    EZ_TEST(p.GetRayIntersection(ezVec3(3, 1, 7), ezVec3(0, 1, 0), &f, &v));
    EZ_TEST_FLOAT(f, 9, 0.0001f);
    EZ_TEST_VEC3(v, ezVec3(3, 10, 7), 0.0001f);

    EZ_TEST(!p.GetRayIntersection(ezVec3(3, 1, 7), ezVec3(1, 0, 0), &f, &v));
    EZ_TEST(!p.GetRayIntersection(ezVec3(3, 1, 7), ezVec3(0, -1, 0), &f, &v));
  }

  EZ_TEST_BLOCK(true, "GetRayIntersectionBiDirectional")
  {
    ezPlane p(ezVec3(0, 1, 0), ezVec3(0, 10, 0));

    float f;
    ezVec3 v;

    EZ_TEST(p.GetRayIntersectionBiDirectional(ezVec3(3, 1, 7), ezVec3(0, 1, 0), &f, &v));
    EZ_TEST_FLOAT(f, 9, 0.0001f);
    EZ_TEST_VEC3(v, ezVec3(3, 10, 7), 0.0001f);

    EZ_TEST(!p.GetRayIntersectionBiDirectional(ezVec3(3, 1, 7), ezVec3(1, 0, 0), &f, &v));

    EZ_TEST(p.GetRayIntersectionBiDirectional(ezVec3(3, 1, 7), ezVec3(0, -1, 0), &f, &v));
    EZ_TEST_FLOAT(f, -9, 0.0001f);
    EZ_TEST_VEC3(v, ezVec3(3, 10, 7), 0.0001f);
  }

  EZ_TEST_BLOCK(true, "GetLineSegmentIntersection")
  {
    ezPlane p(ezVec3(0, 1, 0), ezVec3(0, 10, 0));

    float f;
    ezVec3 v;

    EZ_TEST(p.GetLineSegmentIntersection(ezVec3(3, 5, 7), ezVec3(3, 15, 7), &f, &v));
    EZ_TEST_FLOAT(f, 0.5f, 0.0001f);
    EZ_TEST_VEC3(v, ezVec3(3, 10, 7), 0.0001f);

    EZ_TEST(!p.GetLineSegmentIntersection(ezVec3(3, 5, 7), ezVec3(13, 5, 7), &f, &v));
  }

  EZ_TEST_BLOCK(true, "GetPlanesIntersectionPoint")
  {
    ezPlane p1(ezVec3(1, 0, 0), ezVec3(0, 10, 0));
    ezPlane p2(ezVec3(0, 1, 0), ezVec3(0, 10, 0));
    ezPlane p3(ezVec3(0, 0, 1), ezVec3(0, 10, 0));

    ezVec3 r;

    EZ_TEST(ezPlane::GetPlanesIntersectionPoint(p1, p2, p3, r) == EZ_SUCCESS);
    EZ_TEST_VEC3(r, ezVec3(0, 10, 0), 0.0001f);

    EZ_TEST(ezPlane::GetPlanesIntersectionPoint(p1, p1, p3, r) == EZ_FAILURE);
    EZ_TEST(ezPlane::GetPlanesIntersectionPoint(p1, p2, p2, r) == EZ_FAILURE);
    EZ_TEST(ezPlane::GetPlanesIntersectionPoint(p3, p2, p3, r) == EZ_FAILURE);
  }

  EZ_TEST_BLOCK(true, "FindSupportPoints")
  {
    ezVec3 v[6] = { ezVec3(-1, 5, 1), ezVec3(-1, 5, 1), ezVec3(1, 5, 1), ezVec3(1, 5, 1), ezVec3(0, 5, -5), ezVec3(0, 5, -5) };

    ezInt32 i1, i2, i3;

    ezPlane::FindSupportPoints(v, 6, i1, i2, i3);

    EZ_TEST_INT(i1, 0);
    EZ_TEST_INT(i2, 2);
    EZ_TEST_INT(i3, 4);  
  }

}

