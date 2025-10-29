#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

/// Render pass that applies a gaussian blur to an input texture.
///
/// Performs a two-pass separable gaussian blur with configurable radius.
/// Output has the same format as the input.
class EZ_RENDERERCORE_DLL ezBlurPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezBlurPass, ezRenderPipelinePass);

public:
  ezBlurPass();
  ~ezBlurPass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) override;

  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;
  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

  void SetRadius(ezInt32 iRadius);           ///< Sets the blur radius in pixels.
  ezInt32 GetRadius() const;                 ///< Returns the current blur radius.

protected:
  ezRenderPipelineNodeInputPin m_PinInput;   ///< Input texture to blur.
  ezRenderPipelineNodeOutputPin m_PinOutput; ///< Blurred output texture.

  ezInt32 m_iRadius = 15;                    ///< Blur radius in pixels.
  ezConstantBufferStorageHandle m_hBlurCB;   ///< Constant buffer for blur parameters.
  ezShaderResourceHandle m_hShader;          ///< Blur shader.
};
