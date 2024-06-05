#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

/// \brief Blends the two inputs by the given blend factor and writes the result to output.
/// Note that while the output format is taken from InputA, both inputs really should have the same size and format.
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
  ezRenderPipelineNodeInputPin m_PinInputA;
  ezRenderPipelineNodeInputPin m_PinInputB;
  ezRenderPipelineNodeOutputPin m_PinOutput;

  float m_fBlendFactor = 0.5f;
  ezShaderResourceHandle m_hShader;
};
