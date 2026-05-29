#pragma once

#include <RendererCore/Pipeline/Passes/ForwardRenderPass.h>

/// Forward render pass that renders transparent objects with proper blending.
///
/// Provides access to the scene color for refraction and distortion effects.
class EZ_RENDERERCORE_DLL ezTransparentForwardRenderPass : public ezForwardRenderPass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTransparentForwardRenderPass, ezForwardRenderPass);

public:
  ezTransparentForwardRenderPass(const char* szName = "TransparentForwardRenderPass");
  ~ezTransparentForwardRenderPass();

  virtual ezStatus AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs) override;

protected:
  virtual void RenderObjects(const ezRenderViewContext& renderViewContext) override;

  void CreateSamplerState();

  ezRenderPipelineNodeInputPin m_PinResolvedDepth;   ///< Optional resolved depth for soft particles.

  ezGALSamplerStateHandle m_hSceneColorSamplerState; ///< Sampler for scene color texture.
};
