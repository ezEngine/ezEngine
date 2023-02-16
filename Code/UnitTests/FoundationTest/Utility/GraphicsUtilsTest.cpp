#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Utilities/GraphicsUtils.h>

EZ_CREATE_SIMPLE_TEST(Utility, GraphicsUtils)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Perspective (-1/1): ConvertWorldPosToScreenPos / ConvertScreenPosToWorldPos")
  {
    ezMat4 mProj, mProjInv;

    mProj = ezGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(
      ezAngle::Degree(85.0f), 2.0f, 1.0f, 1000.0f, ezClipSpaceDepthRange::MinusOneToOne, ezClipSpaceYMode::Regular, ezHandedness::LeftHanded);
    mProjInv = mProj.GetInverse();

    for (ezUInt32 y = 0; y < 25; ++y)
    {
      for (ezUInt32 x = 0; x < 50; ++x)
      {
        ezVec3 vPoint, vDir;
        EZ_TEST_BOOL(ezGraphicsUtils::ConvertScreenPosToWorldPos(
          mProjInv, 0, 0, 50, 25, ezVec3((float)x, (float)y, 0.5f), vPoint, &vDir, ezClipSpaceDepthRange::MinusOneToOne)
                       .Succeeded());

        EZ_TEST_VEC3(vDir, vPoint.GetNormalized(), 0.01f);

        ezVec3 vScreen;
        EZ_TEST_BOOL(
          ezGraphicsUtils::ConvertWorldPosToScreenPos(mProj, 0, 0, 50, 25, vPoint, vScreen, ezClipSpaceDepthRange::MinusOneToOne).Succeeded());

        EZ_TEST_VEC3(vScreen, ezVec3((float)x, (float)y, 0.5f), 0.01f);
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Perspective (0/1): ConvertWorldPosToScreenPos / ConvertScreenPosToWorldPos")
  {
    ezMat4 mProj, mProjInv;
    mProj = ezGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(
      ezAngle::Degree(85.0f), 2.0f, 1.0f, 1000.0f, ezClipSpaceDepthRange::ZeroToOne, ezClipSpaceYMode::Regular, ezHandedness::LeftHanded);
    mProjInv = mProj.GetInverse();

    for (ezUInt32 y = 0; y < 25; ++y)
    {
      for (ezUInt32 x = 0; x < 50; ++x)
      {
        ezVec3 vPoint, vDir;
        EZ_TEST_BOOL(ezGraphicsUtils::ConvertScreenPosToWorldPos(
          mProjInv, 0, 0, 50, 25, ezVec3((float)x, (float)y, 0.5f), vPoint, &vDir, ezClipSpaceDepthRange::ZeroToOne)
                       .Succeeded());

        EZ_TEST_VEC3(vDir, vPoint.GetNormalized(), 0.01f);

        ezVec3 vScreen;
        EZ_TEST_BOOL(ezGraphicsUtils::ConvertWorldPosToScreenPos(mProj, 0, 0, 50, 25, vPoint, vScreen, ezClipSpaceDepthRange::ZeroToOne).Succeeded());

        EZ_TEST_VEC3(vScreen, ezVec3((float)x, (float)y, 0.5f), 0.01f);
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Ortho (-1/1): ConvertWorldPosToScreenPos / ConvertScreenPosToWorldPos")
  {
    ezMat4 mProj, mProjInv;
    mProj = ezGraphicsUtils::CreateOrthographicProjectionMatrix(
      50, 25, 1.0f, 1000.0f, ezClipSpaceDepthRange::MinusOneToOne, ezClipSpaceYMode::Regular, ezHandedness::LeftHanded);

    mProjInv = mProj.GetInverse();

    for (ezUInt32 y = 0; y < 25; ++y)
    {
      for (ezUInt32 x = 0; x < 50; ++x)
      {
        ezVec3 vPoint, vDir;
        EZ_TEST_BOOL(ezGraphicsUtils::ConvertScreenPosToWorldPos(
          mProjInv, 0, 0, 50, 25, ezVec3((float)x, (float)y, 0.5f), vPoint, &vDir, ezClipSpaceDepthRange::MinusOneToOne)
                       .Succeeded());

        EZ_TEST_VEC3(vDir, ezVec3(0, 0, 1.0f), 0.01f);

        ezVec3 vScreen;
        EZ_TEST_BOOL(
          ezGraphicsUtils::ConvertWorldPosToScreenPos(mProj, 0, 0, 50, 25, vPoint, vScreen, ezClipSpaceDepthRange::MinusOneToOne).Succeeded());

        EZ_TEST_VEC3(vScreen, ezVec3((float)x, (float)y, 0.5f), 0.01f);
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Ortho (0/1): ConvertWorldPosToScreenPos / ConvertScreenPosToWorldPos")
  {
    ezMat4 mProj, mProjInv;
    mProj = ezGraphicsUtils::CreateOrthographicProjectionMatrix(
      50, 25, 1.0f, 1000.0f, ezClipSpaceDepthRange::ZeroToOne, ezClipSpaceYMode::Regular, ezHandedness::LeftHanded);
    mProjInv = mProj.GetInverse();

    for (ezUInt32 y = 0; y < 25; ++y)
    {
      for (ezUInt32 x = 0; x < 50; ++x)
      {
        ezVec3 vPoint, vDir;
        EZ_TEST_BOOL(ezGraphicsUtils::ConvertScreenPosToWorldPos(
          mProjInv, 0, 0, 50, 25, ezVec3((float)x, (float)y, 0.5f), vPoint, &vDir, ezClipSpaceDepthRange::ZeroToOne)
                       .Succeeded());

        EZ_TEST_VEC3(vDir, ezVec3(0, 0, 1.0f), 0.01f);

        ezVec3 vScreen;
        EZ_TEST_BOOL(ezGraphicsUtils::ConvertWorldPosToScreenPos(mProj, 0, 0, 50, 25, vPoint, vScreen, ezClipSpaceDepthRange::ZeroToOne).Succeeded());

        EZ_TEST_VEC3(vScreen, ezVec3((float)x, (float)y, 0.5f), 0.01f);
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ConvertProjectionMatrixDepthRange")
  {
    ezMat4 mProj1, mProj2;
    mProj1 = ezGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(
      ezAngle::Degree(85.0f), 2.0f, 1.0f, 1000.0f, ezClipSpaceDepthRange::ZeroToOne, ezClipSpaceYMode::Regular, ezHandedness::LeftHanded);
    mProj2 = ezGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(
      ezAngle::Degree(85.0f), 2.0f, 1.0f, 1000.0f, ezClipSpaceDepthRange::MinusOneToOne, ezClipSpaceYMode::Regular, ezHandedness::LeftHanded);

    ezMat4 mProj1b = mProj1;
    ezMat4 mProj2b = mProj2;
    ezGraphicsUtils::ConvertProjectionMatrixDepthRange(mProj1b, ezClipSpaceDepthRange::ZeroToOne, ezClipSpaceDepthRange::MinusOneToOne);
    ezGraphicsUtils::ConvertProjectionMatrixDepthRange(mProj2b, ezClipSpaceDepthRange::MinusOneToOne, ezClipSpaceDepthRange::ZeroToOne);

    EZ_TEST_BOOL(mProj1.IsEqual(mProj2b, 0.001f));
    EZ_TEST_BOOL(mProj2.IsEqual(mProj1b, 0.001f));
  }

  struct DepthRange
  {
    float fNear = 0.0f;
    float fFar = 0.0f;
  };

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExtractPerspectiveMatrixFieldOfView")
  {
    DepthRange depthRanges[] = {{1.0f, 1000.0f}, {1000.0f, 1.0f}, {0.5f, 20.0f}, {20.0f, 0.5f}};
    ezClipSpaceDepthRange::Enum clipRanges[] = {ezClipSpaceDepthRange::ZeroToOne, ezClipSpaceDepthRange::MinusOneToOne};
    ezHandedness::Enum handednesses[] = {ezHandedness::LeftHanded, ezHandedness::RightHanded};
    ezClipSpaceYMode::Enum clipSpaceYModes[] = {ezClipSpaceYMode::Regular, ezClipSpaceYMode::Flipped};

    for (auto clipSpaceYMode : clipSpaceYModes)
    {
      for (auto handedness : handednesses)
      {
        for (auto depthRange : depthRanges)
        {
          for (auto clipRange : clipRanges)
          {
            for (ezUInt32 angle = 10; angle < 180; angle += 10)
            {
              {
                ezMat4 mProj;
                mProj = ezGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(
                  ezAngle::Degree((float)angle), 2.0f, depthRange.fNear, depthRange.fFar, clipRange, clipSpaceYMode, handedness);

                ezAngle fovx, fovy;
                ezGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(mProj, fovx, fovy);

                EZ_TEST_FLOAT(fovx.GetDegree(), (float)angle, 0.5f);
              }

              {
                ezMat4 mProj;
                mProj = ezGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(
                  ezAngle::Degree((float)angle), 1.0f / 3.0f, depthRange.fNear, depthRange.fFar, clipRange, clipSpaceYMode, handedness);

                ezAngle fovx, fovy;
                ezGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(mProj, fovx, fovy);

                EZ_TEST_FLOAT(fovy.GetDegree(), (float)angle, 0.5f);
              }

              {
                const float fMinDepth = ezMath::Min(depthRange.fNear, depthRange.fFar);
                const ezAngle right = ezAngle::Degree((float)angle) / 2;
                const ezAngle top = ezAngle::Degree((float)angle) / 2;
                const float fLeft = ezMath::Tan(-right) * fMinDepth;
                const float fRight = ezMath::Tan(right) * fMinDepth * 0.8f;
                const float fBottom = ezMath::Tan(-top) * fMinDepth;
                const float fTop = ezMath::Tan(top) * fMinDepth * 0.7f;

                ezMat4 mProj;
                mProj = ezGraphicsUtils::CreatePerspectiveProjectionMatrix(fLeft, fRight, fBottom, fTop, depthRange.fNear, depthRange.fFar, clipRange, clipSpaceYMode, handedness);

                float fNearOut, fFarOut;
                EZ_TEST_BOOL(ezGraphicsUtils::ExtractNearAndFarClipPlaneDistances(fNearOut, fFarOut, mProj, clipRange).Succeeded());
                EZ_TEST_FLOAT(depthRange.fNear, fNearOut, 0.1f);
                EZ_TEST_FLOAT(depthRange.fFar, fFarOut, 0.1f);

                float fLeftOut, fRightOut, fBottomOut, fTopOut;
                EZ_TEST_BOOL(ezGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(mProj, fLeftOut, fRightOut, fBottomOut, fTopOut, clipRange, clipSpaceYMode).Succeeded());
                EZ_TEST_FLOAT(fLeft, fLeftOut, ezMath::LargeEpsilon<float>());
                EZ_TEST_FLOAT(fRight, fRightOut, ezMath::LargeEpsilon<float>());
                EZ_TEST_FLOAT(fBottom, fBottomOut, ezMath::LargeEpsilon<float>());
                EZ_TEST_FLOAT(fTop, fTopOut, ezMath::LargeEpsilon<float>());

                ezAngle fFovLeft;
                ezAngle fFovRight;
                ezAngle fFovBottom;
                ezAngle fFovTop;
                ezGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(mProj, fFovLeft, fFovRight, fFovBottom, fFovTop, clipSpaceYMode);

                EZ_TEST_FLOAT(fLeft, ezMath::Tan(fFovLeft) * fMinDepth, ezMath::LargeEpsilon<float>());
                EZ_TEST_FLOAT(fRight, ezMath::Tan(fFovRight) * fMinDepth, ezMath::LargeEpsilon<float>());
                EZ_TEST_FLOAT(fBottom, ezMath::Tan(fFovBottom) * fMinDepth, ezMath::LargeEpsilon<float>());
                EZ_TEST_FLOAT(fTop, ezMath::Tan(fFovTop) * fMinDepth, ezMath::LargeEpsilon<float>());
              }
            }
          }
        }
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExtractNearAndFarClipPlaneDistances")
  {
    DepthRange depthRanges[] = {{0.001f, 100.0f}, {0.01f, 10.0f}, {10.0f, 0.01f}, {1.01f, 110.0f}, {110.0f, 1.01f}};
    ezClipSpaceDepthRange::Enum clipRanges[] = {ezClipSpaceDepthRange::ZeroToOne, ezClipSpaceDepthRange::MinusOneToOne};
    ezHandedness::Enum handednesses[] = {ezHandedness::LeftHanded, ezHandedness::RightHanded};
    ezClipSpaceYMode::Enum clipSpaceYModes[] = {ezClipSpaceYMode::Regular, ezClipSpaceYMode::Flipped};
    ezAngle fovs[] = {ezAngle::Degree(10.0f), ezAngle::Degree(70.0f)};

    for (auto clipSpaceYMode : clipSpaceYModes)
    {
      for (auto handedness : handednesses)
      {
        for (auto depthRange : depthRanges)
        {
          for (auto clipRange : clipRanges)
          {
            for (auto fov : fovs)
            {
              ezMat4 mProj;
              mProj = ezGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(
                fov, 0.7f, depthRange.fNear, depthRange.fFar, clipRange, clipSpaceYMode, handedness);

              float fNearOut, fFarOut;
              EZ_TEST_BOOL(ezGraphicsUtils::ExtractNearAndFarClipPlaneDistances(fNearOut, fFarOut, mProj, clipRange).Succeeded());

              EZ_TEST_FLOAT(depthRange.fNear, fNearOut, 0.1f);
              EZ_TEST_FLOAT(depthRange.fFar, fFarOut, 0.2f);
            }
          }
        }
      }
    }

    { // Test failure on broken projection matrix
      // This matrix has a 0 in the w-component of the third column (invalid perspective divide)
      float vals[] = {0.770734549f, 0.000000000f, 0.000000000f, 0.000000000f, 0.000000000f, 1.73205078f, 0.000000000f, 0.000000000f, 0.000000000f, 0.000000000f, -1.00000000f, 0.00000000f, 0.000000000, 0.000000000f, -0.100000001f, 0.000000000f};
      ezMat4 mProj;
      memcpy(mProj.m_fElementsCM, vals, 16 * sizeof(float));
      float fNearOut = 0.f, fFarOut = 0.f;
      EZ_TEST_BOOL(ezGraphicsUtils::ExtractNearAndFarClipPlaneDistances(fNearOut, fFarOut, mProj, ezClipSpaceDepthRange::MinusOneToOne).Failed());
      EZ_TEST_BOOL(fNearOut == 0.0f);
      EZ_TEST_BOOL(fFarOut == 0.0f);
    }

    { // Test failure on broken projection matrix
      // This matrix has a 0 in the z-component of the fourth column (one or both projection planes are zero)
      float vals[] = {0.770734549f, 0.000000000f, 0.000000000f, 0.000000000f, 0.000000000f, 1.73205078f, 0.000000000f, 0.000000000f, 0.000000000f, 0.000000000f, -1.00000000f, -1.00000000f, 0.000000000f, 0.000000000f, 0.000000000f, 0.000000000f};
      ezMat4 mProj;
      memcpy(mProj.m_fElementsCM, vals, 16 * sizeof(float));
      float fNearOut = 0.f, fFarOut = 0.f;
      EZ_TEST_BOOL(ezGraphicsUtils::ExtractNearAndFarClipPlaneDistances(fNearOut, fFarOut, mProj, ezClipSpaceDepthRange::MinusOneToOne).Failed());
      EZ_TEST_BOOL(fNearOut == 0.0f);
      EZ_TEST_BOOL(fFarOut == 0.0f);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ComputeInterpolatedFrustumPlane")
  {
    for (ezUInt32 i = 0; i <= 10; ++i)
    {
      float nearPlane = 1.0f;
      float farPlane = 1000.0f;

      ezMat4 mProj;
      mProj = ezGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(
        ezAngle::Degree(90.0f), 1.0f, nearPlane, farPlane, ezClipSpaceDepthRange::ZeroToOne, ezClipSpaceYMode::Regular, ezHandedness::LeftHanded);

      const ezPlane horz = ezGraphicsUtils::ComputeInterpolatedFrustumPlane(
        ezGraphicsUtils::FrustumPlaneInterpolation::LeftToRight, i * 0.1f, mProj, ezClipSpaceDepthRange::ZeroToOne);
      const ezPlane vert = ezGraphicsUtils::ComputeInterpolatedFrustumPlane(
        ezGraphicsUtils::FrustumPlaneInterpolation::BottomToTop, i * 0.1f, mProj, ezClipSpaceDepthRange::ZeroToOne);
      const ezPlane forw = ezGraphicsUtils::ComputeInterpolatedFrustumPlane(
        ezGraphicsUtils::FrustumPlaneInterpolation::NearToFar, i * 0.1f, mProj, ezClipSpaceDepthRange::ZeroToOne);

      // Generate clip space point at intersection of the 3 planes and project to worldspace
      ezVec4 clipSpacePoint = ezVec4(0.1f * i * 2 - 1, 0.1f * i * 2 - 1, 0.1f * i, 1);

      ezVec4 worldSpacePoint = mProj.GetInverse() * clipSpacePoint;
      worldSpacePoint /= worldSpacePoint.w;

      EZ_TEST_FLOAT(horz.GetDistanceTo(ezVec3::ZeroVector()), 0.0f, 0.01f);
      EZ_TEST_FLOAT(vert.GetDistanceTo(ezVec3::ZeroVector()), 0.0f, 0.01f);

      if (i == 0)
      {
        EZ_TEST_FLOAT(forw.GetDistanceTo(ezVec3::ZeroVector()), -nearPlane, 0.01f);
      }
      else if (i == 10)
      {
        EZ_TEST_FLOAT(forw.GetDistanceTo(ezVec3::ZeroVector()), -farPlane, 0.01f);
      }

      EZ_TEST_FLOAT(horz.GetDistanceTo(worldSpacePoint.GetAsVec3()), 0.0f, 0.02f);
      EZ_TEST_FLOAT(vert.GetDistanceTo(worldSpacePoint.GetAsVec3()), 0.0f, 0.02f);
      EZ_TEST_FLOAT(forw.GetDistanceTo(worldSpacePoint.GetAsVec3()), 0.0f, 0.02f);

      // this isn't interpolated linearly across the angle (rotated), so the epsilon has to be very large (just an approx test)
      EZ_TEST_FLOAT(horz.m_vNormal.GetAngleBetween(ezVec3(1, 0, 0)).GetDegree(), ezMath::Abs(-45.0f + 90.0f * i * 0.1f), 4.0f);
      EZ_TEST_FLOAT(vert.m_vNormal.GetAngleBetween(ezVec3(0, 1, 0)).GetDegree(), ezMath::Abs(-45.0f + 90.0f * i * 0.1f), 4.0f);
      EZ_TEST_VEC3(forw.m_vNormal, ezVec3(0, 0, 1), 0.01f);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CreateLookAtViewMatrix / CreateInverseLookAtViewMatrix")
  {
    for (int h = 0; h < 2; ++h)
    {
      const ezHandedness::Enum handedness = (h == 0) ? ezHandedness::LeftHanded : ezHandedness::RightHanded;

      {
        ezMat3 mLook3 = ezGraphicsUtils::CreateLookAtViewMatrix(ezVec3(1, 0, 0), ezVec3(0, 0, 1), handedness);
        ezMat3 mLookInv3 = ezGraphicsUtils::CreateInverseLookAtViewMatrix(ezVec3(1, 0, 0), ezVec3(0, 0, 1), handedness);

        EZ_TEST_BOOL((mLook3 * mLookInv3).IsIdentity(0.01f));

        ezMat4 mLook4 = ezGraphicsUtils::CreateLookAtViewMatrix(ezVec3(0), ezVec3(1, 0, 0), ezVec3(0, 0, 1), handedness);
        ezMat4 mLookInv4 = ezGraphicsUtils::CreateInverseLookAtViewMatrix(ezVec3(0), ezVec3(1, 0, 0), ezVec3(0, 0, 1), handedness);

        EZ_TEST_BOOL((mLook4 * mLookInv4).IsIdentity(0.01f));

        EZ_TEST_BOOL(mLook3.IsEqual(mLook4.GetRotationalPart(), 0.01f));
        EZ_TEST_BOOL(mLookInv3.IsEqual(mLookInv4.GetRotationalPart(), 0.01f));
      }

      {
        ezMat4 mLook4 = ezGraphicsUtils::CreateLookAtViewMatrix(ezVec3(1, 2, 0), ezVec3(4, 5, 0), ezVec3(0, 0, 1), handedness);
        ezMat4 mLookInv4 = ezGraphicsUtils::CreateInverseLookAtViewMatrix(ezVec3(1, 2, 0), ezVec3(4, 5, 0), ezVec3(0, 0, 1), handedness);

        EZ_TEST_BOOL((mLook4 * mLookInv4).IsIdentity(0.01f));
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CreateViewMatrix / DecomposeViewMatrix / CreateInverseViewMatrix")
  {
    for (int h = 0; h < 2; ++h)
    {
      const ezHandedness::Enum handedness = (h == 0) ? ezHandedness::LeftHanded : ezHandedness::RightHanded;

      const ezVec3 vEye(0);
      const ezVec3 vTarget(0, 0, 1);
      const ezVec3 vUp0(0, 1, 0);
      const ezVec3 vFwd = (vTarget - vEye).GetNormalized();
      ezVec3 vRight = vUp0.CrossRH(vFwd).GetNormalized();
      const ezVec3 vUp = vFwd.CrossRH(vRight).GetNormalized();

      if (handedness == ezHandedness::RightHanded)
        vRight = -vRight;

      const ezMat4 mLookAt = ezGraphicsUtils::CreateLookAtViewMatrix(vEye, vTarget, vUp0, handedness);

      ezVec3 decFwd, decRight, decUp, decPos;
      ezGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, mLookAt, handedness);

      EZ_TEST_VEC3(decPos, vEye, 0.01f);
      EZ_TEST_VEC3(decFwd, vFwd, 0.01f);
      EZ_TEST_VEC3(decUp, vUp, 0.01f);
      EZ_TEST_VEC3(decRight, vRight, 0.01f);

      const ezMat4 mView = ezGraphicsUtils::CreateViewMatrix(decPos, decFwd, decRight, decUp, handedness);
      const ezMat4 mViewInv = ezGraphicsUtils::CreateInverseViewMatrix(decPos, decFwd, decRight, decUp, handedness);

      EZ_TEST_BOOL(mLookAt.IsEqual(mView, 0.01f));

      EZ_TEST_BOOL((mLookAt * mViewInv).IsIdentity());
    }
  }
}
