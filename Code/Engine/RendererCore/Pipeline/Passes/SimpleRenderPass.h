#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>

/// Basic render pass that renders into a color target.
///
/// Can work as passthrough or directly render into the view's render target if no input is present.
/// It renders unlit and foreground objects. Debug output of ezDebugRenderer is rendered by ezDebugRenderPass instead.
class EZ_RENDERERCORE_DLL ezSimpleRenderPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSimpleRenderPass, ezRenderPipelinePass);

public:
  ezSimpleRenderPass(const char* szName = "SimpleRenderPass");
  ~ezSimpleRenderPass();

  virtual ezStatus AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs) override;
  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

protected:
  ezRenderPipelineNodePassThroughPin m_PinColor;        ///< Color target pass-through.
  ezRenderPipelineNodePassThroughPin m_PinDepthStencil; ///< Depth-stencil target pass-through.
};
