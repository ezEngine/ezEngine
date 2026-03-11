#include <GameEngineTest/GameEngineTestPCH.h>

#include "SubstanceTest.h"
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>

#include <optional>

static ezGameEngineTestSubstance s_GameEngineTestAnimations;

const char* ezGameEngineTestSubstance::GetTestName() const
{
  return "Substance Tests";
}

ezGameEngineTestApplication* ezGameEngineTestSubstance::CreateApplication()
{
  m_pOwnApplication = EZ_DEFAULT_NEW(ezGameEngineTestApplication, "Substance");
  return m_pOwnApplication;
}

// static
bool ezGameEngineTestSubstance::HasSubstanceDesignerInstalled()
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  static std::optional<bool> s_Cache;
  if (s_Cache.has_value())
  {
    return *s_Cache;
  }

  auto CheckPath = [&](ezStringView sPath)
  {
    ezStringBuilder path = sPath;
    path.AppendPath("sbscooker.exe");

    if (ezOSFile::ExistsFile(path))
    {
      s_Cache = true;
      return true;
    }

    return false;
  };

  ezStringBuilder sPath = "C:/Program Files/Allegorithmic/Substance Designer";
  if (CheckPath(sPath))
    return true;

  s_Cache = false;
  return false;
#else
  return false;
#endif
}

void ezGameEngineTestSubstance::SetupSubTests()
{
  AddSubTest("Basics", SubTests::Basics);
}

ezResult ezGameEngineTestSubstance::InitializeSubTest(ezInt32 iIdentifier)
{
  EZ_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  if (HasSubstanceDesignerInstalled() == false)
  {
    ezLog::Warning("Substance Designer is not installed. Skipping test.");
    return EZ_SUCCESS;
  }

  m_iFrame = -1;
  m_uiImgCompIdx = 0;
  m_ImgCompFrames.Clear();

  if (iIdentifier == SubTests::Basics)
  {
    m_ImgCompFrames.PushBack(9);

    EZ_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("Substance/AssetCache/Common/Scenes/Substance.ezBinScene"));
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezTestAppRun ezGameEngineTestSubstance::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  if (HasSubstanceDesignerInstalled() == false)
  {
    return ezTestAppRun::Quit;
  }

  const bool bVulkan = ezGameApplication::GetActiveRenderer().IsEqual_NoCase("Vulkan");
  ++m_iFrame;

  m_pOwnApplication->Run();
  if (m_pOwnApplication->ShouldApplicationQuit())
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
