#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>

/// \brief A standard forward render pass that renders into the color target.
class EZ_RENDERERCORE_DLL ezForwardRenderPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezForwardRenderPass, ezRenderPipelinePass);

public:
  ezForwardRenderPass(const char* szName = "ForwardRenderPass");
  ~ezForwardRenderPass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs,
    ezArrayPtr<ezGALTextureCreationDescription> outputs) override;
  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
    const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;

protected:

  virtual void SetupPermutationVars(const ezRenderViewContext& renderViewContext);

  ezPassThroughNodePin m_PinColor;
  ezPassThroughNodePin m_PinDepthStencil;
};
