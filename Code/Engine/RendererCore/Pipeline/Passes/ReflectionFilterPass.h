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
  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

  ezUInt32 GetInputCubemap() const;
  void SetInputCubemap(ezUInt32 uiCubemapHandle);

protected:
  void UpdateFilteredSpecularConstantBuffer(ezUInt32 uiMipMapIndex, ezUInt32 uiNumMipMaps);
  void UpdateIrradianceConstantBuffer();

  ezRenderPipelineNodeOutputPin m_PinFilteredSpecular;
  ezRenderPipelineNodeOutputPin m_PinAvgLuminance;
  ezRenderPipelineNodeOutputPin m_PinIrradianceData;

  float m_fIntensity = 1.0f;
  float m_fSaturation = 1.0f;
  ezUInt32 m_uiSpecularOutputIndex = 0;
  ezUInt32 m_uiIrradianceOutputIndex = 0;

  ezGALTextureHandle m_hInputCubemap;

  ezConstantBufferStorageHandle m_hFilteredSpecularConstantBuffer;
  ezShaderResourceHandle m_hFilteredSpecularShader;

  ezConstantBufferStorageHandle m_hIrradianceConstantBuffer;
  ezShaderResourceHandle m_hIrradianceShader;
};
