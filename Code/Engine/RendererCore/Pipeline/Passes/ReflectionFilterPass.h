#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

class EZ_RENDERERCORE_DLL ezReflectionFilterPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezReflectionFilterPass, ezRenderPipelinePass);

public:
  ezReflectionFilterPass();
  ~ezReflectionFilterPass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs,
    ezArrayPtr<ezGALTextureCreationDescription> outputs) override;

  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
    const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;

  ezUInt32 GetInputCubemap() const;
  void SetInputCubemap(ezUInt32 uiCubemapHandle);

protected:

  void UpdateIrradianceConstantBuffer();

  ezOutputNodePin m_PinFilteredSpecular;
  ezOutputNodePin m_PinAvgLuminance;
  ezOutputNodePin m_PinIrradianceData;

  float m_fIntensity;
  float m_fSaturation;
  ezUInt32 m_uiOutputIndex;

  ezGALTextureHandle m_hInputCubemap;

  ezConstantBufferStorageHandle m_hIrradianceConstantBuffer;
  ezShaderResourceHandle m_hIrradianceShader;
};

