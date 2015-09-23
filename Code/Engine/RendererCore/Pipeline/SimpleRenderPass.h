#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>

class EZ_RENDERERCORE_DLL ezSimpleRenderPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSimpleRenderPass);

public:
  ezSimpleRenderPass(const char* szName = "SimpleRenderPass");
  ezSimpleRenderPass(const ezGALRenderTagetSetup& RenderTargetSetup, const char* szName = "SimpleRenderPass");
  ~ezSimpleRenderPass();

  virtual void Execute(const ezRenderViewContext& renderViewContext) override;

private:

  ezPassThroughNodePin m_PinColor;
  ezPassThroughNodePin m_PinDepthStencil;

  ezGALRenderTagetSetup m_RenderTargetSetup;
};
