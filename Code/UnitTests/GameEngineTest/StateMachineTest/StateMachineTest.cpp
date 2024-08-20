#include <GameEngineTest/GameEngineTestPCH.h>

#include "StateMachineTest.h"
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>

static ezGameEngineTestStateMachine s_GameEngineTestAnimations;

const char* ezGameEngineTestStateMachine::GetTestName() const
{
  return "StateMachine Tests";
}

ezGameEngineTestApplication* ezGameEngineTestStateMachine::CreateApplication()
{
  m_pOwnApplication = EZ_DEFAULT_NEW(ezGameEngineTestApplication, "StateMachine");
  return m_pOwnApplication;
}

void ezGameEngineTestStateMachine::SetupSubTests()
{
  AddSubTest("Builtins", SubTests::Builtins);
  AddSubTest("SimpleTransitions", SubTests::SimpleTransitions);
}

ezResult ezGameEngineTestStateMachine::InitializeSubTest(ezInt32 iIdentifier)
{
  EZ_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame = -1;
  m_uiImgCompIdx = 0;
  m_ImgCompFrames.Clear();

  if (iIdentifier == SubTests::Builtins)
  {
    return EZ_SUCCESS;
  }
  else if (iIdentifier == SubTests::SimpleTransitions)
  {
    m_ImgCompFrames.PushBack(1);
    m_ImgCompFrames.PushBack(17);
    m_ImgCompFrames.PushBack(34);
    m_ImgCompFrames.PushBack(51);

    EZ_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("StateMachine/AssetCache/Common/Scenes/StateMachine.ezBinScene"));
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezTestAppRun ezGameEngineTestStateMachine::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  if (iIdentifier == SubTests::Builtins)
  {
    RunBuiltinsTest();
    return ezTestAppRun::Quit;
  }

  const bool bVulkan = ezGameApplication::GetActiveRenderer().IsEqual_NoCase("Vulkan");
  ++m_iFrame;

  if (m_pOwnApplication->Run() == ezApplication::Execution::Quit)
    return ezTestAppRun::Quit;

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
