#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>

/// Forward render pass that renders secondary lens flares.
class EZ_RENDERERCORE_DLL ezLensEffectsPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLensEffectsPass, ezRenderPipelinePass);

public:
  ezLensEffectsPass(const char* szName = "LensEffectsPass");
  ~ezLensEffectsPass();

  virtual ezStatus AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs) override;

protected:
  ezRenderPipelineNodePassThroughPin m_PinColor;
  ezRenderPipelineNodeInputPin m_PinResolvedDepth;
};
