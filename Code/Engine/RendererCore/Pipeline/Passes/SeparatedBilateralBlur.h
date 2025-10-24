#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

/// Render pass that applies depth-aware blur to preserve edges.
///
/// Uses a bilateral filter that considers both spatial distance and depth difference
/// to avoid blurring across edges. Implemented as a two-pass separable filter, which
/// is technically approximate but works well in practice (hence "separated" not "separable").
class EZ_RENDERERCORE_DLL ezSeparatedBilateralBlurPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSeparatedBilateralBlurPass, ezRenderPipelinePass);

public:
  ezSeparatedBilateralBlurPass();
  ~ezSeparatedBilateralBlurPass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) override;

  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;
  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

  void SetRadius(ezUInt32 uiRadius);   ///< Sets the blur kernel radius in pixels.
  ezUInt32 GetRadius() const;

  void SetGaussianSigma(float fSigma); ///< Sets the gaussian distribution sigma.
  float GetGaussianSigma() const;

  void SetSharpness(float fSharpness); ///< Sets edge preservation strength.
  float GetSharpness() const;

protected:
  ezRenderPipelineNodeInputPin m_PinBlurSourceInput; ///< Source texture to blur.
  ezRenderPipelineNodeInputPin m_PinDepthInput;      ///< Depth buffer for edge detection.
  ezRenderPipelineNodeOutputPin m_PinOutput;         ///< Blurred output.

  ezUInt32 m_uiRadius = 7;                           ///< Blur kernel radius in pixels.
  float m_fGaussianSigma = 3.5f;                     ///< Gaussian distribution sigma.
  float m_fSharpness = 120.0f;                       ///< Edge preservation strength (higher = sharper edges).
  ezConstantBufferStorageHandle m_hBilateralBlurCB;  ///< Constant buffer for blur parameters.
  ezShaderResourceHandle m_hShader;                  ///< Bilateral blur shader.
};
