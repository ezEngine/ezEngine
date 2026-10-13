#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ShaderResource.h>

/// Render pass that upscales a texture to a multi-sampled render target.
///
/// Converts a regular texture to an MSAA texture by replicating samples. Used when
/// transitioning from non-MSAA to MSAA rendering in the pipeline.
///
/// If MSAA_Mode is None, or the input already has that sample count, the input is forwarded unchanged.
/// An input with a different, non-zero sample count is reported as an error.
class EZ_RENDERERCORE_DLL ezMsaaUpscalePass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMsaaUpscalePass, ezRenderPipelinePass);

public:
  ezMsaaUpscalePass();
  ~ezMsaaUpscalePass();

  virtual ezStatus AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs) override;
  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

protected:
  ezRenderPipelineNodeInputPin m_PinInput;
  ezRenderPipelineNodeOutputPin m_PinOutput;

  ezEnum<ezGALMSAASampleCount> m_MsaaMode = ezGALMSAASampleCount::None;
  ezShaderResourceHandle m_hShader;
};
