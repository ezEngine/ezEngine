#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/Texture3DAsset/Texture3DContext.h>
#include <EnginePluginAssets/Texture3DAsset/Texture3DView.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/RendererReflection.h>

ezTexture3DViewContext::ezTexture3DViewContext(ezTexture3DContext* pContext)
  : ezEngineProcessViewContext(pContext)
{
  m_pTexture3DContext = pContext;
}

ezTexture3DViewContext::~ezTexture3DViewContext() = default;

ezViewHandle ezTexture3DViewContext::CreateView()
{
  ezView* pView = CreateDefaultView("Texture 3D Editor - View");
  pView->SetRenderPipelineResource(CreateDebugRenderPipeline());
  pView->GetBlackboard()->SetEntryValue(ezMakeHashedString("DepthPrePass.Active"), false);
  pView->GetBlackboard()->SetEntryValue(ezMakeHashedString("AOPass.Active"), false);

  return pView->GetHandle();
}

void ezTexture3DViewContext::SetCamera(const ezViewRedrawMsgToEngine* pMsg)
{
  ezCameraMode::Enum cameraMode = (ezCameraMode::Enum)pMsg->m_iCameraMode;
  m_Camera.SetCameraMode(cameraMode, pMsg->m_fFovOrDim, 0.01f, 100.0f);
  m_Camera.LookAt(pMsg->m_vPosition, pMsg->m_vPosition + pMsg->m_vDirForwards, pMsg->m_vDirUp);

  auto hResource = m_pTexture3DContext->GetTexture();
  if (hResource.IsValid())
  {
    ezResourceLock<ezTexture3DResource> pResource(hResource, ezResourceAcquireMode::AllowLoadingFallback);
    ezGALResourceFormat::Enum format = pResource->GetFormat();
    ezUInt32 uiWidth = pResource->GetWidth();
    ezUInt32 uiHeight = pResource->GetHeight();
    ezUInt32 uiDepth = pResource->GetDepth();

    ezStringBuilder sText;
    if (!ezReflectionUtils::EnumerationToString(ezGetStaticRTTI<ezGALResourceFormat>(), format, sText, ezReflectionUtils::EnumConversionMode::ValueNameOnly))
    {
      sText = "Unknown format";
    }

    sText.PrependFormat("{0}x{1}x{2} - ", uiWidth, uiHeight, uiDepth);

    ezDebugRenderer::DrawInfoText(m_hView, ezDebugTextPlacement::BottomLeft, "AssetStats", sText);
  }
}
