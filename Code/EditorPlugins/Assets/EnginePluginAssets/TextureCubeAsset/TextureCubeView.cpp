#include <EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/TextureCubeAsset/TextureCubeContext.h>
#include <EnginePluginAssets/TextureCubeAsset/TextureCubeView.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

ezTextureCubeViewContext::ezTextureCubeViewContext(ezTextureCubeContext* pContext)
  : ezEngineProcessViewContext(pContext)
{
  m_pTextureContext = pContext;
}

ezTextureCubeViewContext::~ezTextureCubeViewContext() {}

ezViewHandle ezTextureCubeViewContext::CreateView()
{
  ezView* pView = nullptr;
  ezRenderWorld::CreateView("Texture Cube Editor - View", pView);

  pView->SetRenderPipelineResource(CreateDebugRenderPipeline());
  pView->SetRenderPassProperty("DepthPrePass", "Active", false);
  pView->SetRenderPassProperty("AOPass", "Active", false);

  ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}

void ezTextureCubeViewContext::SetCamera(const ezViewRedrawMsgToEngine* pMsg)
{
  // Do not apply render mode here otherwise we would switch to a different pipeline.
  // Also use hard-coded clipping planes so the quad is not culled to early.

  ezCameraMode::Enum cameraMode = (ezCameraMode::Enum)pMsg->m_iCameraMode;
  m_Camera.SetCameraMode(cameraMode, pMsg->m_fFovOrDim, 0.0001f, 50.0f);
  m_Camera.LookAt(pMsg->m_vPosition, pMsg->m_vPosition + pMsg->m_vDirForwards, pMsg->m_vDirUp);

  // Draw some stats
  auto hResource = m_pTextureContext->GetTexture();
  if (hResource.IsValid())
  {
    ezResourceLock<ezTextureCubeResource> pResource(hResource, ezResourceAcquireMode::AllowLoadingFallback);
    ezGALResourceFormat::Enum format = pResource->GetFormat();
    ezUInt32 uiWidthAndHeight = pResource->GetWidthAndHeight();

    const ezUInt32 viewHeight = pMsg->m_uiWindowHeight;

    ezStringBuilder sText;
    if (ezReflectionUtils::EnumerationToString(ezGetStaticRTTI<ezGALResourceFormat>(), format, sText))
    {
      sText.Shrink(21, 0);
    }
    else
    {
      sText = "Unknown format";
    }

    sText.PrependFormat("{0}x{1}x6 - ", uiWidthAndHeight, uiWidthAndHeight);

    ezDebugRenderer::Draw2DText(m_hView, sText, ezVec2I32(10, viewHeight - 10), ezColor::White, 16,
      ezDebugRenderer::HorizontalAlignment::Left, ezDebugRenderer::VerticalAlignment::Bottom);
  }
}
