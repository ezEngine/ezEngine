#include <CoreTestPCH.h>

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

    const ezMat4 mLookAt =
      ezGraphicsUtils::CreateLookAtViewMatrix(ezVec3(2, 3, 4), ezVec3(3, 3, 4), ezVec3(0, 0, 1), ezHandedness::LeftHanded);
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
}
