#include <PCH.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessAppUWP.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererCore/Pipeline/View.h>
#include <EditorEngineProcessFramework/EngineProcess/RemoteViewContext.h>

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
#include <GameEngine/MixedReality/MixedRealityFramework.h>
#include <WindowsMixedReality/HolographicSpace.h>
#include <WindowsMixedReality/SpatialLocationService.h>
#include <WindowsMixedReality/SpatialReferenceFrame.h>
#endif

ezEditorEngineProcessAppUWP::ezEditorEngineProcessAppUWP()
{

}

ezEditorEngineProcessAppUWP::~ezEditorEngineProcessAppUWP()
{
}

ezViewHandle ezEditorEngineProcessAppUWP::CreateRemoteWindowAndView(ezCamera* pCamera)
{
  EZ_ASSERT_DEV(IsRemoteMode(), "Incorrect app mode");

  CreateRemoteWindow();

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
  if (m_hRemoteView.IsInvalidated())
  {
    auto hRenderPipeline = ezResourceManager::LoadResource<ezRenderPipelineResource>("{ 2fe25ded-776c-7f9e-354f-e4c52a33d125 }");

    auto pHoloFramework = ezMixedRealityFramework::GetSingleton();
    m_hRemoteView = pHoloFramework->CreateHolographicView(m_pRemoteWindow.Borrow(), hRenderPipeline, pCamera);
  }
  else
  {
    auto pHoloFramework = ezMixedRealityFramework::GetSingleton();
    pCamera->SetCameraMode(ezCameraMode::Stereo, 90.0f, 0.1f, 100.0f);
    pHoloFramework->SetCameraForPredictionSynchronization(pCamera);
  }
#endif

  return ezEditorEngineProcessApp::CreateRemoteWindowAndView(pCamera);
}

ezRenderPipelineResourceHandle ezEditorEngineProcessAppUWP::CreateDefaultMainRenderPipeline()
{
#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
  if (ezWindowsHolographicSpace::GetSingleton() && ezWindowsHolographicSpace::GetSingleton()->IsAvailable())
  {
    // HololensRenderPipeline.ezRendePipelineAsset
    return ezResourceManager::LoadResource<ezRenderPipelineResource>("{ 2fe25ded-776c-7f9e-354f-e4c52a33d125 }");
  }
#endif

  return ezEditorEngineProcessApp::CreateDefaultMainRenderPipeline();
}

ezRenderPipelineResourceHandle ezEditorEngineProcessAppUWP::CreateDefaultDebugRenderPipeline()
{
  return CreateDefaultMainRenderPipeline();
}

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
void ezEditorEngineProcessAppUWP::SetAnchor(const ezVec3& position)
{
  auto pHoloSpace = ezWindowsHolographicSpace::GetSingleton();
  if (pHoloSpace->IsAvailable())
  {
    auto pCurLoc = pHoloSpace->GetSpatialLocationService().CreateStationaryReferenceFrame_CurrentLocation(*pHoloSpace->GetDefaultReferenceFrame(), position);

    if (pCurLoc)
    {
      pHoloSpace->SetDefaultReferenceFrame(std::move(pCurLoc));
    }
  }
}
#endif

