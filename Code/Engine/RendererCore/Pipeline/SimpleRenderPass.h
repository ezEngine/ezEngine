#pragma once

#include <RendererFoundation/Resources/RenderTargetConfig.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>

class EZ_RENDERERCORE_DLL ezSimpleRenderPass : public ezRenderPipelinePass
{
public:
  ezSimpleRenderPass(ezGALRenderTargetConfigHandle hRTConfig);
  ~ezSimpleRenderPass();

  virtual void Execute(const ezRenderViewContext& renderViewContext) override;

private:
  ezGALRenderTargetConfigHandle m_hRTConfig;
};
