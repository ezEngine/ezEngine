#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>

/// Renders the world space output of ezDebugRenderer (lines, boxes, 3D text).
///
/// Connect the scene's depth buffer, so that the output is occluded by the scene.
class EZ_RENDERERCORE_DLL ezDebugWorldRenderPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDebugWorldRenderPass, ezRenderPipelinePass);

public:
  ezDebugWorldRenderPass(const char* szName = "DebugWorldRenderPass");
  ~ezDebugWorldRenderPass();

  virtual ezStatus AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs) override;

protected:
  ezRenderPipelineNodePassThroughPin m_PinColor;
  ezRenderPipelineNodePassThroughPin m_PinDepthStencil; ///< Has to match the size and MSAA mode of the color target, otherwise adding the pass fails.
};

/// Renders the screen space output of ezDebugRenderer (2D text, info text, 2D lines and rectangles).
///
/// The output uses pixel coordinates of the view's viewport, so with a render scale this pass has to come after the ezUpscalePass.
class EZ_RENDERERCORE_DLL ezDebugScreenRenderPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDebugScreenRenderPass, ezRenderPipelinePass);

public:
  ezDebugScreenRenderPass(const char* szName = "DebugScreenRenderPass");
  ~ezDebugScreenRenderPass();

  virtual ezStatus AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs) override;
  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

  /// Sets a text that is displayed in the top left corner. Not reflected, only used by the fallback for a missing pipeline asset.
  void SetMessage(ezStringView sMessage) { m_sMessage = sMessage; }

protected:
  ezRenderPipelineNodePassThroughPin m_PinColor;

  ezString m_sMessage;
};
