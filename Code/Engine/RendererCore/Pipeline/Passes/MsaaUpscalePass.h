#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ShaderResource.h>

/// Render pass that upscales a texture to a multi-sampled render target.
///
/// Converts a regular texture to an MSAA texture by replicating samples. Used when
/// transitioning from non-MSAA to MSAA rendering in the pipeline.
class EZ_RENDERERCORE_DLL ezMsaaUpscalePass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMsaaUpscalePass, ezRenderPipelinePass);

public:
  ezMsaaUpscalePass();
  ~ezMsaaUpscalePass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) override;

  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;
  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

protected:
  ezRenderPipelineNodeInputPin m_PinInput;
  ezRenderPipelineNodeOutputPin m_PinOutput;

  ezEnum<ezGALMSAASampleCount> m_MsaaMode = ezGALMSAASampleCount::None;
  ezShaderResourceHandle m_hShader;
};
