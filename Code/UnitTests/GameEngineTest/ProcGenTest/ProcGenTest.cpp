#include <GameEngineTest/GameEngineTestPCH.h>

#include "ProcGenTest.h"
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>

static ezGameEngineTestProcGen s_GameEngineTestAnimations;

const char* ezGameEngineTestProcGen::GetTestName() const
{
  return "ProcGen Tests";
}

ezGameEngineTestApplication* ezGameEngineTestProcGen::CreateApplication()
{
  m_pOwnApplication = EZ_DEFAULT_NEW(ezGameEngineTestApplication, "ProcGen");
  return m_pOwnApplication;
}

void ezGameEngineTestProcGen::SetupSubTests()
{
  AddSubTest("VertexColors", SubTests::VertexColors);
}

ezResult ezGameEngineTestProcGen::InitializeSubTest(ezInt32 iIdentifier)
{
  EZ_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame = -1;
  m_uiImgCompIdx = 0;
  m_ImgCompFrames.Clear();

  if (iIdentifier == SubTests::VertexColors)
  {
    m_ImgCompFrames.PushBack(1);

    EZ_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("ProcGen/AssetCache/Common/Scenes/VertexColors.ezBinScene"));
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezTestAppRun ezGameEngineTestProcGen::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
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
