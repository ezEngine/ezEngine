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

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) override;

  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;

protected:
  ezRenderPipelineNodeInputPin m_PinInput;
  ezRenderPipelineNodeOutputPin m_PinOutput;

  bool m_bIsDepth = false;
  ezGALMSAASampleCount::Enum m_MsaaSampleCount = ezGALMSAASampleCount::None;
  ezShaderResourceHandle m_hDepthResolveShader;
};
