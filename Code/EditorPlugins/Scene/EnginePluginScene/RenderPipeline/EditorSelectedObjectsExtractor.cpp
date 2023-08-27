#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/RenderPipeline/EditorSelectedObjectsExtractor.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/SwapChain.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorSelectedObjectsExtractor, 1, ezRTTIDefaultAllocator<ezEditorSelectedObjectsExtractor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("SceneContext", GetSceneContext, SetSceneContext),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezEditorSelectedObjectsExtractor::ezEditorSelectedObjectsExtractor()
{
  m_pSceneContext = nullptr;
}

ezEditorSelectedObjectsExtractor::~ezEditorSelectedObjectsExtractor()
{
  ezRenderWorld::DeleteView(m_hRenderTargetView);
}

const ezDeque<ezGameObjectHandle>* ezEditorSelectedObjectsExtractor::GetSelection()
{
  if (m_pSceneContext == nullptr)
    return nullptr;

  return &m_pSceneContext->GetSelectionWithChildren();
}

void ezEditorSelectedObjectsExtractor::Extract(
  const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData& ref_extractedRenderData)
{
  const bool bShowCameraOverlays = view.GetCameraUsageHint() == ezCameraUsageHint::EditorView;

  if (bShowCameraOverlays && m_pSceneContext && m_pSceneContext->GetRenderSelectionBoxes())
  {
    const ezDeque<ezGameObjectHandle>* pSelection = GetSelection();
    if (pSelection == nullptr)
      return;

    const ezCameraComponent* pCamComp = nullptr;

    CreateRenderTargetTexture(view);

    EZ_LOCK(view.GetWorld()->GetReadMarker());

    for (const auto& hObj : *pSelection)
    {
      const ezGameObject* pObject = nullptr;
      if (!view.GetWorld()->TryGetObject(hObj, pObject))
        continue;

      if (FilterByViewTags(view, pObject))
        continue;

      if (pObject->TryGetComponentOfBaseType(pCamComp))
      {
        UpdateRenderTargetCamera(pCamComp);

        const float fAspect = 9.0f / 16.0f;

        // TODO: use aspect ratio of camera render target, if available
        ezDebugRenderer::Draw2DRectangle(view.GetHandle(), ezRectFloat(20, 20, 256, 256 * fAspect), 0, ezColor::White, m_hRenderTarget);

        // TODO: if the camera renders to a texture anyway, use its view + render target instead

        ezRenderWorld::AddViewToRender(m_hRenderTargetView);

        break;
      }
    }
  }

  ezSelectedObjectsExtractorBase::Extract(view, visibleObjects, ref_extractedRenderData);
}

ezResult ezEditorSelectedObjectsExtractor::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  return EZ_SUCCESS;
}

ezResult ezEditorSelectedObjectsExtractor::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_IGNORE_UNUSED(uiVersion);
  return EZ_SUCCESS;
}

void ezEditorSelectedObjectsExtractor::CreateRenderTargetTexture(const ezView& view)
{
  if (m_hRenderTarget.IsValid())
    return;

  m_hRenderTarget = ezResourceManager::GetExistingResource<ezRenderToTexture2DResource>("EditorCameraRT");

  if (!m_hRenderTarget.IsValid())
  {
    const float fAspect = 9.0f / 16.0f;
    const ezUInt32 uiWidth = 256;

    ezRenderToTexture2DResourceDescriptor d;
    d.m_Format = ezGALResourceFormat::RGBAUByteNormalizedsRGB;
    d.m_uiWidth = uiWidth;
    d.m_uiHeight = (ezUInt32)(uiWidth * fAspect);

    m_hRenderTarget = ezResourceManager::GetOrCreateResource<ezRenderToTexture2DResource>("EditorCameraRT", std::move(d));
  }

  CreateRenderTargetView(view);
}

void ezEditorSelectedObjectsExtractor::CreateRenderTargetView(const ezView& view)
{
  EZ_ASSERT_DEV(m_hRenderTargetView.IsInvalidated(), "Render target view is already created");

  ezResourceLock<ezRenderToTexture2DResource> pRenderTarget(m_hRenderTarget, ezResourceAcquireMode::BlockTillLoaded);

  ezStringBuilder name("EditorCameraRT");

  ezView* pRenderTargetView = nullptr;
  m_hRenderTargetView = ezRenderWorld::CreateView(name, pRenderTargetView);

  // MainRenderPipeline.ezRenderPipelineAsset
  auto hRenderPipeline = ezResourceManager::LoadResource<ezRenderPipelineResource>("{ c533e113-2a4c-4f42-a546-653c78f5e8a7 }");
  pRenderTargetView->SetRenderPipelineResource(hRenderPipeline);

  // TODO: get rid of const cast ?
  pRenderTargetView->SetWorld(const_cast<ezWorld*>(view.GetWorld()));
  pRenderTargetView->SetCamera(&m_RenderTargetCamera);

  m_RenderTargetCamera.SetCameraMode(ezCameraMode::PerspectiveFixedFovY, 45, 0.1f, 100.0f);

  ezGALRenderTargets renderTargets;
  renderTargets.m_hRTs[0] = pRenderTarget->GetGALTexture();
  pRenderTargetView->SetRenderTargets(renderTargets);

  const float resX = (float)pRenderTarget->GetWidth();
  const float resY = (float)pRenderTarget->GetHeight();

  pRenderTargetView->SetViewport(ezRectFloat(0, 0, resX, resY));
}

void ezEditorSelectedObjectsExtractor::UpdateRenderTargetCamera(const ezCameraComponent* pCamComp)
{
  float fFarPlane = ezMath::Max(pCamComp->GetNearPlane() + 0.00001f, pCamComp->GetFarPlane());
  switch (pCamComp->GetCameraMode())
  {
    case ezCameraMode::OrthoFixedHeight:
    case ezCameraMode::OrthoFixedWidth:
      m_RenderTargetCamera.SetCameraMode(pCamComp->GetCameraMode(), pCamComp->GetOrthoDimension(), pCamComp->GetNearPlane(), fFarPlane);
      break;
    case ezCameraMode::PerspectiveFixedFovX:
    case ezCameraMode::PerspectiveFixedFovY:
      m_RenderTargetCamera.SetCameraMode(pCamComp->GetCameraMode(), pCamComp->GetFieldOfView(), pCamComp->GetNearPlane(), fFarPlane);
      break;
    case ezCameraMode::Stereo:
      m_RenderTargetCamera.SetCameraMode(ezCameraMode::PerspectiveFixedFovY, 45, pCamComp->GetNearPlane(), fFarPlane);
      break;
    default:
      break;
  }


  ezView* pRenderTargetView = nullptr;
  if (!ezRenderWorld::TryGetView(m_hRenderTargetView, pRenderTargetView))
    return;

  pRenderTargetView->m_IncludeTags = pCamComp->m_IncludeTags;
  pRenderTargetView->m_ExcludeTags = pCamComp->m_ExcludeTags;
  pRenderTargetView->m_ExcludeTags.SetByName("Editor");

  if (pCamComp->GetRenderPipeline().IsValid())
  {
    pRenderTargetView->SetRenderPipelineResource(pCamComp->GetRenderPipeline());
  }
  else
  {
    // MainRenderPipeline.ezRenderPipelineAsset
    auto hRenderPipeline = ezResourceManager::LoadResource<ezRenderPipelineResource>("{ c533e113-2a4c-4f42-a546-653c78f5e8a7 }");
    pRenderTargetView->SetRenderPipelineResource(hRenderPipeline);
  }

  const ezVec3 pos = pCamComp->GetOwner()->GetGlobalPosition();
  const ezVec3 dir = pCamComp->GetOwner()->GetGlobalDirForwards();
  const ezVec3 up = pCamComp->GetOwner()->GetGlobalDirUp();

  m_RenderTargetCamera.LookAt(pos, pos + dir, up);
  m_RenderTargetCamera.SetExposure(pCamComp->GetExposure());
}
