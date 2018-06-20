#pragma once

#include <RendererCore/Pipeline/Extractor.h>
#include <Foundation/Types/UniquePtr.h>

struct ezPerLightData;
struct ezPerDecalData;
struct ezPerClusterData;

class ezClusteredDataCPU : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezClusteredDataCPU, ezRenderData);
public:

  ezClusteredDataCPU();
  ~ezClusteredDataCPU();

  enum
  {
    MAX_LIGHT_DATA = 1024,
    MAX_DECAL_DATA = 1024,
    MAX_ITEMS_PER_CLUSTER = 256
  };

  ezArrayPtr<ezPerLightData> m_LightData;
  ezArrayPtr<ezPerDecalData> m_DecalData;
  ezArrayPtr<ezPerClusterData> m_ClusterData;
  ezArrayPtr<ezUInt32> m_ClusterItemList;

  ezColor m_AmbientTopColor;
  ezColor m_AmbientBottomColor;

  float m_fFogHeight;
  float m_fFogHeightFalloff;
  float m_fFogDensityAtCameraPos;
  float m_fFogDensity;
  ezColor m_FogColor;
};

class EZ_RENDERERCORE_DLL ezClusteredDataExtractor : public ezExtractor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezClusteredDataExtractor, ezExtractor);
public:
  ezClusteredDataExtractor(const char* szName = "ClusteredDataExtractor");
  ~ezClusteredDataExtractor();

  virtual void PostSortAndBatch(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects,
    ezExtractedRenderData& extractedRenderData) override;

private:
  void FillItemListAndClusterData(ezClusteredDataCPU* pData);

  template <ezUInt32 MaxData>
  struct TempCluster
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt32 m_BitMask[MaxData / 32];
  };

  ezDynamicArray<ezPerLightData, ezAlignedAllocatorWrapper> m_TempLightData;
  ezDynamicArray<ezPerDecalData, ezAlignedAllocatorWrapper> m_TempDecalData;
  ezDynamicArray<TempCluster<ezClusteredDataCPU::MAX_LIGHT_DATA>> m_TempLightsClusters;
  ezDynamicArray<TempCluster<ezClusteredDataCPU::MAX_DECAL_DATA>> m_TempDecalsClusters;
  ezDynamicArray<ezUInt32> m_TempClusterItemList;

  ezDynamicArray<ezSimdBSphere, ezAlignedAllocatorWrapper> m_ClusterBoundingSpheres;
};

