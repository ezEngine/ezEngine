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

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) override;

  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;
  virtual void ExecuteInactive(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;
  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

  void SetFadeOutStart(float fStart);
  float GetFadeOutStart() const;

  void SetFadeOutEnd(float fEnd);
  float GetFadeOutEnd() const;

protected:
  void CreateSamplerState();

  ezRenderPipelineNodeInputPin m_PinDepthInput;
  ezRenderPipelineNodeOutputPin m_PinOutput;

  float m_fRadius = 1.0f;
  float m_fMaxScreenSpaceRadius = 1.0f;
  float m_fContrast = 2.0f;
  float m_fIntensity = 0.7f;

  float m_fFadeOutStart = 80.0f;
  float m_fFadeOutEnd = 100.0f;

  float m_fPositionBias = 5.0f;
  float m_fMipLevelScale = 10.0f;
  float m_fDepthBlurThreshold = 2.0f;

  ezConstantBufferStorageHandle m_hDownscaleConstantBuffer;
  ezConstantBufferStorageHandle m_hSSAOConstantBuffer;

  ezTexture2DResourceHandle m_hNoiseTexture;

  ezGALSamplerStateHandle m_hSSAOSamplerState;

  ezShaderResourceHandle m_hDownscaleShader;
  ezShaderResourceHandle m_hSSAOShader;
  ezShaderResourceHandle m_hBlurShader;
};
