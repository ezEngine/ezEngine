#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

/// Render pass that pre-filters cubemaps for image-based lighting.
///
/// Generates filtered specular reflections and irradiance data from an input cubemap.
/// Creates mipmap chains with increasing roughness for specular reflections and computes
/// diffuse irradiance. Used for physically-based rendering with environment maps.
class EZ_RENDERERCORE_DLL ezReflectionFilterPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezReflectionFilterPass, ezRenderPipelinePass);

public:
  ezReflectionFilterPass();
  ~ezReflectionFilterPass();

  virtual ezStatus AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs) override;
  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

  ezUInt32 GetInputCubemap() const;
  void SetInputCubemap(ezUInt32 uiCubemapHandle);

protected:
  void UpdateFilteredSpecularConstantBuffer(ezUInt32 uiMipMapIndex, ezUInt32 uiNumMipMaps, ezUInt32 uiWidth, ezUInt32 uiHeight);
  void UpdateIrradianceConstantBuffer();

  ezRenderPipelineNodeOutputPin m_PinFilteredSpecular;
  ezRenderPipelineNodeOutputPin m_PinAvgLuminance;
  ezRenderPipelineNodeOutputPin m_PinIrradianceData;

  float m_fDiffuseIntensity = 1.0f;
  float m_fDiffuseSaturation = 1.0f;
  float m_fSpecularIntensity = 1.0f;
  ezUInt32 m_uiSpecularOutputIndex = 0;
  ezUInt32 m_uiIrradianceOutputIndex = 0;

  ezGALTextureHandle m_hInputCubemap;

  ezConstantBufferStorageHandle m_hFilteredSpecularConstantBuffer;
  ezShaderResourceHandle m_hFilteredSpecularShader;

  ezConstantBufferStorageHandle m_hIrradianceConstantBuffer;
  ezShaderResourceHandle m_hIrradianceShader;
};
