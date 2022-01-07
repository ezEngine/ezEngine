#include <GameEngineTest/GameEngineTestPCH.h>

#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <GameEngineTest/StereoTest/StereoTest.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>

static ezStereoTest s_StereoTest;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStereoTestGameState, 1, ezRTTIDefaultAllocator<ezStereoTestGameState>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

void ezStereoTestGameState::OverrideRenderPipeline(ezTypedResourceHandle<ezRenderPipelineResource> hPipeline)
{
  ezView* pView = nullptr;
  if (ezRenderWorld::TryGetView(m_hMainView, pView))
  {
    pView->SetRenderPipelineResource(hPipeline);
  }
}

//////////////////////////////////////////////////////////////////////////

ezStereoTestApplication::ezStereoTestApplication(const char* szProjectDirName)
  : ezGameEngineTestApplication(szProjectDirName)
{
}

ezUniquePtr<ezGameStateBase> ezStereoTestApplication::CreateGameState(ezWorld* pWorld)
{
  return EZ_DEFAULT_NEW(ezStereoTestGameState);
}

//////////////////////////////////////////////////////////////////////////

const char* ezStereoTest::GetTestName() const
{
  return "Stereo Test";
}

ezGameEngineTestApplication* ezStereoTest::CreateApplication()
{
  m_pOwnApplication = EZ_DEFAULT_NEW(ezStereoTestApplication, "XR");
  return m_pOwnApplication;
}

void ezStereoTest::SetupSubTests()
{
  AddSubTest("HoloLensPipeline", SubTests::HoloLensPipeline);
  AddSubTest("DefaultPipeline", SubTests::DefaultPipeline);
}

ezResult ezStereoTest::InitializeSubTest(ezInt32 iIdentifier)
{
  EZ_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame = -1;
  m_iImgCompIdx = 0;
  m_ImgCompFrames.Clear();

  if (iIdentifier == SubTests::HoloLensPipeline)
  {
    m_ImgCompFrames.PushBack(100);

    EZ_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("XR/AssetCache/Common/Scenes/XR.ezObjectGraph"));

    auto renderPipeline = ezResourceManager::LoadResource<ezRenderPipelineResource>("{ 2fe25ded-776c-7f9e-354f-e4c52a33d125 }");
    ezDynamicCast<ezStereoTestGameState*>(m_pOwnApplication->GetActiveGameState())->OverrideRenderPipeline(renderPipeline);

    return EZ_SUCCESS;
  }
  if (iIdentifier == SubTests::DefaultPipeline)
  {
    m_ImgCompFrames.PushBack(100);

    EZ_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("XR/AssetCache/Common/Scenes/XR.ezObjectGraph"));

    auto renderPipeline = ezResourceManager::LoadResource<ezRenderPipelineResource>("{ c533e113-2a4c-4f42-a546-653c78f5e8a7 }");
    ezDynamicCast<ezStereoTestGameState*>(m_pOwnApplication->GetActiveGameState())->OverrideRenderPipeline(renderPipeline);

    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezTestAppRun ezStereoTest::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  ++m_iFrame;

  if (m_pOwnApplication->Run() == ezApplication::Execution::Quit)
    return ezTestAppRun::Quit;

  if (m_ImgCompFrames[m_iImgCompIdx] == m_iFrame)
  {
    EZ_TEST_IMAGE(m_iImgCompIdx, 250);
    ++m_iImgCompIdx;

    if (m_iImgCompIdx >= m_ImgCompFrames.GetCount())
    {
      if (false)
      {
        ezProfilingSystem::ProfilingData profilingData;
        ezProfilingSystem::Capture(profilingData);

        ezStringBuilder sPath(":appdata/Profiling/", ezApplication::GetApplicationInstance()->GetApplicationName());
        sPath.AppendPath("stereoProfiling.json");

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
