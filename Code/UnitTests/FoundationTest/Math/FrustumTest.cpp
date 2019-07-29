#include <FoundationTestPCH.h>

#include <Foundation/Math/Frustum.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/Utilities/GraphicsUtils.h>

EZ_CREATE_SIMPLE_TEST(Math, Frustum)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFrustum (planes)")
  {
    ezFrustum f;

    ezPlane p[6];
    p[0].SetFromNormalAndPoint(ezVec3(1, 0, 0), ezVec3(1, 2, 3));
    p[1].SetFromNormalAndPoint(ezVec3(0, 1, 0), ezVec3(2, 3, 4));
    p[2].SetFromNormalAndPoint(ezVec3(0, 1, 0), ezVec3(2, 3, 4));
    p[3].SetFromNormalAndPoint(ezVec3(0, 1, 0), ezVec3(2, 3, 4));
    p[4].SetFromNormalAndPoint(ezVec3(0, 1, 0), ezVec3(2, 3, 4));
    p[5].SetFromNormalAndPoint(ezVec3(0, 1, 0), ezVec3(2, 3, 4));

    f.SetFrustum(p);

    EZ_TEST_BOOL(f.GetPlane(0) == p[0]);
    EZ_TEST_BOOL(f.GetPlane(1) == p[1]);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TransformFrustum")
  {
    ezFrustum f;

    ezPlane p[6];
    p[0].SetFromNormalAndPoint(ezVec3(1, 0, 0), ezVec3(1, 2, 3));
    p[1].SetFromNormalAndPoint(ezVec3(0, 1, 0), ezVec3(2, 3, 4));
    p[2].SetFromNormalAndPoint(ezVec3(0, 1, 0), ezVec3(2, 3, 4));
    p[3].SetFromNormalAndPoint(ezVec3(0, 1, 0), ezVec3(2, 3, 4));
    p[4].SetFromNormalAndPoint(ezVec3(0, 1, 0), ezVec3(2, 3, 4));
    p[5].SetFromNormalAndPoint(ezVec3(0, 1, 0), ezVec3(2, 3, 4));

    f.SetFrustum(p);

    ezMat4 mTransform;
    mTransform.SetRotationMatrixY(ezAngle::Degree(90.0f));
    mTransform.SetTranslationVector(ezVec3(2, 3, 4));

    f.TransformFrustum(mTransform);

    p[0].Transform(mTransform);
    p[1].Transform(mTransform);

    EZ_TEST_BOOL(f.GetPlane(0).IsEqual(p[0], 0.001f));
    EZ_TEST_BOOL(f.GetPlane(1).IsEqual(p[1], 0.001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "InvertFrustum")
  {
    ezFrustum f;

    ezPlane p[6];
    p[0].SetFromNormalAndPoint(ezVec3(1, 0, 0), ezVec3(1, 2, 3));
    p[1].SetFromNormalAndPoint(ezVec3(0, 1, 0), ezVec3(2, 3, 4));
    p[2].SetFromNormalAndPoint(ezVec3(0, 1, 0), ezVec3(2, 3, 4));
    p[3].SetFromNormalAndPoint(ezVec3(0, 1, 0), ezVec3(2, 3, 4));
    p[4].SetFromNormalAndPoint(ezVec3(0, 1, 0), ezVec3(2, 3, 4));
    p[5].SetFromNormalAndPoint(ezVec3(0, 1, 0), ezVec3(2, 3, 4));

    f.SetFrustum(p);

    f.InvertFrustum();

    p[0].Flip();
    p[1].Flip();

    EZ_TEST_BOOL(f.GetPlane(0) == p[0]);
    EZ_TEST_BOOL(f.GetPlane(1) == p[1]);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFrustum")
  {
    // check that the extracted frustum planes are always the same, no matter the handedness or depth-range

    // test the different depth ranges
    for (int r = 0; r < 2; ++r)
    {
      const ezClipSpaceDepthRange::Enum range = (r == 0) ? ezClipSpaceDepthRange::MinusOneToOne : ezClipSpaceDepthRange::ZeroToOne;

      // test rotated model-view matrices
      for (int rot = 0; rot < 360; rot += 45)
      {
        ezVec3 vLookDir;
        vLookDir.Set(ezMath::Sin(ezAngle::Degree(rot)), 0, -ezMath::Cos(ezAngle::Degree(rot)));

        ezVec3 vRightDir;
        vRightDir.Set(ezMath::Sin(ezAngle::Degree(rot + 90)), 0, -ezMath::Cos(ezAngle::Degree(rot + 90)));

        const ezVec3 vCamPos(rot, rot * 0.5f, rot * -0.3f);

        //const ezMat4 mViewLH = ezGraphicsUtils::CreateViewMatrix(vCamPos, vLookDir, -vRightDir, ezVec3(0, 1, 0), ezHandedness::LeftHanded);
        //const ezMat4 mViewRH = ezGraphicsUtils::CreateViewMatrix(vCamPos, vLookDir, vRightDir, ezVec3(0, 1, 0), ezHandedness::RightHanded);
        const ezMat4 mViewLH = ezGraphicsUtils::CreateLookAtViewMatrix(vCamPos, vCamPos + vLookDir, ezVec3(0, 1, 0), ezHandedness::LeftHanded);
        const ezMat4 mViewRH = ezGraphicsUtils::CreateLookAtViewMatrix(vCamPos, vCamPos + vLookDir, ezVec3(0, 1, 0), ezHandedness::RightHanded);

        const ezMat4 mProjLH = ezGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(ezAngle::Degree(90), 1.0f, 1.0f, 100.0f, range, ezClipSpaceYMode::Regular, ezHandedness::LeftHanded);
        const ezMat4 mProjRH = ezGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(ezAngle::Degree(90), 1.0f, 1.0f, 100.0f, range, ezClipSpaceYMode::Regular, ezHandedness::RightHanded);

        const ezMat4 mViewProjLH = mProjLH * mViewLH;
        const ezMat4 mViewProjRH = mProjRH * mViewRH;

        ezFrustum fLH, fRH, fB;
        fLH.SetFrustum(mViewProjLH, range, ezHandedness::LeftHanded);
        fRH.SetFrustum(mViewProjRH, range, ezHandedness::RightHanded);

        fB.SetFrustum(vCamPos, vLookDir, ezVec3(0, 1, 0), ezAngle::Degree(90), ezAngle::Degree(90), 1.0f, 100.0f);

        EZ_TEST_BOOL(fRH.GetPlane(ezFrustum::NearPlane).IsEqual(fB.GetPlane(ezFrustum::NearPlane), 0.1f));
        EZ_TEST_BOOL(fRH.GetPlane(ezFrustum::LeftPlane).IsEqual(fB.GetPlane(ezFrustum::LeftPlane), 0.1f));
        EZ_TEST_BOOL(fRH.GetPlane(ezFrustum::RightPlane).IsEqual(fB.GetPlane(ezFrustum::RightPlane), 0.1f));
        EZ_TEST_BOOL(fRH.GetPlane(ezFrustum::FarPlane).IsEqual(fB.GetPlane(ezFrustum::FarPlane), 0.1f));
        EZ_TEST_BOOL(fRH.GetPlane(ezFrustum::BottomPlane).IsEqual(fB.GetPlane(ezFrustum::BottomPlane), 0.1f));
        EZ_TEST_BOOL(fRH.GetPlane(ezFrustum::TopPlane).IsEqual(fB.GetPlane(ezFrustum::TopPlane), 0.1f));

        EZ_TEST_BOOL(fLH.GetPlane(ezFrustum::NearPlane).IsEqual(fB.GetPlane(ezFrustum::NearPlane), 0.1f));
        EZ_TEST_BOOL(fLH.GetPlane(ezFrustum::LeftPlane).IsEqual(fB.GetPlane(ezFrustum::LeftPlane), 0.1f));
        EZ_TEST_BOOL(fLH.GetPlane(ezFrustum::RightPlane).IsEqual(fB.GetPlane(ezFrustum::RightPlane), 0.1f));
        EZ_TEST_BOOL(fLH.GetPlane(ezFrustum::FarPlane).IsEqual(fB.GetPlane(ezFrustum::FarPlane), 0.1f));
        EZ_TEST_BOOL(fLH.GetPlane(ezFrustum::BottomPlane).IsEqual(fB.GetPlane(ezFrustum::BottomPlane), 0.1f));
        EZ_TEST_BOOL(fLH.GetPlane(ezFrustum::TopPlane).IsEqual(fB.GetPlane(ezFrustum::TopPlane), 0.1f));
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Culling")
  {
    const ezVec3 offsetPos(23, 17, -9);
    const ezVec3 camDir[6] = {ezVec3(-1, 0, 0), ezVec3(1, 0, 0), ezVec3(0, -1, 0), ezVec3(0, 1, 0), ezVec3(0, 0, -1), ezVec3(0, 0, 1)};
    const ezVec3 objPos[6] = {ezVec3(-9, 0, 0), ezVec3(9, 0, 0), ezVec3(0, -9, 0), ezVec3(0, 9, 0), ezVec3(0, 0, -9), ezVec3(0, 0, 9)};

    for (ezUInt32 dir = 0; dir < 6; ++dir)
    {
      ezFrustum fDir;
      fDir.SetFrustum(
        offsetPos, camDir[dir], camDir[dir].GetOrthogonalVector() /*arbitrary*/, ezAngle::Degree(90), ezAngle::Degree(90), 1.0f, 100.0f);

      for (ezUInt32 obj = 0; obj < 6; ++obj)
      {
        // box
        {
          ezBoundingBox boundingObj;
          boundingObj.SetCenterAndHalfExtents(offsetPos + objPos[obj], ezVec3(1.0f));

          const ezVolumePosition::Enum res = fDir.GetObjectPosition(boundingObj);

          if (obj == dir)
            EZ_TEST_BOOL(res == ezVolumePosition::Inside);
          else
            EZ_TEST_BOOL(res == ezVolumePosition::Outside);
        }

        // sphere
        {
          ezBoundingSphere boundingObj;
          boundingObj.SetElements(offsetPos + objPos[obj], 0.93f);

          const ezVolumePosition::Enum res = fDir.GetObjectPosition(boundingObj);

          if (obj == dir)
            EZ_TEST_BOOL(res == ezVolumePosition::Inside);
          else
            EZ_TEST_BOOL(res == ezVolumePosition::Outside);
        }

        // vertices
        {
          ezBoundingBox boundingObj;
          boundingObj.SetCenterAndHalfExtents(offsetPos + objPos[obj], ezVec3(1.0f));

          ezVec3 vertices[8];
          boundingObj.GetCorners(vertices);

          const ezVolumePosition::Enum res = fDir.GetObjectPosition(vertices, 8);

          if (obj == dir)
            EZ_TEST_BOOL(res == ezVolumePosition::Inside);
          else
            EZ_TEST_BOOL(res == ezVolumePosition::Outside);
        }

        // vertices + transform
        {
          ezBoundingBox boundingObj;
          boundingObj.SetCenterAndHalfExtents(objPos[obj], ezVec3(1.0f));

          ezVec3 vertices[8];
          boundingObj.GetCorners(vertices);

          ezMat4 transform;
          transform.SetTranslationMatrix(offsetPos);

          const ezVolumePosition::Enum res = fDir.GetObjectPosition(vertices, 8, transform);

          if (obj == dir)
            EZ_TEST_BOOL(res == ezVolumePosition::Inside);
          else
            EZ_TEST_BOOL(res == ezVolumePosition::Outside);
        }

        // SIMD box
        {
          ezBoundingBox boundingObj;
          boundingObj.SetCenterAndHalfExtents(offsetPos + objPos[obj], ezVec3(1.0f));

          const bool res = fDir.Overlaps(ezSimdConversion::ToBBox(boundingObj));

          if (obj == dir)
            EZ_TEST_BOOL(res == true);
          else
            EZ_TEST_BOOL(res == false);
        }

        // SIMD sphere
        {
          ezBoundingSphere boundingObj;
          boundingObj.SetElements(offsetPos + objPos[obj], 0.93f);

          const bool res = fDir.Overlaps(ezSimdConversion::ToBSphere(boundingObj));

          if (obj == dir)
            EZ_TEST_BOOL(res == true);
          else
            EZ_TEST_BOOL(res == false);
        }
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ComputeCornerPoints")
  {
    const ezMat4 mProj = ezGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(
      ezAngle::Degree(90), 1.0f, 1.0f, 10.0f, ezClipSpaceDepthRange::MinusOneToOne, ezClipSpaceYMode::Regular, ezHandedness::RightHanded);

    ezFrustum frustum[2];
    frustum[0].SetFrustum(mProj, ezClipSpaceDepthRange::MinusOneToOne, ezHandedness::RightHanded);
    frustum[1].SetFrustum(ezVec3::ZeroVector(), ezVec3(0, 0, -1), ezVec3(0, 1, 0), ezAngle::Degree(90), ezAngle::Degree(90), 1.0f, 10.0f);

    for (int f = 0; f < 2; ++f)
    {
      ezVec3 corner[8];
      frustum[f].ComputeCornerPoints(corner);

      ezPositionOnPlane::Enum results[8][6];

      for (int c = 0; c < 8; ++c)
      {
        for (int p = 0; p < 6; ++p)
        {
          results[c][p] = ezPositionOnPlane::Back;
        }
      }

      results[ezFrustum::FrustumCorner::NearTopLeft][ezFrustum::PlaneType::NearPlane] = ezPositionOnPlane::OnPlane;
      results[ezFrustum::FrustumCorner::NearTopLeft][ezFrustum::PlaneType::TopPlane] = ezPositionOnPlane::OnPlane;
      results[ezFrustum::FrustumCorner::NearTopLeft][ezFrustum::PlaneType::LeftPlane] = ezPositionOnPlane::OnPlane;

      results[ezFrustum::FrustumCorner::NearTopRight][ezFrustum::PlaneType::NearPlane] = ezPositionOnPlane::OnPlane;
      results[ezFrustum::FrustumCorner::NearTopRight][ezFrustum::PlaneType::TopPlane] = ezPositionOnPlane::OnPlane;
      results[ezFrustum::FrustumCorner::NearTopRight][ezFrustum::PlaneType::RightPlane] = ezPositionOnPlane::OnPlane;

      results[ezFrustum::FrustumCorner::NearBottomLeft][ezFrustum::PlaneType::NearPlane] = ezPositionOnPlane::OnPlane;
      results[ezFrustum::FrustumCorner::NearBottomLeft][ezFrustum::PlaneType::BottomPlane] = ezPositionOnPlane::OnPlane;
      results[ezFrustum::FrustumCorner::NearBottomLeft][ezFrustum::PlaneType::LeftPlane] = ezPositionOnPlane::OnPlane;

      results[ezFrustum::FrustumCorner::NearBottomRight][ezFrustum::PlaneType::NearPlane] = ezPositionOnPlane::OnPlane;
      results[ezFrustum::FrustumCorner::NearBottomRight][ezFrustum::PlaneType::BottomPlane] = ezPositionOnPlane::OnPlane;
      results[ezFrustum::FrustumCorner::NearBottomRight][ezFrustum::PlaneType::RightPlane] = ezPositionOnPlane::OnPlane;

      results[ezFrustum::FrustumCorner::FarTopLeft][ezFrustum::PlaneType::FarPlane] = ezPositionOnPlane::OnPlane;
      results[ezFrustum::FrustumCorner::FarTopLeft][ezFrustum::PlaneType::TopPlane] = ezPositionOnPlane::OnPlane;
      results[ezFrustum::FrustumCorner::FarTopLeft][ezFrustum::PlaneType::LeftPlane] = ezPositionOnPlane::OnPlane;

      results[ezFrustum::FrustumCorner::FarTopRight][ezFrustum::PlaneType::FarPlane] = ezPositionOnPlane::OnPlane;
      results[ezFrustum::FrustumCorner::FarTopRight][ezFrustum::PlaneType::TopPlane] = ezPositionOnPlane::OnPlane;
      results[ezFrustum::FrustumCorner::FarTopRight][ezFrustum::PlaneType::RightPlane] = ezPositionOnPlane::OnPlane;

      results[ezFrustum::FrustumCorner::FarBottomLeft][ezFrustum::PlaneType::FarPlane] = ezPositionOnPlane::OnPlane;
      results[ezFrustum::FrustumCorner::FarBottomLeft][ezFrustum::PlaneType::BottomPlane] = ezPositionOnPlane::OnPlane;
      results[ezFrustum::FrustumCorner::FarBottomLeft][ezFrustum::PlaneType::LeftPlane] = ezPositionOnPlane::OnPlane;

      results[ezFrustum::FrustumCorner::FarBottomRight][ezFrustum::PlaneType::FarPlane] = ezPositionOnPlane::OnPlane;
      results[ezFrustum::FrustumCorner::FarBottomRight][ezFrustum::PlaneType::BottomPlane] = ezPositionOnPlane::OnPlane;
      results[ezFrustum::FrustumCorner::FarBottomRight][ezFrustum::PlaneType::RightPlane] = ezPositionOnPlane::OnPlane;

      for (int c = 0; c < 8; ++c)
      {
        ezFrustum::FrustumCorner cornerName = (ezFrustum::FrustumCorner)c;

        for (int p = 0; p < 6; ++p)
        {
          ezFrustum::PlaneType planeName = (ezFrustum::PlaneType)p;

          ezPlane plane = frustum[f].GetPlane(planeName);
          ezPositionOnPlane::Enum expected = results[cornerName][planeName];
          ezPositionOnPlane::Enum result = plane.GetPointPosition(corner[cornerName], 0.1f);
          // float fDistToPlane = plane.GetDistanceTo(corner[cornerName]);
          EZ_TEST_BOOL(result == expected);
        }
      }
    }
  }
}
