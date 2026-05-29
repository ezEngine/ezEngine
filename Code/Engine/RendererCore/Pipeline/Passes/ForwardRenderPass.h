#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>

class ezRenderGraphPassBuilder;


/// Base class for forward rendering passes.
///
/// Forward rendering evaluates lighting for each rendered object during the geometry pass.
/// Derived classes implement specific object filtering (opaque, transparent, etc.).
class EZ_RENDERERCORE_DLL ezForwardRenderPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezForwardRenderPass, ezRenderPipelinePass);

public:
  ezForwardRenderPass(const char* szName = "ForwardRenderPass");
  ~ezForwardRenderPass();

  virtual ezStatus AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs) override;

  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

protected:
  /// Configures shader permutation variables based on render settings.
  virtual void SetupPermutationVars(const ezRenderViewContext& renderViewContext);

  /// Renders the objects for this pass. Must be implemented by derived classes.
  virtual void RenderObjects(const ezRenderViewContext& renderViewContext) = 0;

  ezRenderPipelineNodePassThroughPin m_PinColor;          ///< Color target for rendering.
  ezRenderPipelineNodePassThroughPin m_PinDepthStencil;   ///< Depth-stencil target.

  ezEnum<ezForwardRenderShadingQuality> m_ShadingQuality; ///< Quality level for shading calculations.

  ezTexture2DResourceHandle m_hWhiteTexture;              ///< Fallback white texture for unbound inputs.
};
