#include <GameEngineTest/GameEngineTestPCH.h>

#include "AnimationsTest.h"
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>

static ezGameEngineTestAnimations s_GameEngineTestAnimations;

const char* ezGameEngineTestAnimations::GetTestName() const
{
  return "Animations Tests";
}

ezGameEngineTestApplication* ezGameEngineTestAnimations::CreateApplication()
{
  m_pOwnApplication = EZ_DEFAULT_NEW(ezGameEngineTestApplication, "Animations");
  return m_pOwnApplication;
}

void ezGameEngineTestAnimations::SetupSubTests()
{
  AddSubTest("Skeletal", SubTests::Skeletal);
}

ezResult ezGameEngineTestAnimations::InitializeSubTest(ezInt32 iIdentifier)
{
  EZ_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame = -1;
  m_uiImgCompIdx = 0;
  m_ImgCompFrames.Clear();

  if (iIdentifier == SubTests::Skeletal)
  {
    m_ImgCompFrames.PushBack(1);
    m_ImgCompFrames.PushBack(30);
    m_ImgCompFrames.PushBack(60);

    EZ_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("Animations/AssetCache/Common/Scenes/AnimController.ezBinScene"));
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezTestAppRun ezGameEngineTestAnimations::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  const bool bVulkan = ezGameApplication::GetActiveRenderer().IsEqual_NoCase("Vulkan");
  ++m_iFrame;

  if (m_pOwnApplication->Run() == ezApplication::Execution::Quit)
    return ezTestAppRun::Quit;

  if (m_ImgCompFrames[m_uiImgCompIdx] == m_iFrame)
  {
    EZ_TEST_IMAGE(m_uiImgCompIdx, 300);
    ++m_uiImgCompIdx;

    if (m_uiImgCompIdx >= m_ImgCompFrames.GetCount())
    {
      return ezTestAppRun::Quit;
    }
  }

  return ezTestAppRun::Continue;
}
