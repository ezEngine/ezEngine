#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/TextureCubeAsset/TextureCubeContext.h>
#include <EnginePluginAssets/TextureCubeAsset/TextureCubeView.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/RendererReflection.h>

ezTextureCubeViewContext::ezTextureCubeViewContext(ezTextureCubeContext* pContext)
  : ezEngineProcessViewContext(pContext)
{
  m_pTextureContext = pContext;
}

ezTextureCubeViewContext::~ezTextureCubeViewContext() = default;

ezViewHandle ezTextureCubeViewContext::CreateView()
{
  ezView* pView = CreateDefaultView("Texture Cube Editor - View");
  pView->SetRenderPipelineResource(CreateDebugRenderPipeline());
  pView->GetBlackboard()->SetEntryValue(ezMakeHashedString("DepthPrePass.Active"), false);
  pView->GetBlackboard()->SetEntryValue(ezMakeHashedString("AOPass.Active"), false);

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

    ezStringBuilder sText;
    if (!ezReflectionUtils::EnumerationToString(ezGetStaticRTTI<ezGALResourceFormat>(), format, sText, ezReflectionUtils::EnumConversionMode::ValueNameOnly))
    {
      sText = "Unknown format";
    }

    sText.PrependFormat("{0}x{1}x6 - ", uiWidthAndHeight, uiWidthAndHeight);

    ezDebugRenderer::DrawInfoText(m_hView, ezDebugTextPlacement::BottomLeft, "AssetStats", sText);
  }
}
