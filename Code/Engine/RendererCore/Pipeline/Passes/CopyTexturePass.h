#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ShaderResource.h>

/// Render pass that copies a texture from input to output.
///
/// Simple utility pass for duplicating textures within the render pipeline.
/// Can be used to preserve intermediate results or create copies for multi-pass effects.
// BEGIN-DOCS-CODE-SNIPPET: renderpass-header
class EZ_RENDERERCORE_DLL ezCopyTexturePass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCopyTexturePass, ezRenderPipelinePass);

public:
  ezCopyTexturePass();
  ~ezCopyTexturePass();

  virtual ezStatus AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs) override;

protected:
  ezRenderPipelineNodeInputPin m_PinInput;
  ezRenderPipelineNodeOutputPin m_PinOutput;
};
// END-DOCS-CODE-SNIPPET
