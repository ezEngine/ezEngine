#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

/// Render pass that blends two input textures together.
///
/// Linearly interpolates between two input textures using a blend factor.
/// The output format matches InputA. Both inputs should have the same size and format.
class EZ_RENDERERCORE_DLL ezBlendPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezBlendPass, ezRenderPipelinePass);

public:
  ezBlendPass();
  ~ezBlendPass();

  virtual ezStatus AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs) override;
  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

protected:
  ezRenderPipelineNodeInputPin m_PinInputA;  ///< First input texture.
  ezRenderPipelineNodeInputPin m_PinInputB;  ///< Second input texture.
  ezRenderPipelineNodeOutputPin m_PinOutput; ///< Blended output.

  float m_fBlendFactor = 0.5f;               ///< Blend factor between inputs (0 = full A, 1 = full B).
  ezShaderResourceHandle m_hShader;          ///< Shader for blending operation.
};
