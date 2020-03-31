#include <GameEngineTestPCH.h>

#include "EffectsTest.h"
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>
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
}

ezResult ezGameEngineTestEffects::InitializeSubTest(ezInt32 iIdentifier)
{
  SUPER::InitializeSubTest(iIdentifier);

  m_iFrame = -1;
  m_iImgCompIdx = 0;
  m_ImgCompFrames.Clear();

  if (iIdentifier == SubTests::Decals)
  {
    m_ImgCompFrames.PushBack(1);
    m_ImgCompFrames.PushBack(30);
    m_ImgCompFrames.PushBack(60);

    m_pOwnApplication->LoadScene("Effects/AssetCache/Common/Scenes/Decals.ezObjectGraph");
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezTestAppRun ezGameEngineTestEffects::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  ++m_iFrame;

  if (m_pOwnApplication->Run() == ezApplication::Quit)
    return ezTestAppRun::Quit;

  if (m_ImgCompFrames[m_iImgCompIdx] == m_iFrame)
  {
    EZ_TEST_IMAGE(m_iImgCompIdx, 100);
    ++m_iImgCompIdx;

    if (m_iImgCompIdx >= m_ImgCompFrames.GetCount())
    {
      return ezTestAppRun::Quit;
    }
  }

  return ezTestAppRun::Continue;
}
