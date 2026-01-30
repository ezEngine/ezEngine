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

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) override;

  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;

  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

protected:
  ezRenderPipelineNodePassThroughPin m_PinColor;        ///< Color target for rendering.
  ezRenderPipelineNodePassThroughPin m_PinDepthStencil; ///< Depth-stencil target.

  ezString m_sRenderDataCategoryName;
  ezRenderData::Category m_RenderDataCategory;
  ezEnum<ezRenderSortingFunctions> m_SortingFunction;
};
