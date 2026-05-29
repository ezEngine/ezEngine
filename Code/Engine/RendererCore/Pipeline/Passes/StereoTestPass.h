#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ShaderResource.h>

/// Render pass for testing stereo rendering configurations.
///
/// Debug pass used to verify stereo rendering pipeline setup and eye separation.
class EZ_RENDERERCORE_DLL ezStereoTestPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStereoTestPass, ezRenderPipelinePass);

public:
  ezStereoTestPass();
  ~ezStereoTestPass();

  virtual ezStatus AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs) override;

protected:
  ezRenderPipelineNodeInputPin m_PinInput;   ///< Input texture.
  ezRenderPipelineNodeOutputPin m_PinOutput; ///< Output texture with stereo test pattern.

  ezShaderResourceHandle m_hShader;          ///< Shader for generating the stereo test pattern.
};
