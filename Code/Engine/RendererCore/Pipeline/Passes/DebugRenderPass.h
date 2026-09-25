#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>

/// Render pass that renders the world space output of ezDebugRenderer (lines, boxes, 3D text).
///
/// Belongs where the scene is rendered, with the scene's depth buffer connected, so that the output is occluded by the scene.
/// A pipeline without this pass silently shows no world space debug output. See also ezDebugScreenRenderPass.
class EZ_RENDERERCORE_DLL ezDebugWorldRenderPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDebugWorldRenderPass, ezRenderPipelinePass);

public:
  ezDebugWorldRenderPass(const char* szName = "DebugWorldRenderPass");
  ~ezDebugWorldRenderPass();

  virtual ezStatus AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs) override;

protected:
  ezRenderPipelineNodePassThroughPin m_PinColor;
  ezRenderPipelineNodePassThroughPin m_PinDepthStencil; ///< Ignored if its size doesn't match the color target.
};

/// Render pass that renders the screen space output of ezDebugRenderer (2D text, info text, 2D lines and rectangles).
///
/// That output is placed in pixel coordinates of the view's viewport, so with a render scale this pass has to come after the ezUpscalePass.
/// A pipeline without this pass silently shows no screen space debug output. See also ezDebugWorldRenderPass.
class EZ_RENDERERCORE_DLL ezDebugScreenRenderPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDebugScreenRenderPass, ezRenderPipelinePass);

public:
  ezDebugScreenRenderPass(const char* szName = "DebugScreenRenderPass");
  ~ezDebugScreenRenderPass();

  virtual ezStatus AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs) override;
  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

  /// Sets a text that is displayed in the top left corner.
  ///
  /// Not a reflected property, it is only set from code, e.g. by the pipeline that replaces a missing render pipeline asset.
  void SetMessage(ezStringView sMessage) { m_sMessage = sMessage; }

protected:
  ezRenderPipelineNodePassThroughPin m_PinColor;

  ezString m_sMessage;
};
