#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

/// Render pass that draws visual highlights around selected objects.
///
/// Reads from the depth-stencil buffer to identify selected objects and renders
/// a colored outline and optional overlay. Used in editors and tools to show selection state.
class EZ_RENDERERCORE_DLL ezSelectionHighlightPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSelectionHighlightPass, ezRenderPipelinePass);

public:
  ezSelectionHighlightPass(const char* szName = "SelectionHighlightPass");
  ~ezSelectionHighlightPass();

  virtual ezStatus AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs) override;
  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

protected:
  ezRenderPipelineNodePassThroughPin m_PinColor;                            ///< Pass-through for color buffer with highlight rendered on top.
  ezRenderPipelineNodeInputPin m_PinDepthStencil;                           ///< Depth-stencil input used to identify selected objects.

  ezShaderResourceHandle m_hShader;                                         ///< Shader for rendering the highlight effect.
  ezConstantBufferStorageHandle m_hConstantBuffer;                          ///< Constant buffer for highlight parameters.

  ezColor m_HighlightColor = ezColorScheme::LightUI(ezColorScheme::Yellow); ///< Color of the highlight outline.
  float m_fOverlayOpacity = 0.1f;                                           ///< Opacity of the overlay drawn over selected objects.
};
