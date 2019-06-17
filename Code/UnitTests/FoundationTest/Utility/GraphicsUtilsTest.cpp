#include <FoundationTestPCH.h>

#include <Foundation/Utilities/GraphicsUtils.h>

EZ_CREATE_SIMPLE_TEST(Utility, GraphicsUtils)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Perspective (-1/1): ConvertWorldPosToScreenPos / ConvertScreenPosToWorldPos")
  {
    ezMat4 mProj, mProjInv;
    mProj.SetPerspectiveProjectionMatrixFromFovX(ezAngle::Degree(85.0f), 2.0f, 1.0f, 1000.0f, ezProjectionDepthRange::MinusOneToOne);
    mProjInv = mProj.GetInverse();

    for (ezUInt32 y = 0; y < 25; ++y)
    {
      for (ezUInt32 x = 0; x < 50; ++x)
      {
        ezVec3 vPoint, vDir;
        EZ_TEST_BOOL(ezGraphicsUtils::ConvertScreenPosToWorldPos(
          mProjInv, 0, 0, 50, 25, ezVec3((float)x, (float)y, 0.5f), vPoint, &vDir, ezProjectionDepthRange::MinusOneToOne)
                       .Succeeded());

        EZ_TEST_VEC3(vDir, vPoint.GetNormalized(), 0.01f);

        ezVec3 vScreen;
        EZ_TEST_BOOL(
          ezGraphicsUtils::ConvertWorldPosToScreenPos(mProj, 0, 0, 50, 25, vPoint, vScreen, ezProjectionDepthRange::MinusOneToOne)
            .Succeeded());

        EZ_TEST_VEC3(vScreen, ezVec3((float)x, (float)y, 0.5f), 0.01f);
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Perspective (0/1): ConvertWorldPosToScreenPos / ConvertScreenPosToWorldPos")
  {
    ezMat4 mProj, mProjInv;
    mProj.SetPerspectiveProjectionMatrixFromFovX(ezAngle::Degree(85.0f), 2.0f, 1.0f, 1000.0f, ezProjectionDepthRange::ZeroToOne);
    mProjInv = mProj.GetInverse();

    for (ezUInt32 y = 0; y < 25; ++y)
    {
      for (ezUInt32 x = 0; x < 50; ++x)
      {
        ezVec3 vPoint, vDir;
        EZ_TEST_BOOL(ezGraphicsUtils::ConvertScreenPosToWorldPos(
          mProjInv, 0, 0, 50, 25, ezVec3((float)x, (float)y, 0.5f), vPoint, &vDir, ezProjectionDepthRange::ZeroToOne)
                       .Succeeded());

        EZ_TEST_VEC3(vDir, vPoint.GetNormalized(), 0.01f);

        ezVec3 vScreen;
        EZ_TEST_BOOL(
          ezGraphicsUtils::ConvertWorldPosToScreenPos(mProj, 0, 0, 50, 25, vPoint, vScreen, ezProjectionDepthRange::ZeroToOne).Succeeded());

        EZ_TEST_VEC3(vScreen, ezVec3((float)x, (float)y, 0.5f), 0.01f);
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Ortho (-1/1): ConvertWorldPosToScreenPos / ConvertScreenPosToWorldPos")
  {
    ezMat4 mProj, mProjInv;
    mProj.SetOrthographicProjectionMatrix(50, 25, 1.0f, 1000.0f, ezProjectionDepthRange::MinusOneToOne);
    mProjInv = mProj.GetInverse();

    for (ezUInt32 y = 0; y < 25; ++y)
    {
      for (ezUInt32 x = 0; x < 50; ++x)
      {
        ezVec3 vPoint, vDir;
        EZ_TEST_BOOL(ezGraphicsUtils::ConvertScreenPosToWorldPos(
          mProjInv, 0, 0, 50, 25, ezVec3((float)x, (float)y, 0.5f), vPoint, &vDir, ezProjectionDepthRange::MinusOneToOne)
                       .Succeeded());

        EZ_TEST_VEC3(vDir, ezVec3(0, 0, 1.0f), 0.01f);

        ezVec3 vScreen;
        EZ_TEST_BOOL(
          ezGraphicsUtils::ConvertWorldPosToScreenPos(mProj, 0, 0, 50, 25, vPoint, vScreen, ezProjectionDepthRange::MinusOneToOne)
            .Succeeded());

        EZ_TEST_VEC3(vScreen, ezVec3((float)x, (float)y, 0.5f), 0.01f);
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Ortho (0/1): ConvertWorldPosToScreenPos / ConvertScreenPosToWorldPos")
  {
    ezMat4 mProj, mProjInv;
    mProj.SetOrthographicProjectionMatrix(50, 25, 1.0f, 1000.0f, ezProjectionDepthRange::ZeroToOne);
    mProjInv = mProj.GetInverse();

    for (ezUInt32 y = 0; y < 25; ++y)
    {
      for (ezUInt32 x = 0; x < 50; ++x)
      {
        ezVec3 vPoint, vDir;
        EZ_TEST_BOOL(ezGraphicsUtils::ConvertScreenPosToWorldPos(
          mProjInv, 0, 0, 50, 25, ezVec3((float)x, (float)y, 0.5f), vPoint, &vDir, ezProjectionDepthRange::ZeroToOne)
                       .Succeeded());

        EZ_TEST_VEC3(vDir, ezVec3(0, 0, 1.0f), 0.01f);

        ezVec3 vScreen;
        EZ_TEST_BOOL(
          ezGraphicsUtils::ConvertWorldPosToScreenPos(mProj, 0, 0, 50, 25, vPoint, vScreen, ezProjectionDepthRange::ZeroToOne).Succeeded());

        EZ_TEST_VEC3(vScreen, ezVec3((float)x, (float)y, 0.5f), 0.01f);
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ConvertProjectionMatrixDepthRange")
  {
    ezMat4 mProj1, mProj2;
    mProj1.SetPerspectiveProjectionMatrixFromFovX(ezAngle::Degree(85.0f), 2.0f, 1.0f, 1000.0f, ezProjectionDepthRange::ZeroToOne);
    mProj2.SetPerspectiveProjectionMatrixFromFovX(ezAngle::Degree(85.0f), 2.0f, 1.0f, 1000.0f, ezProjectionDepthRange::MinusOneToOne);

    ezMat4 mProj1b = mProj1;
    ezMat4 mProj2b = mProj2;
    ezGraphicsUtils::ConvertProjectionMatrixDepthRange(mProj1b, ezProjectionDepthRange::ZeroToOne, ezProjectionDepthRange::MinusOneToOne);
    ezGraphicsUtils::ConvertProjectionMatrixDepthRange(mProj2b, ezProjectionDepthRange::MinusOneToOne, ezProjectionDepthRange::ZeroToOne);

    EZ_TEST_BOOL(mProj1.IsEqual(mProj2b, 0.001f));
    EZ_TEST_BOOL(mProj2.IsEqual(mProj1b, 0.001f));
  }
}
