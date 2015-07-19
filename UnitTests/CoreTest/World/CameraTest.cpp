#include <PCH.h>
#include <CoreUtils/Graphics/Camera.h>

EZ_CREATE_SIMPLE_TEST(World, Camera)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "LookAt")
  {
    ezCamera camera;

    camera.LookAt(ezVec3(2, 3, 4), ezVec3(3, 3, 4), ezVec3(0, 0, 1));
    EZ_TEST_VEC3(camera.GetPosition(), ezVec3(2, 3, 4), ezMath::BasicType<float>::DefaultEpsilon());
    EZ_TEST_VEC3(camera.GetDirForwards(), ezVec3(1, 0, 0), ezMath::BasicType<float>::DefaultEpsilon());
    EZ_TEST_VEC3(camera.GetDirRight(), ezVec3(0, 1, 0), ezMath::BasicType<float>::DefaultEpsilon());
    EZ_TEST_VEC3(camera.GetDirUp(), ezVec3(0, 0, 1), ezMath::BasicType<float>::DefaultEpsilon());

    ezMat4 mLookAt; mLookAt.SetLookAtMatrix(ezVec3(2, 3, 4), ezVec3(3, 3, 4), ezVec3(0, 0, 1));
    camera.SetFromMatrix(mLookAt);

    EZ_TEST_VEC3(camera.GetPosition(), ezVec3(2, 3, 4), ezMath::BasicType<float>::DefaultEpsilon());
    EZ_TEST_VEC3(camera.GetDirForwards(), ezVec3(1, 0, 0), ezMath::BasicType<float>::DefaultEpsilon());
    EZ_TEST_VEC3(camera.GetDirRight(), ezVec3(0, 1, 0), ezMath::BasicType<float>::DefaultEpsilon());
    EZ_TEST_VEC3(camera.GetDirUp(), ezVec3(0, 0, 1), ezMath::BasicType<float>::DefaultEpsilon());

    // look at with dir == up vector
    camera.LookAt(ezVec3(2, 3, 4), ezVec3(2, 3, 5), ezVec3(0, 0, 1));
    EZ_TEST_VEC3(camera.GetPosition(), ezVec3(2, 3, 4), ezMath::BasicType<float>::DefaultEpsilon());
    EZ_TEST_VEC3(camera.GetDirForwards(), ezVec3(0, 0, 1), ezMath::BasicType<float>::DefaultEpsilon());
    EZ_TEST_VEC3(camera.GetDirRight(), ezVec3(0, 1, 0), ezMath::BasicType<float>::DefaultEpsilon());
    EZ_TEST_VEC3(camera.GetDirUp(), ezVec3(-1, 0, 0), ezMath::BasicType<float>::DefaultEpsilon());

    camera.LookAt(ezVec3(2, 3, 4), ezVec3(2, 3, 3), ezVec3(0, 0, 1));
    EZ_TEST_VEC3(camera.GetPosition(), ezVec3(2, 3, 4), ezMath::BasicType<float>::DefaultEpsilon());
    EZ_TEST_VEC3(camera.GetDirForwards(), ezVec3(0, 0, -1), ezMath::BasicType<float>::DefaultEpsilon());
    EZ_TEST_VEC3(camera.GetDirRight(), ezVec3(0, 1, 0), ezMath::BasicType<float>::DefaultEpsilon());
    EZ_TEST_VEC3(camera.GetDirUp(), ezVec3(1, 0, 0), ezMath::BasicType<float>::DefaultEpsilon());
  }
}
