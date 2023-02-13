#include <GameEngineTest/GameEngineTestPCH.h>

#include "EffectsTest.h"
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Components/ParticleComponent.h>

static ezGameEngineTestEffects s_GameEngineTestEffects;

const char* ezGameEngineTestEffects::GetTestName() const
{
  return "Effects Tests";
}

ezGameEngineTestApplication* ezGameEngineTestEffects::CreateApplication()
{
  m_pOwnApplication = EZ_DEFAULT_NEW(ezGameEngineTestApplication, "Effects");
  return m_pOwnApplication;
}

void ezGameEngineTestEffects::SetupSubTests()
{
  AddSubTest("Decals", SubTests::Decals);
  AddSubTest("Heightfield", SubTests::Heightfield);
  AddSubTest("WindClothRopes", SubTests::WindClothRopes);
  AddSubTest("Reflections", SubTests::Reflections);
  AddSubTest("StressTest", SubTests::StressTest);
}

ezResult ezGameEngineTestEffects::InitializeSubTest(ezInt32 iIdentifier)
{
  EZ_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame = -1;
  m_uiImgCompIdx = 0;
  m_ImgCompFrames.Clear();

  if (iIdentifier == SubTests::Decals)
  {
    m_ImgCompFrames.PushBack({5});
    m_ImgCompFrames.PushBack({30});
    m_ImgCompFrames.PushBack({60});

    EZ_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("Effects/AssetCache/Common/Scenes/Decals.ezObjectGraph"));
    return EZ_SUCCESS;
  }

  if (iIdentifier == SubTests::Heightfield)
  {
    m_ImgCompFrames.PushBack({20});

    EZ_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("Effects/AssetCache/Common/Scenes/Heightfield.ezObjectGraph"));
    return EZ_SUCCESS;
  }

  if (iIdentifier == SubTests::WindClothRopes)
  {
    m_ImgCompFrames.PushBack({20, 550});
    m_ImgCompFrames.PushBack({100, 600});

    EZ_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("Effects/AssetCache/Common/Scenes/Wind.ezObjectGraph"));
    return EZ_SUCCESS;
  }
  if (iIdentifier == SubTests::Reflections)
  {
    m_ImgCompFrames.PushBack({30});

    EZ_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("Effects/AssetCache/Common/Scenes/Reflections.ezObjectGraph"));
    return EZ_SUCCESS;
  }
  if (iIdentifier == SubTests::StressTest)
  {
    m_ImgCompFrames.PushBack({100});

    EZ_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("Effects/AssetCache/Common/Scenes/StressTest.ezObjectGraph"));
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezTestAppRun ezGameEngineTestEffects::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  ++m_iFrame;

  if (m_pOwnApplication->Run() == ezApplication::Execution::Quit)
    return ezTestAppRun::Quit;

  if (m_ImgCompFrames[m_uiImgCompIdx].m_uiFrame == m_iFrame)
  {
    EZ_TEST_IMAGE(m_uiImgCompIdx, m_ImgCompFrames[m_uiImgCompIdx].m_uiThreshold);
    ++m_uiImgCompIdx;

    if (m_uiImgCompIdx >= m_ImgCompFrames.GetCount())
    {
      if (false)
      {
        ezProfilingSystem::ProfilingData profilingData;
        ezProfilingSystem::Capture(profilingData);

        ezStringBuilder sPath(":appdata/Profiling/", ezApplication::GetApplicationInstance()->GetApplicationName());
        sPath.AppendPath("effectsProfiling.json");

        ezFileWriter fileWriter;
        if (fileWriter.Open(sPath) == EZ_SUCCESS)
        {
          profilingData.Write(fileWriter).IgnoreResult();
          ezLog::Info("Profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
        }
        else
        {
          ezLog::Error("Could not write profiling capture to '{0}'.", sPath);
        }
      }
      return ezTestAppRun::Quit;
    }
  }

  return ezTestAppRun::Continue;
}
