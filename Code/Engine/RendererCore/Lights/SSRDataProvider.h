#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/FrameDataProvider.h>

struct EZ_RENDERERCORE_DLL ezSSRData
{
  void BindResources(ezRenderContext* pRenderContext);

  ezGALTextureHandle m_hSSRTexture;
};

/// Provides the SSR result texture to other render passes.
///
/// The SSR pass writes its result into this provider each frame.
/// DeferredLighting queries it to blend screen-space reflections with probe reflections.
/// The texture contains the previous frame's SSR result (one-frame latency is standard).
class EZ_RENDERERCORE_DLL ezSSRDataProvider : public ezFrameDataProvider<ezSSRData>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSSRDataProvider, ezFrameDataProviderBase);

public:
  ezSSRDataProvider();
  ~ezSSRDataProvider();

  void SetSSRTexture(ezGALTextureHandle hTexture);

private:
  virtual void* UpdateData(const ezRenderViewContext& renderViewContext, const ezExtractedRenderData& extractedData) override;

  ezSSRData m_Data;
};