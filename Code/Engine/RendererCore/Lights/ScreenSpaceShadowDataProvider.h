#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/FrameDataProvider.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct EZ_RENDERERCORE_DLL ezScreenSpaceShadowData
{
  void BindResources(ezRenderContext* pRenderContext);

  ezGALTextureHandle m_hTexture;
};

/// Provides the screen-space shadow result texture to other render passes.
///
/// The ScreenSpaceShadowPass writes a contact shadow mask into this provider each frame.
/// DeferredLighting and forward passes query it to modulate the directional light shadow term.
class EZ_RENDERERCORE_DLL ezScreenSpaceShadowDataProvider : public ezFrameDataProvider<ezScreenSpaceShadowData>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezScreenSpaceShadowDataProvider, ezFrameDataProviderBase);

public:
  ezScreenSpaceShadowDataProvider();
  ~ezScreenSpaceShadowDataProvider();

  void SetTexture(ezGALTextureHandle hTexture);

private:
  virtual void* UpdateData(const ezRenderViewContext& renderViewContext, const ezExtractedRenderData& extractedData) override;

  ezScreenSpaceShadowData m_Data;
};
