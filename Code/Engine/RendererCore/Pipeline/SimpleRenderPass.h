#pragma once

#include <RendererFoundation/Resources/RenderTargetConfig.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>

class EZ_RENDERERCORE_DLL ezSimpleRenderPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSimpleRenderPass);

public:
  ezSimpleRenderPass();
  ezSimpleRenderPass(ezGALRenderTargetConfigHandle hRTConfig);
  ~ezSimpleRenderPass();

  virtual void Execute(const ezRenderViewContext& renderViewContext) override;

private:
  ezGALRenderTargetConfigHandle m_hRTConfig;

  ezPassThroughNodePin m_PinColor;
  ezPassThroughNodePin m_PinDepthStencil;
};
