#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ShaderResource.h>

/// Stretches its input to the size of the view's viewport with bilinear filtering.
///
/// Counterpart to source passes with 'ApplyRenderScale' enabled. Typically placed after tonemapping, so that UI rendered afterwards
/// stays at full resolution. Passes after it can't use the scaled depth buffer, since its size doesn't match.
///
/// The optional sharpening is meant for tonemapped input in [0; 1] range, brighter pixels are left unchanged.
/// If the input already has the viewport size, it is forwarded without rendering or sharpening.
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

  float m_fSharpness = 0.5f; ///< Sharpening at half resolution and below, less above that. 0 disables it.

  ezShaderResourceHandle m_hShader;
};
