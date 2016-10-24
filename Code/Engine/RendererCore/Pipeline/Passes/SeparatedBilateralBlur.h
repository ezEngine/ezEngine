#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Declarations.h>
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

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs,
    ezArrayPtr<ezGALTextureCreationDescription> outputs) override;

  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
    const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;

  void SetRadius(ezUInt32 uiRadius);
  ezUInt32 GetRadius() const;

  void SetGaussianSigma(float fSigma);
  float GetGaussianSigma() const;

  void SetSharpness(float fSharpness);
  float GetSharpness() const;

protected:
  ezInputNodePin m_PinBlurSourceInput;
  ezInputNodePin m_PinDepthInput;
  ezOutputNodePin m_PinOutput;

  ezUInt32 m_uiRadius;
  float m_fGaussianSigma;
  float m_fSharpness;
  ezConstantBufferStorageHandle m_hBilateralBlurCB;
  ezShaderResourceHandle m_hShader;
};
