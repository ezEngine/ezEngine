#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

class EZ_RENDERERCORE_DLL ezReflectionFilterPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezReflectionFilterPass, ezRenderPipelinePass);

public:
  ezReflectionFilterPass();
  ~ezReflectionFilterPass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) override;

  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;

  ezUInt32 GetInputCubemap() const;
  void SetInputCubemap(ezUInt32 uiCubemapHandle);

protected:
  void UpdateFilteredSpecularConstantBuffer(ezUInt32 uiMipMapIndex, ezUInt32 uiNumMipMaps, ezUInt32 outputIndex);
  void UpdateIrradianceConstantBuffer();

  ezRenderPipelineNodeOutputPin m_PinFilteredSpecular;
  ezRenderPipelineNodeOutputPin m_PinAvgLuminance;
  ezRenderPipelineNodeOutputPin m_PinIrradianceData;

  float m_fIntensity;
  float m_fSaturation;
  ezUInt32 m_uiSpecularOutputIndex;
  ezUInt32 m_uiIrradianceOutputIndex;

  ezGALTextureHandle m_hInputCubemap;

  ezConstantBufferStorageHandle m_hFilteredSpecularConstantBuffer;
  ezShaderResourceHandle m_hFilteredSpecularShader;

  ezConstantBufferStorageHandle m_hIrradianceConstantBuffer;
  ezShaderResourceHandle m_hIrradianceShader;
};
