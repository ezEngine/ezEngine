#pragma once

#include <RendererCore/Pipeline/Passes/ForwardRenderPass.h>

/// Forward render pass that renders opaque objects.
///
/// Renders all opaque geometry with full lighting and shading.
/// Optionally accepts an ambient occlusion input for enhanced shading.
class EZ_RENDERERCORE_DLL ezOpaqueForwardRenderPass : public ezForwardRenderPass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezOpaqueForwardRenderPass, ezForwardRenderPass);

public:
  ezOpaqueForwardRenderPass(const char* szName = "OpaqueForwardRenderPass");
  ~ezOpaqueForwardRenderPass();

  virtual ezStatus AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs) override;

protected:
  virtual void SetupPermutationVars(const ezRenderViewContext& renderViewContext) override;

  virtual void RenderObjects(const ezRenderViewContext& renderViewContext) override;

  ezRenderPipelineNodeInputPin m_PinSSAO;        ///< Optional SSAO input for ambient occlusion.
  ezRenderPipelineNodeInputPin m_PinShadowMasks; ///< Optional shadow mask input for deferred shadows.

  bool m_bWriteDepth = true;                     ///< Whether to write to the depth buffer.
};
