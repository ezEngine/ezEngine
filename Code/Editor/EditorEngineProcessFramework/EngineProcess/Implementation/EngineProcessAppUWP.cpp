#include <EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessAppUWP.h>
#include <EditorEngineProcessFramework/EngineProcess/RemoteViewContext.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
#include <GameEngine/MixedReality/MixedRealityFramework.h>
#include <WindowsMixedReality/HolographicSpace.h>
#include <WindowsMixedReality/SpatialAnchor.h>
#include <WindowsMixedReality/SpatialLocationService.h>
#include <WindowsMixedReality/SpatialReferenceFrame.h>
#include <windows.perception.spatial.h>
#endif

ezEditorEngineProcessAppUWP::ezEditorEngineProcessAppUWP() {}

ezEditorEngineProcessAppUWP::~ezEditorEngineProcessAppUWP() {}

ezViewHandle ezEditorEngineProcessAppUWP::CreateRemoteWindowAndView(ezCamera* pCamera)
{
  EZ_ASSERT_DEV(IsRemoteMode(), "Incorrect app mode");

  CreateRemoteWindow();

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
  if (m_hRemoteView.IsInvalidated())
  {
    // HololensRenderPipeline.ezRenderPipelineAsset
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
  if (pHoloSpace == nullptr || !pHoloSpace->IsAvailable())
    return;

  auto locService = pHoloSpace->GetSpatialLocationService();

  auto pCurLoc = locService.CreateStationaryReferenceFrame_CurrentHeading(*pHoloSpace->GetDefaultReferenceFrame(), ezTransform(position));

  if (pCurLoc)
  {
    pHoloSpace->SetDefaultReferenceFrame(std::move(pCurLoc));

    auto pAnchor = locService.CreateSpatialAnchor(ezTransform::IdentityTransform());

    locService.SavePersistentAnchor(*pAnchor, "EngineProcessOrigin");
  }
}

void ezEditorEngineProcessAppUWP::SetAnchor(const ezTransform& offset)
{
  auto pHoloSpace = ezWindowsHolographicSpace::GetSingleton();
  if (pHoloSpace == nullptr || !pHoloSpace->IsAvailable())
    return;

  auto locService = pHoloSpace->GetSpatialLocationService();

  auto pCurLoc = locService.CreateStationaryReferenceFrame(*pHoloSpace->GetDefaultReferenceFrame(), offset);

  if (pCurLoc)
  {
    pHoloSpace->SetDefaultReferenceFrame(std::move(pCurLoc));

    auto pAnchor = locService.CreateSpatialAnchor(ezTransform::IdentityTransform());

    locService.SavePersistentAnchor(*pAnchor, "EngineProcessOrigin");
  }
}

void ezEditorEngineProcessAppUWP::LoadAnchor()
{
  if (m_bAnchorLoaded)
    return;

  auto pHoloSpace = ezWindowsHolographicSpace::GetSingleton();
  if (pHoloSpace == nullptr || !pHoloSpace->IsAvailable())
  {
    m_bAnchorLoaded = true;
    return;
  }

  auto locService = pHoloSpace->GetSpatialLocationService();
  if (!locService.IsPersistantAnchorDataLoaded())
    return;

  m_bAnchorLoaded = true;

  auto pAnchor = locService.LoadPersistentAnchor("EngineProcessOrigin");
  if (pAnchor == nullptr)
    return;

  ComPtr<ABI::Windows::Perception::Spatial::ISpatialCoordinateSystem> pReferenceCoords;
  pHoloSpace->GetDefaultReferenceFrame()->GetInternalCoordinateSystem(pReferenceCoords);

  ComPtr<ABI::Windows::Perception::Spatial::ISpatialCoordinateSystem> pAnchorCoords;
  pAnchor->GetInternalCoordinateSystem(pAnchorCoords);

  ComPtr<__FIReference_1_Windows__CFoundation__CNumerics__CMatrix4x4> pMatToAnchor;
  if (FAILED(pAnchorCoords->TryGetTransformTo(pReferenceCoords.Get(), &pMatToAnchor)) || pMatToAnchor == nullptr)
    return;

  ezMat4 mMatToAnchor = ezUwpUtils::ConvertMat4(pMatToAnchor.Get());
  ezTransform tToAnchor;
  tToAnchor.SetIdentity();
  tToAnchor.m_vPosition = mMatToAnchor.GetTranslationVector();
  tToAnchor.m_qRotation.SetFromMat3(mMatToAnchor.GetRotationalPart());
  SetAnchor(tToAnchor);
}

#endif
