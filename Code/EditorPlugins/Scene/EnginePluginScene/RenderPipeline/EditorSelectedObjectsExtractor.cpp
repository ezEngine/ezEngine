#include <EnginePluginScenePCH.h>

#include <EnginePluginScene/RenderPipeline/EditorSelectedObjectsExtractor.h>
#include <EnginePluginScene/SceneContext/SceneContext.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererFoundation/Device/Device.h>

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

void ezEditorSelectedObjectsExtractor::Extract(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects,
                                               ezExtractedRenderData& extractedRenderData)
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

  ezSelectedObjectsExtractor::Extract(view, visibleObjects, extractedRenderData);
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

    m_hRenderTarget = ezResourceManager::CreateResource<ezRenderToTexture2DResource>("EditorCameraRT", std::move(d));
  }

  CreateRenderTargetView(view);
}

void ezEditorSelectedObjectsExtractor::CreateRenderTargetView(const ezView& view)
{
  EZ_ASSERT_DEV(m_hRenderTargetView.IsInvalidated(), "Render target view is already created");

  ezResourceLock<ezRenderToTexture2DResource> pRenderTarget(m_hRenderTarget, ezResourceAcquireMode::NoFallback);

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

  ezGALRenderTargetSetup renderTargetSetup;
  renderTargetSetup.SetRenderTarget(0, pRenderTarget->GetRenderTargetView());
  pRenderTargetView->SetRenderTargetSetup(renderTargetSetup);

  const float resX = (float)pRenderTarget->GetWidth();
  const float resY = (float)pRenderTarget->GetHeight();

  pRenderTargetView->SetViewport(ezRectFloat(0, 0, resX, resY));
}

void ezEditorSelectedObjectsExtractor::UpdateRenderTargetCamera(const ezCameraComponent* pCamComp)
{
  if (pCamComp->GetCameraMode() == ezCameraMode::OrthoFixedHeight || pCamComp->GetCameraMode() == ezCameraMode::OrthoFixedWidth)
  {
    m_RenderTargetCamera.SetCameraMode(pCamComp->GetCameraMode(), pCamComp->GetOrthoDimension(), pCamComp->GetNearPlane(),
                                       pCamComp->GetFarPlane());
  }
  else
  {
    m_RenderTargetCamera.SetCameraMode(pCamComp->GetCameraMode(), pCamComp->GetFieldOfView(), pCamComp->GetNearPlane(),
                                       pCamComp->GetFarPlane());
  }

  ezView* pRenderTargetView = nullptr;
  if (!ezRenderWorld::TryGetView(m_hRenderTargetView, pRenderTargetView))
    return;

  pRenderTargetView->m_IncludeTags = pCamComp->m_IncludeTags;
  pRenderTargetView->m_ExcludeTags = pCamComp->m_ExcludeTags;

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
