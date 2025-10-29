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

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) override;

  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;
  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

protected:
  ezRenderPipelineNodeInputPin m_PinInputA;  ///< First input texture.
  ezRenderPipelineNodeInputPin m_PinInputB;  ///< Second input texture.
  ezRenderPipelineNodeOutputPin m_PinOutput; ///< Blended output.

  float m_fBlendFactor = 0.5f;               ///< Blend factor between inputs (0 = full A, 1 = full B).
  ezShaderResourceHandle m_hShader;          ///< Shader for blending operation.
};
