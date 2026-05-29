#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Pipeline/SortingFunctions.h>

/// Render pass that renders render data with a custom render data category.
class EZ_RENDERERCORE_DLL ezCustomRenderDataPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCustomRenderDataPass, ezRenderPipelinePass);

public:
  ezCustomRenderDataPass(const char* szName = "CustomRenderDataPass");
  ~ezCustomRenderDataPass();

  virtual ezStatus AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs) override;

  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

protected:
  ezRenderPipelineNodePassThroughPin m_PinColor;        ///< Color target for rendering.
  ezRenderPipelineNodePassThroughPin m_PinDepthStencil; ///< Depth-stencil target.

  ezString m_sRenderDataCategoryName;
  ezRenderData::Category m_RenderDataCategory;
  ezEnum<ezRenderSortingFunctions> m_SortingFunction;
};
