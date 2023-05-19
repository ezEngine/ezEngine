#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

/// \brief Depth aware blur on input and writes it to an output buffer of the same format.
///
/// In theory it is mathematical nonsense to separate a bilateral blur, but it is common praxis and works good enough.
/// (Thus the name "separated" in contrast to "separable")
class EZ_RENDERERCORE_DLL ezSeparatedBilateralBlurPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSeparatedBilateralBlurPass, ezRenderPipelinePass);

public:
  ezSeparatedBilateralBlurPass();
  ~ezSeparatedBilateralBlurPass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) override;

  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;

  void SetRadius(ezUInt32 uiRadius);
  ezUInt32 GetRadius() const;

  void SetGaussianSigma(float fSigma);
  float GetGaussianSigma() const;

  void SetSharpness(float fSharpness);
  float GetSharpness() const;

protected:
  ezRenderPipelineNodeInputPin m_PinBlurSourceInput;
  ezRenderPipelineNodeInputPin m_PinDepthInput;
  ezRenderPipelineNodeOutputPin m_PinOutput;

  ezUInt32 m_uiRadius = 7;
  float m_fGaussianSigma = 3.5f;
  float m_fSharpness = 120.0f;
  ezConstantBufferStorageHandle m_hBilateralBlurCB;
  ezShaderResourceHandle m_hShader;
};
