#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ShaderResource.h>

/// Render pass that stretches its input to the size of the view's viewport.
///
/// Counterpart to source passes with 'ApplyRenderScale' enabled: the scene is rendered at a reduced resolution and this pass
/// brings the result back to the output size with bilinear filtering. Typically placed after tonemapping, so that UI
/// rendered afterwards stays at full resolution. Passes after it must not use the scaled depth buffer, since its size doesn't match.
///
/// Optionally sharpens the result, to counter the blur of the bilinear filtering. The sharpening adapts to the local contrast and is meant for
/// tonemapped input in [0; 1] range; brighter pixels are left unchanged.
///
/// If the input already has the viewport size, it is forwarded to the output without any rendering, so there is also no sharpening then.
class EZ_RENDERERCORE_DLL ezUpscalePass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezUpscalePass, ezRenderPipelinePass);

public:
  ezUpscalePass();
  ~ezUpscalePass();

  virtual ezStatus AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs) override;
  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

protected:
  ezRenderPipelineNodeInputPin m_PinInput;
  ezRenderPipelineNodeOutputPin m_PinOutput;

  float m_fSharpness = 0.5f; ///< Sharpening at half resolution and below, 0 disables it. Less is applied between that and full resolution.

  ezShaderResourceHandle m_hShader;
  ezConstantBufferStorageHandle m_hConstantBuffer;
};
