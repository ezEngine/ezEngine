#include <GameEngineTest/GameEngineTestPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)

#  include "KrautTest.h"
#  include <Core/WorldSerializer/WorldReader.h>
#  include <Foundation/IO/FileSystem/FileReader.h>
#  include <ParticlePlugin/Components/ParticleComponent.h>

static ezGameEngineTestKraut s_GameEngineTestAnimations;

const char* ezGameEngineTestKraut::GetTestName() const
{
  return "Kraut Tests";
}

ezGameEngineTestApplication* ezGameEngineTestKraut::CreateApplication()
{
  m_pOwnApplication = EZ_DEFAULT_NEW(ezGameEngineTestApplication, "PlatformWin");
  return m_pOwnApplication;
}

void ezGameEngineTestKraut::SetupSubTests()
{
  AddSubTest("TreeRendering", SubTests::TreeRendering);
}

ezResult ezGameEngineTestKraut::InitializeSubTest(ezInt32 iIdentifier)
{
  EZ_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame = -1;
  m_uiImgCompIdx = 0;
  m_ImgCompFrames.Clear();

  if (iIdentifier == SubTests::TreeRendering)
  {
    m_ImgCompFrames.PushBack(3);
    m_ImgCompFrames.PushBack(60);

    EZ_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("PlatformWin/AssetCache/Common/Kraut/Kraut.ezBinScene"));
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezTestAppRun ezGameEngineTestKraut::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  ++m_iFrame;

  m_pOwnApplication->Run();
  if (m_pOwnApplication->ShouldApplicationQuit())
    return ezTestAppRun::Quit;

  if (m_ImgCompFrames[m_uiImgCompIdx] == m_iFrame)
  {
    // wind simulation is done during rendering, which seems to introduce very slight differences due to threading
    // which, over many frames, can result in slightly different results
    EZ_TEST_IMAGE(m_uiImgCompIdx, 1300);
    ++m_uiImgCompIdx;

    if (m_uiImgCompIdx >= m_ImgCompFrames.GetCount())
    {
      return ezTestAppRun::Quit;
    }
  }

  return ezTestAppRun::Continue;
}

#endif
