#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>

/// \brief A render pass that renders debug geometry.
class EZ_RENDERERCORE_DLL ezDebugRenderPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDebugRenderPass, ezRenderPipelinePass);

public:
  ezDebugRenderPass(const char* szName = "DebugRenderPass");
  ~ezDebugRenderPass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs,
    ezArrayPtr<ezGALTextureCreationDescription> outputs) override;
  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
    const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;

protected:
  ezPassThroughNodePin m_PinColor;
  ezPassThroughNodePin m_PinDepthStencil;
};
