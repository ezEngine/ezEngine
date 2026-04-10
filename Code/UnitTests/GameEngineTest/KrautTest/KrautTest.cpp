#include <GameEngineTest/GameEngineTestPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP) || EZ_ENABLED(EZ_PLATFORM_LINUX)

#  include "KrautTest.h"
#  include <Core/WorldSerializer/WorldReader.h>
#  include <Foundation/IO/FileSystem/FileReader.h>
#  include <KrautPlugin/Components/KrautTreeComponent.h>
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

    // Force synchronous tree generation so image comparisons are deterministic.
    // Without this, async generation means the tree may not be visible at frame 3
    // and wind accumulation differs from the reference images.
    {
      ezWorld* pWorld = m_pOwnApplication->GetWorld();
      EZ_LOCK(pWorld->GetWriteMarker());
      ezKrautTreeComponentManager* pManager = pWorld->GetOrCreateComponentManager<ezKrautTreeComponentManager>();
      for (auto it = pManager->GetComponents(); it.IsValid(); it.Next())
      {
        it->m_bForceGenerateImmediate = true;
      }
    }

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
    EZ_TEST_IMAGE(m_uiImgCompIdx, 450);
    ++m_uiImgCompIdx;

    if (m_uiImgCompIdx >= m_ImgCompFrames.GetCount())
    {
      return ezTestAppRun::Quit;
    }
  }

  return ezTestAppRun::Continue;
}

#endif
