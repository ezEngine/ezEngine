#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ShaderResource.h>

/// Copies a color texture into a new texture with the format of the view's render target.
///
/// Used in front of ezTargetPass when the input format may not match the target, e.g. on a branch that skips tonemapping.
/// Values are copied as is: HDR input gets clamped, not tonemapped. Size, array size and MSAA sample count are kept.
class EZ_RENDERERCORE_DLL ezConvertToTargetFormatPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezConvertToTargetFormatPass, ezRenderPipelinePass);

public:
  ezConvertToTargetFormatPass();
  ~ezConvertToTargetFormatPass();

  virtual ezStatus AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs) override;

protected:
  ezRenderPipelineNodeInputPin m_PinInput;
  ezRenderPipelineNodeOutputPin m_PinOutput;

  ezShaderResourceHandle m_hShader;
};
