#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ShaderResource.h>

/// Render pass that applies post-process anti-aliasing.
///
/// Currently it only does an advanced resolve of MSAA render targets using a two pixel wide bspline filter.
class EZ_RENDERERCORE_DLL ezAntialiasingPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAntialiasingPass, ezRenderPipelinePass);

public:
  ezAntialiasingPass();
  ~ezAntialiasingPass();

  virtual ezStatus AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs) override;

  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

protected:
  ezRenderPipelineNodeInputPin m_PinInput;
  ezRenderPipelineNodeOutputPin m_PinOutput;

  ezHashedString m_sMsaaSampleCount;
  ezShaderResourceHandle m_hShader;
};
