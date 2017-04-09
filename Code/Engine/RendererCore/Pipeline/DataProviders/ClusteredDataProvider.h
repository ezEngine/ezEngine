#pragma once

#include <RendererCore/Pipeline/FrameDataProvider.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>

struct ezPerLightData;
struct ezPerClusterData;

struct ezClusteredData
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezClusteredData);
public:

  enum
  {
    MAX_LIGHT_DATA = 1024,
    MAX_LIGHTS_PER_CLUSTER = 256
  };

  ezClusteredData();
  ~ezClusteredData();

  ezDynamicArray<ezPerLightData, ezAlignedAllocatorWrapper> m_LightData;
  ezDynamicArray<ezPerClusterData, ezAlignedAllocatorWrapper> m_ClusterData;
  ezDynamicArray<ezUInt32, ezAlignedAllocatorWrapper> m_ClusterItemList;

  ezGALBufferHandle m_hLightDataBuffer;
  ezGALBufferHandle m_hClusterDataBuffer;
  ezGALBufferHandle m_hClusterItemListBuffer;

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

  void FillItemListAndClusterData();

  ezClusteredData m_Data;

  struct TempCluster
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt32 m_BitMask[ezClusteredData::MAX_LIGHT_DATA / 32];
  };

  ezDynamicArray<TempCluster> m_TempClusters;
};
