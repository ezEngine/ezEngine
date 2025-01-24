#include <GameEngineTest/GameEngineTestPCH.h>

#include "RmlUiTest.h"
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>

static ezGameEngineTestRmlUi s_GameEngineTestAnimations;

const char* ezGameEngineTestRmlUi::GetTestName() const
{
  return "RmlUi Tests";
}

ezGameEngineTestApplication* ezGameEngineTestRmlUi::CreateApplication()
{
  m_pOwnApplication = EZ_DEFAULT_NEW(ezGameEngineTestApplication, "RmlUi");
  return m_pOwnApplication;
}

void ezGameEngineTestRmlUi::SetupSubTests()
{
  AddSubTest("Demo", SubTests::Demo);
}

ezResult ezGameEngineTestRmlUi::InitializeSubTest(ezInt32 iIdentifier)
{
  EZ_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame = -1;
  m_uiImgCompIdx = 0;
  m_ImgCompFrames.Clear();

  if (iIdentifier == SubTests::Demo)
  {
    m_ImgCompFrames.PushBack(2);
    m_ImgCompFrames.PushBack(4);
    m_ImgCompFrames.PushBack(7);

    EZ_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("RmlUi/AssetCache/Common/Scenes/Demo.ezBinScene"));
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezTestAppRun ezGameEngineTestRmlUi::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  const bool bVulkan = ezGameApplication::GetActiveRenderer().IsEqual_NoCase("Vulkan");
  ++m_iFrame;

  m_pOwnApplication->Run();
  if (m_pOwnApplication->ShouldApplicationQuit())
  {
    return ezTestAppRun::Quit;
  }

  if (m_ImgCompFrames[m_uiImgCompIdx] == m_iFrame)
  {
    EZ_TEST_IMAGE(m_uiImgCompIdx, bVulkan ? 300 : 250);
    ++m_uiImgCompIdx;

    if (m_uiImgCompIdx >= m_ImgCompFrames.GetCount())
    {
      return ezTestAppRun::Quit;
    }
  }

  return ezTestAppRun::Continue;
}
