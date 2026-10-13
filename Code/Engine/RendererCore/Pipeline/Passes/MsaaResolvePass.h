#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ShaderResource.h>

/// Render pass that resolves a multi-sampled texture to a non-multi-sampled texture.
///
/// Converts MSAA render targets to regular textures by averaging the samples. Supports both
/// color and depth textures. Required when using MSAA rendering with post-processing effects
/// that cannot operate on multi-sampled textures.
///
/// If the input is not multi-sampled, it is copied, so the same pipeline works with and without MSAA.
/// With GuaranteeCopy disabled, the input is forwarded instead, which saves the copy, but then the output is the same
/// texture as the input. Passes that modify the input in place (pass-through pins) are only ordered after the direct
/// consumers of the input, not after the consumers of this pass's output, so those may observe the modified content.
class EZ_RENDERERCORE_DLL ezMsaaResolvePass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMsaaResolvePass, ezRenderPipelinePass);

public:
  ezMsaaResolvePass();
  ~ezMsaaResolvePass();

  virtual ezStatus AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs) override;
  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

protected:
  ezRenderPipelineNodeInputPin m_PinInput;
  ezRenderPipelineNodeOutputPin m_PinOutput;

  /// Without MSAA, copies the input instead of forwarding it, so that in-place writes to the input don't affect the consumers of the output.
  bool m_bGuaranteeCopy = true;
  bool m_bIsDepth = false;
  ezGALMSAASampleCount::Enum m_MsaaSampleCount = ezGALMSAASampleCount::None;
  ezShaderResourceHandle m_hDepthResolveShader;
};
