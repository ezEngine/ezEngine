#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>

/// Render pass that renders geometry to a depth buffer without color output.
///
/// Used for depth pre-pass, shadow map generation, or depth-only rendering.
/// Can selectively render static, dynamic, and transparent objects.
class EZ_RENDERERCORE_DLL ezDepthOnlyPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDepthOnlyPass, ezRenderPipelinePass);

public:
  ezDepthOnlyPass(const char* szName = "DepthOnlyPass");
  ~ezDepthOnlyPass();

  virtual ezStatus AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs) override;

  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

protected:
  ezRenderPipelineNodePassThroughPin m_PinDepthStencil; ///< Depth-stencil target for depth writes.

  bool m_bRenderStaticObjects = true;                   ///< Whether to render static objects.
  bool m_bRenderDynamicObjects = true;                  ///< Whether to render dynamic objects.
  bool m_bRenderTransparentObjects = false;             ///< Whether to render transparent objects.
};
