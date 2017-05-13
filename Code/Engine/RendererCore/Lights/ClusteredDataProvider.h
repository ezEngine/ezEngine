#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/FrameDataProvider.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>

struct ezClusteredDataGPU
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezClusteredDataGPU);
public:

  ezClusteredDataGPU();
  ~ezClusteredDataGPU();

  ezGALBufferHandle m_hLightDataBuffer;
  ezGALBufferHandle m_hClusterDataBuffer;
  ezGALBufferHandle m_hClusterItemBuffer;

  ezConstantBufferStorageHandle m_hConstantBuffer;

  ezGALSamplerStateHandle m_hShadowSampler;
  ezTexture2DResourceHandle m_hNoiseTexture;

  void BindResources(ezRenderContext* pRenderContext);
};

class EZ_RENDERERCORE_DLL ezClusteredDataProvider : public ezFrameDataProvider<ezClusteredDataGPU>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezClusteredDataProvider, ezFrameDataProviderBase);

public:
  ezClusteredDataProvider();
  ~ezClusteredDataProvider();

private:

  virtual void* UpdateData(const ezRenderViewContext& renderViewContext, const ezExtractedRenderData& extractedData) override;

  ezClusteredDataGPU m_Data;
};
