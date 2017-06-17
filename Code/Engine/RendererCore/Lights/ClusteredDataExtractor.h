#pragma once

#include <RendererCore/Pipeline/Extractor.h>
#include <Foundation/Types/UniquePtr.h>

struct ezPerLightData;
struct ezPerClusterData;

class ezClusteredDataCPU : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezClusteredDataCPU, ezRenderData);
public:

  enum
  {
    MAX_LIGHT_DATA = 1024,
    MAX_LIGHTS_PER_CLUSTER = 256
  };

  ezArrayPtr<ezPerLightData> m_LightData;
  ezArrayPtr<ezPerClusterData> m_ClusterData;
  ezArrayPtr<ezUInt32> m_ClusterItemList;

  ezColor m_AmbientTopColor;
  ezColor m_AmbientBottomColor;
};

class EZ_RENDERERCORE_DLL ezClusteredDataExtractor : public ezExtractor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezClusteredDataExtractor, ezExtractor);
public:
  ezClusteredDataExtractor(const char* szName = "ClusteredDataExtractor");
  ~ezClusteredDataExtractor();

  virtual void PostSortAndBatch(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects,
    ezExtractedRenderData* pExtractedRenderData) override;

private:
  void FillItemListAndClusterData(ezClusteredDataCPU* pData);

  struct TempCluster
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt32 m_BitMask[ezClusteredDataCPU::MAX_LIGHT_DATA / 32];
  };

  ezDynamicArray<ezPerLightData, ezAlignedAllocatorWrapper> m_TempLightData;
  ezDynamicArray<TempCluster> m_TempClusters;
  ezDynamicArray<ezUInt32> m_TempClusterItemList;

  ezDynamicArray<ezSimdBSphere, ezAlignedAllocatorWrapper> m_ClusterBoundingSpheres;
};
