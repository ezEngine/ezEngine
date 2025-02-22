#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Pipeline/Extractor.h>

struct ezPerLightData;
struct ezPerDecalData;
struct ezPerReflectionProbeData;
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
    MAX_REFLECTION_PROBE_DATA = 1024,
    MAX_ITEMS_PER_CLUSTER = 256
  };

  ezArrayPtr<ezPerLightData> m_LightData;
  ezArrayPtr<ezPerDecalData> m_DecalData;
  ezArrayPtr<ezPerReflectionProbeData> m_ReflectionProbeData;
  ezArrayPtr<ezPerClusterData> m_ClusterData;
  ezArrayPtr<ezUInt32> m_ClusterItemList;

  ezUInt32 m_uiSkyIrradianceIndex = 0;
  ezEnum<ezCameraUsageHint> m_cameraUsageHint = ezCameraUsageHint::Default;

  float m_fFogHeight = 0.0f;
  float m_fFogHeightFalloff = 0.0f;
  float m_fFogDensityAtCameraPos = 0.0f;
  float m_fFogDensity = 0.0f;
  float m_fFogInvSkyDistance = 0.0f;
  ezColor m_FogColor = ezColor::Black;
};

class EZ_RENDERERCORE_DLL ezClusteredDataExtractor : public ezExtractor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezClusteredDataExtractor, ezExtractor);

public:
  ezClusteredDataExtractor(const char* szName = "ClusteredDataExtractor");
  ~ezClusteredDataExtractor();

  virtual void Extract(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData& ref_extractedRenderData) override {}
  virtual void PostSortAndBatch(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData& ref_extractedRenderData) override;

  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

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
  ezDynamicArray<ezPerReflectionProbeData, ezAlignedAllocatorWrapper> m_TempReflectionProbeData;
  ezDynamicArray<TempCluster<ezClusteredDataCPU::MAX_LIGHT_DATA>> m_TempLightsClusters;
  ezDynamicArray<TempCluster<ezClusteredDataCPU::MAX_DECAL_DATA>> m_TempDecalsClusters;
  ezDynamicArray<TempCluster<ezClusteredDataCPU::MAX_REFLECTION_PROBE_DATA>> m_TempReflectionProbeClusters;
  ezDynamicArray<ezUInt32> m_TempClusterItemList;

  ezDynamicArray<ezSimdBSphere, ezAlignedAllocatorWrapper> m_ClusterBoundingSpheres;
  ezMat4 m_mProjection = ezMat4::MakeZero();
};
