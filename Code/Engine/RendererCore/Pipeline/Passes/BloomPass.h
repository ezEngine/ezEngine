#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

class EZ_RENDERERCORE_DLL ezBloomPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezBloomPass, ezRenderPipelinePass);

public:
  ezBloomPass();
  ~ezBloomPass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs,
    ezArrayPtr<ezGALTextureCreationDescription> outputs) override;

  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
    const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;
  virtual void ExecuteInactive(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
    const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;

protected:

  void UpdateConstantBuffer(ezVec2 pixelSize, const ezColor& tintColor);

  ezInputNodePin m_PinInput;
  ezOutputNodePin m_PinOutput;

  ezUInt32 m_uiNumBlurPasses;
  float m_fRadius;
  float m_fThreshold;
  float m_fIntensity;
  ezHybridArray<ezColor, 8> m_tintColors;
  ezConstantBufferStorageHandle m_hConstantBuffer;
  ezShaderResourceHandle m_hShader;
};
