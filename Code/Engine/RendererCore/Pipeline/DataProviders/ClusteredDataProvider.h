#pragma once

#include <RendererCore/Pipeline/FrameDataProvider.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>

struct ezPerLightData;

struct ezClusteredData
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezClusteredData);
public:

  ezClusteredData();
  ~ezClusteredData();

  ezDynamicArray<ezPerLightData, ezAlignedAllocatorWrapper> m_LightData;
  ezGALBufferHandle m_hLightDataBuffer;

  ezConstantBufferStorageHandle m_hConstantBuffer;

  void BindResources(ezRenderContext* pRenderContext);
};

class EZ_RENDERERCORE_DLL ezClusteredDataProvider : public ezFrameDataProvider<ezClusteredData>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezClusteredDataProvider, ezFrameDataProviderBase);

public:
  ezClusteredDataProvider();
  ~ezClusteredDataProvider();

private:

  virtual void* UpdateData(const ezRenderViewContext& renderViewContext, const ezExtractedRenderData& extractedData) override;

  ezClusteredData m_Data;
};
