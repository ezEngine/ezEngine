#include <GameEngineTest/GameEngineTestPCH.h>

#include "VisualScriptTest.h"
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>

static ezGameEngineTestVisualScript s_GameEngineTestAnimations;

const char* ezGameEngineTestVisualScript::GetTestName() const
{
  return "VisualScript Tests";
}

ezGameEngineTestApplication* ezGameEngineTestVisualScript::CreateApplication()
{
  m_pOwnApplication = EZ_DEFAULT_NEW(ezGameEngineTestApplication, "VisualScript");
  return m_pOwnApplication;
}

void ezGameEngineTestVisualScript::SetupSubTests()
{
  AddSubTest("Variables", SubTests::Variables);
  AddSubTest("Coroutines", SubTests::Coroutines);
  AddSubTest("Messages", SubTests::Messages);
  AddSubTest("EnumsAndSwitch", SubTests::EnumsAndSwitch);
  AddSubTest("Blackboard", SubTests::Blackboard);
  AddSubTest("Loops", SubTests::Loops);
  AddSubTest("Loops2", SubTests::Loops2);
  AddSubTest("Loops3", SubTests::Loops3);
  AddSubTest("Properties", SubTests::Properties);
  AddSubTest("Arrays", SubTests::Arrays);
  AddSubTest("Expressions", SubTests::Expressions);
  AddSubTest("Physics", SubTests::Physics);
  AddSubTest("Misc", SubTests::Misc);
}

ezResult ezGameEngineTestVisualScript::InitializeSubTest(ezInt32 iIdentifier)
{
  EZ_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame = -1;
  m_uiImgCompIdx = 0;
  m_ImgCompFrames.Clear();

  if (iIdentifier == SubTests::Variables)
  {
    m_ImgCompFrames.PushBack(1);

    return m_pOwnApplication->LoadScene("VisualScript/AssetCache/Common/Scenes/Variables.ezBinScene");
  }
  else if (iIdentifier == SubTests::Coroutines)
  {
    m_ImgCompFrames.PushBack(1);
    m_ImgCompFrames.PushBack(4);
    m_ImgCompFrames.PushBack(10);
    m_ImgCompFrames.PushBack(17);

    return m_pOwnApplication->LoadScene("VisualScript/AssetCache/Common/Scenes/Coroutines.ezBinScene");
  }
  else if (iIdentifier == SubTests::Messages)
  {
    m_ImgCompFrames.PushBack(1);
    m_ImgCompFrames.PushBack(4);

    return m_pOwnApplication->LoadScene("VisualScript/AssetCache/Common/Scenes/Messages.ezBinScene");
  }
  else if (iIdentifier == SubTests::EnumsAndSwitch)
  {
    m_ImgCompFrames.PushBack(1);
    m_ImgCompFrames.PushBack(2);
    m_ImgCompFrames.PushBack(3);
    m_ImgCompFrames.PushBack(6);

    return m_pOwnApplication->LoadScene("VisualScript/AssetCache/Common/Scenes/EnumsAndSwitch.ezBinScene");
  }
  else if (iIdentifier == SubTests::Blackboard)
  {
    m_ImgCompFrames.PushBack(1);

    return m_pOwnApplication->LoadScene("VisualScript/AssetCache/Common/Scenes/Blackboard.ezBinScene");
  }
  else if (iIdentifier == SubTests::Loops)
  {
    m_ImgCompFrames.PushBack(1);
    m_ImgCompFrames.PushBack(8);
    m_ImgCompFrames.PushBack(9);
    m_ImgCompFrames.PushBack(41);
    m_ImgCompFrames.PushBack(42);

    return m_pOwnApplication->LoadScene("VisualScript/AssetCache/Common/Scenes/Loops.ezBinScene");
  }
  else if (iIdentifier == SubTests::Loops2)
  {
    m_ImgCompFrames.PushBack(1);
    m_ImgCompFrames.PushBack(2);
    m_ImgCompFrames.PushBack(5);
    m_pTestLog = EZ_DEFAULT_NEW(TestLog);
    m_pTestLog->m_Interface.ExpectMessage("Maximum node executions (100000) reached, execution will be aborted. Does the script contain an infinite loop?");

    return m_pOwnApplication->LoadScene("VisualScript/AssetCache/Common/Scenes/Loops2.ezBinScene");
  }
  else if (iIdentifier == SubTests::Loops3)
  {
    m_ImgCompFrames.PushBack(1);

    return m_pOwnApplication->LoadScene("VisualScript/AssetCache/Common/Scenes/Loops3.ezBinScene");
  }
  else if (iIdentifier == SubTests::Properties)
  {
    m_ImgCompFrames.PushBack(1);

    return m_pOwnApplication->LoadScene("VisualScript/AssetCache/Common/Scenes/Properties.ezBinScene");
  }
  else if (iIdentifier == SubTests::Arrays)
  {
    m_ImgCompFrames.PushBack(1);

    return m_pOwnApplication->LoadScene("VisualScript/AssetCache/Common/Scenes/Arrays.ezBinScene");
  }
  else if (iIdentifier == SubTests::Expressions)
  {
    m_ImgCompFrames.PushBack(1);
    m_ImgCompFrames.PushBack(15);
    m_ImgCompFrames.PushBack(31);

    return m_pOwnApplication->LoadScene("VisualScript/AssetCache/Common/Scenes/Expressions.ezBinScene");
  }
  else if (iIdentifier == SubTests::Physics)
  {
    m_ImgCompFrames.PushBack(2);

    return m_pOwnApplication->LoadScene("VisualScript/AssetCache/Common/Scenes/Physics.ezBinScene");
  }
  else if (iIdentifier == SubTests::Misc)
  {
    m_ImgCompFrames.PushBack(2);

    return m_pOwnApplication->LoadScene("VisualScript/AssetCache/Common/Scenes/Misc.ezBinScene");
  }

  return EZ_FAILURE;
}

ezTestAppRun ezGameEngineTestVisualScript::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  const bool bVulkan = ezGameApplication::GetActiveRenderer().IsEqual_NoCase("Vulkan");
  ++m_iFrame;

  m_pOwnApplication->Run();
  if (m_pOwnApplication->ShouldApplicationQuit())
  {
    m_pTestLog = nullptr;
    return ezTestAppRun::Quit;
  }

  if (m_ImgCompFrames[m_uiImgCompIdx] == m_iFrame)
  {
    EZ_TEST_IMAGE(m_uiImgCompIdx, bVulkan ? 300 : 250);
    ++m_uiImgCompIdx;

    if (m_uiImgCompIdx >= m_ImgCompFrames.GetCount())
    {
      m_pTestLog = nullptr;
      return ezTestAppRun::Quit;
    }
  }

  return ezTestAppRun::Continue;
}
