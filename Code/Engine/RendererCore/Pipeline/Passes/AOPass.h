#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

class EZ_RENDERERCORE_DLL ezAOPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAOPass, ezRenderPipelinePass);

public:
  ezAOPass();
  ~ezAOPass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs,
    ezArrayPtr<ezGALTextureCreationDescription> outputs) override;

  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
    const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;
  virtual void ExecuteInactive(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
    const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;

  void SetFadeOutStart(float fStart);
  float GetFadeOutStart() const;

  void SetFadeOutEnd(float fEnd);
  float GetFadeOutEnd() const;

protected:

  void CreateSamplerState();

  ezInputNodePin m_PinDepthInput;
  ezOutputNodePin m_PinOutput;

  float m_fRadius;
  float m_fMaxScreenSpaceRadius;
  float m_fContrast;
  float m_fIntensity;

  float m_fFadeOutStart;
  float m_fFadeOutEnd;

  float m_fPositionBias;
  float m_fMipLevelScale;
  float m_fDepthBlurThreshold;

  ezConstantBufferStorageHandle m_hDownscaleConstantBuffer;
  ezConstantBufferStorageHandle m_hSSAOConstantBuffer;

  ezTexture2DResourceHandle m_hNoiseTexture;

  ezGALSamplerStateHandle m_hSSAOSamplerState;

  ezShaderResourceHandle m_hDownscaleShader;
  ezShaderResourceHandle m_hSSAOShader;
  ezShaderResourceHandle m_hBlurShader;
};
