#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ShaderResource.h>

/// Render pass that resolves a multi-sampled texture to a non-multi-sampled texture.
///
/// Converts MSAA render targets to regular textures by averaging the samples. Supports both
/// color and depth textures. Required when using MSAA rendering with post-processing effects
/// that cannot operate on multi-sampled textures.
class EZ_RENDERERCORE_DLL ezMsaaResolvePass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMsaaResolvePass, ezRenderPipelinePass);

public:
  ezMsaaResolvePass();
  ~ezMsaaResolvePass();

  virtual ezStatus AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs) override;

protected:
  ezRenderPipelineNodeInputPin m_PinInput;
  ezRenderPipelineNodeOutputPin m_PinOutput;

  bool m_bIsDepth = false;
  ezGALMSAASampleCount::Enum m_MsaaSampleCount = ezGALMSAASampleCount::None;
  ezShaderResourceHandle m_hDepthResolveShader;
};
