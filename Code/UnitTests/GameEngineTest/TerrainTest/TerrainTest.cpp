#include <GameEngineTest/GameEngineTestPCH.h>

#include "TerrainTest.h"

static ezGameEngineTestTerrain s_GameEngineTestEffects;

const char* ezGameEngineTestTerrain::GetTestName() const
{
  return "Terrain Tests";
}

ezGameEngineTestApplication* ezGameEngineTestTerrain::CreateApplication()
{
  m_pOwnApplication = EZ_DEFAULT_NEW(ezGameEngineTestApplication, "Terrain");
  return m_pOwnApplication;
}

void ezGameEngineTestTerrain::SetupSubTests()
{
  AddSubTest("Heightfields", SubTests::HeightfieldTerrain);
}

ezResult ezGameEngineTestTerrain::InitializeSubTest(ezInt32 iIdentifier)
{
  EZ_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame = -1;
  m_uiImgCompIdx = 0;
  m_ImgCompFrames.Clear();

  switch (iIdentifier)
  {
    case SubTests::HeightfieldTerrain:
    {
      // due to how the system works, frame 3 is the first one that will see terrain
      m_ImgCompFrames.PushBack({3});

      return m_pOwnApplication->LoadScene("Terrain/AssetCache/Common/Scenes/Heightfields.ezBinScene");
    }

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return EZ_FAILURE;
}

ezTestAppRun ezGameEngineTestTerrain::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  ++m_iFrame;

  m_pOwnApplication->Run();
  if (m_pOwnApplication->ShouldApplicationQuit())
    return ezTestAppRun::Quit;

  m_pOwnApplication->SwitchToCamera(m_iFrame);

  if (m_ImgCompFrames[m_uiImgCompIdx].m_uiFrame == m_iFrame)
  {
    EZ_TEST_IMAGE(m_uiImgCompIdx, m_ImgCompFrames[m_uiImgCompIdx].m_uiThreshold);
    ++m_uiImgCompIdx;

    if (m_uiImgCompIdx >= m_ImgCompFrames.GetCount())
    {
      return ezTestAppRun::Quit;
    }
  }

  return ezTestAppRun::Continue;
}
