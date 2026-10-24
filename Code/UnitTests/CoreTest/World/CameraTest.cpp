#include <CoreTest/CoreTestPCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Utilities/GraphicsUtils.h>

EZ_CREATE_SIMPLE_TEST(World, Camera)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "LookAt")
  {
    ezCamera camera;

    camera.LookAt(ezVec3(0, 0, 0), ezVec3(1, 0, 0), ezVec3(0, 0, 1));
    EZ_TEST_VEC3(camera.GetPosition(), ezVec3(0, 0, 0), ezMath::DefaultEpsilon<float>());
    EZ_TEST_VEC3(camera.GetDirForwards(), ezVec3(1, 0, 0), ezMath::DefaultEpsilon<float>());
    EZ_TEST_VEC3(camera.GetDirRight(), ezVec3(0, 1, 0), ezMath::DefaultEpsilon<float>());
    EZ_TEST_VEC3(camera.GetDirUp(), ezVec3(0, 0, 1), ezMath::DefaultEpsilon<float>());

    camera.LookAt(ezVec3(0, 0, 0), ezVec3(-1, 0, 0), ezVec3(0, 0, 1));
    EZ_TEST_VEC3(camera.GetPosition(), ezVec3(0, 0, 0), ezMath::DefaultEpsilon<float>());
    EZ_TEST_VEC3(camera.GetDirForwards(), ezVec3(-1, 0, 0), ezMath::DefaultEpsilon<float>());
    EZ_TEST_VEC3(camera.GetDirRight(), ezVec3(0, -1, 0), ezMath::DefaultEpsilon<float>());
    EZ_TEST_VEC3(camera.GetDirUp(), ezVec3(0, 0, 1), ezMath::DefaultEpsilon<float>());

    camera.LookAt(ezVec3(0, 0, 0), ezVec3(0, 0, 1), ezVec3(0, 1, 0));
    EZ_TEST_VEC3(camera.GetPosition(), ezVec3(0, 0, 0), ezMath::DefaultEpsilon<float>());
    EZ_TEST_VEC3(camera.GetDirForwards(), ezVec3(0, 0, 1), ezMath::DefaultEpsilon<float>());
    EZ_TEST_VEC3(camera.GetDirRight(), ezVec3(1, 0, 0), ezMath::DefaultEpsilon<float>());
    EZ_TEST_VEC3(camera.GetDirUp(), ezVec3(0, 1, 0), ezMath::DefaultEpsilon<float>());

    camera.LookAt(ezVec3(0, 0, 0), ezVec3(0, 0, -1), ezVec3(0, 1, 0));
    EZ_TEST_VEC3(camera.GetPosition(), ezVec3(0, 0, 0), ezMath::DefaultEpsilon<float>());
    EZ_TEST_VEC3(camera.GetDirForwards(), ezVec3(0, 0, -1), ezMath::DefaultEpsilon<float>());
    EZ_TEST_VEC3(camera.GetDirRight(), ezVec3(-1, 0, 0), ezMath::DefaultEpsilon<float>());
    EZ_TEST_VEC3(camera.GetDirUp(), ezVec3(0, 1, 0), ezMath::DefaultEpsilon<float>());

    const ezMat4 mLookAt = ezGraphicsUtils::CreateLookAtViewMatrix(ezVec3(2, 3, 4), ezVec3(3, 3, 4), ezVec3(0, 0, 1), ezHandedness::LeftHanded);
    camera.SetViewMatrix(mLookAt);

    EZ_TEST_VEC3(camera.GetPosition(), ezVec3(2, 3, 4), ezMath::DefaultEpsilon<float>());
    EZ_TEST_VEC3(camera.GetDirForwards(), ezVec3(1, 0, 0), ezMath::DefaultEpsilon<float>());
    EZ_TEST_VEC3(camera.GetDirRight(), ezVec3(0, 1, 0), ezMath::DefaultEpsilon<float>());
    EZ_TEST_VEC3(camera.GetDirUp(), ezVec3(0, 0, 1), ezMath::DefaultEpsilon<float>());

    // look at with dir == up vector
    camera.LookAt(ezVec3(2, 3, 4), ezVec3(2, 3, 5), ezVec3(0, 0, 1));
    EZ_TEST_VEC3(camera.GetPosition(), ezVec3(2, 3, 4), ezMath::DefaultEpsilon<float>());
    EZ_TEST_VEC3(camera.GetDirForwards(), ezVec3(0, 0, 1), ezMath::DefaultEpsilon<float>());
    EZ_TEST_VEC3(camera.GetDirRight(), ezVec3(0, 1, 0), ezMath::DefaultEpsilon<float>());
    EZ_TEST_VEC3(camera.GetDirUp(), ezVec3(-1, 0, 0), ezMath::DefaultEpsilon<float>());

    camera.LookAt(ezVec3(2, 3, 4), ezVec3(2, 3, 3), ezVec3(0, 0, 1));
    EZ_TEST_VEC3(camera.GetPosition(), ezVec3(2, 3, 4), ezMath::DefaultEpsilon<float>());
    EZ_TEST_VEC3(camera.GetDirForwards(), ezVec3(0, 0, -1), ezMath::DefaultEpsilon<float>());
    EZ_TEST_VEC3(camera.GetDirRight(), ezVec3(0, 1, 0), ezMath::DefaultEpsilon<float>());
    EZ_TEST_VEC3(camera.GetDirUp(), ezVec3(1, 0, 0), ezMath::DefaultEpsilon<float>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Inverse Depth Projection")
  {
    const ezCameraMode::Enum modes[] = {ezCameraMode::PerspectiveFixedFovX, ezCameraMode::PerspectiveFixedFovY, ezCameraMode::OrthoFixedWidth, ezCameraMode::OrthoFixedHeight};
    const ezClipSpaceDepthRange::Enum depthRanges[] = {ezClipSpaceDepthRange::ZeroToOne, ezClipSpaceDepthRange::MinusOneToOne};

    for (auto mode : modes)
    {
      ezCamera camera;
      camera.SetCameraMode(mode, 60.0f, 0.5f, 100.0f);

      EZ_TEST_FLOAT(camera.GetNearPlane(), 0.5f, 0.0f);
      EZ_TEST_FLOAT(camera.GetFarPlane(), 100.0f, 0.0f);

      for (auto depthRange : depthRanges)
      {
        ezMat4 projection;
        camera.GetProjectionMatrix(1.5f, projection, ezCameraEye::Left, depthRange);

        ezVec3 screenNear;
        ezVec3 screenFar;
        EZ_TEST_BOOL(ezGraphicsUtils::ConvertWorldPosToScreenPos(projection, ezVec3(0, 0, 0.5f), screenNear, depthRange).Succeeded());
        EZ_TEST_BOOL(ezGraphicsUtils::ConvertWorldPosToScreenPos(projection, ezVec3(0, 0, 100.0f), screenFar, depthRange).Succeeded());
        EZ_TEST_FLOAT(screenNear.z, 1.0f, ezMath::DefaultEpsilon<float>());
        EZ_TEST_FLOAT(screenFar.z, 0.0f, ezMath::DefaultEpsilon<float>());
      }
    }

    ezCamera stereoCamera;
    stereoCamera.SetCameraMode(ezCameraMode::Stereo, 60.0f, 0.5f, 100.0f);
    const ezMat4 leftProjection = ezGraphicsUtils::CreatePerspectiveProjectionMatrix(
      -0.4f, 0.6f, -0.3f, 0.5f, 100.0f, 0.5f, ezClipSpaceDepthRange::Default, ezClipSpaceYMode::Regular, ezHandedness::LeftHanded);
    const ezMat4 rightProjection = ezGraphicsUtils::CreatePerspectiveProjectionMatrix(
      -0.6f, 0.4f, -0.3f, 0.5f, 100.0f, 0.5f, ezClipSpaceDepthRange::Default, ezClipSpaceYMode::Regular, ezHandedness::LeftHanded);
    stereoCamera.SetStereoProjection(leftProjection, rightProjection, 1.5f);

    for (auto depthRange : depthRanges)
    {
      for (auto eye : {ezCameraEye::Left, ezCameraEye::Right})
      {
        for (float fAspect : {1.5f, 2.0f})
        {
          ezMat4 projection;
          stereoCamera.GetProjectionMatrix(fAspect, projection, eye, depthRange);

          ezMat4 expected = eye == ezCameraEye::Left ? leftProjection : rightProjection;
          if (fAspect == 1.5f)
            ezGraphicsUtils::ConvertProjectionMatrixDepthRange(expected, ezClipSpaceDepthRange::Default, depthRange);
          else
            expected = ezGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(
              ezAngle::MakeFromDegree(60), fAspect, 100.0f, 0.5f, depthRange, ezClipSpaceYMode::Regular, ezHandedness::LeftHanded);
          EZ_TEST_BOOL(projection.IsEqual(expected, ezMath::DefaultEpsilon<float>()));

          ezVec3 screenNear;
          ezVec3 screenFar;
          EZ_TEST_BOOL(ezGraphicsUtils::ConvertWorldPosToScreenPos(projection, ezVec3(0, 0, 0.5f), screenNear, depthRange).Succeeded());
          EZ_TEST_BOOL(ezGraphicsUtils::ConvertWorldPosToScreenPos(projection, ezVec3(0, 0, 100.0f), screenFar, depthRange).Succeeded());
          EZ_TEST_FLOAT(screenNear.z, 1.0f, ezMath::DefaultEpsilon<float>());
          EZ_TEST_FLOAT(screenFar.z, 0.0f, ezMath::DefaultEpsilon<float>());
        }
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Inverse Depth with Infinite Far Plane")
  {
    for (auto mode : {ezCameraMode::PerspectiveFixedFovX, ezCameraMode::PerspectiveFixedFovY})
    {
      ezCamera camera;
      camera.SetCameraMode(mode, 60.0f, 0.5f, ezMath::Infinity<float>());
      EZ_TEST_BOOL(camera.GetFarPlane() == ezMath::Infinity<float>());

      for (auto depthRange : {ezClipSpaceDepthRange::ZeroToOne, ezClipSpaceDepthRange::MinusOneToOne})
      {
        ezMat4 projection;
        camera.GetProjectionMatrix(1.5f, projection, ezCameraEye::Left, depthRange);
        EZ_TEST_BOOL(projection.IsValid());

        ezVec3 screen;
        EZ_TEST_BOOL(ezGraphicsUtils::ConvertWorldPosToScreenPos(projection, ezVec3(0, 0, 0.5f), screen, depthRange).Succeeded());
        EZ_TEST_FLOAT(screen.z, 1.0f, ezMath::DefaultEpsilon<float>());
        EZ_TEST_BOOL(ezGraphicsUtils::ConvertWorldPosToScreenPos(projection, ezVec3(0, 0, 5000.0f), screen, depthRange).Succeeded());
        EZ_TEST_FLOAT(screen.z, 0.0001f, ezMath::DefaultEpsilon<float>());

        ezVec3 point, direction;
        const ezMat4 inverse = projection.GetInverse();
        EZ_TEST_BOOL(ezGraphicsUtils::ConvertScreenPosToWorldPos(inverse, ezVec3(0.5f, 0.5f, 0.001f), point, &direction, depthRange).Succeeded());
        EZ_TEST_VEC3(direction, ezVec3::MakeAxisZ(), 0.001f);
        EZ_TEST_BOOL(ezGraphicsUtils::ConvertScreenPosToWorldPos(inverse, ezVec3(0.5f, 0.5f, 0), point, nullptr, depthRange).Failed());
      }
    }
  }
}
