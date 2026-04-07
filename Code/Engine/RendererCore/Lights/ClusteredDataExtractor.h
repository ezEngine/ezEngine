#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Pipeline/Extractor.h>

struct ezPerLightData;
struct ezPerDecalData;
struct ezPerReflectionProbeData;
struct ezPerClusterData;

/// CPU-side data for clustered rendering containing lights, decals, and reflection probes.
///
/// Used by the clustered rendering system to organize lights, decals, and probes into spatial clusters
/// for efficient per-pixel lookup during shading. The clusters divide the view frustum into a 3D grid.
class ezClusteredDataCPU : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezClusteredDataCPU, ezRenderData);

public:
  ezClusteredDataCPU();
  ~ezClusteredDataCPU();

  enum
  {
    MAX_NUM_LIGHTS = EZ_BIT(10),
    MAX_NUM_DECALS = EZ_BIT(10),
    MAX_NUM_REFLECTION_PROBES = EZ_BIT(8),
  };

  ezArrayPtr<ezPerLightData> m_LightData;
  ezArrayPtr<ezPerDecalData> m_DecalData;
  ezArrayPtr<ezPerReflectionProbeData> m_ReflectionProbeData;
  ezArrayPtr<ezPerClusterData> m_ClusterData;
  ezArrayPtr<ezUInt32> m_ClusterItemList;

  ezUInt32 m_uiBrightestDirectionalLightIndex = 0;
  ezUInt32 m_uiSkyIrradianceIndex = 0;
  ezEnum<ezCameraUsageHint> m_cameraUsageHint = ezCameraUsageHint::Default;

  float m_fFogHeight = 0.0f;
  float m_fFogHeightFalloff = 0.0f;
  float m_fFogDensityAtCameraPos = 0.0f;
  float m_fFogDensity = 0.0f;
  float m_fFogStartDistance = 0.0f;
  float m_fFogInvSkyDistance = 0.0f;
  ezColor m_FogColor = ezColor::Black;

  ezVec3 m_vLightShaftsDirection = ezVec3::MakeZero();
  float m_fLightShaftsIntensity = 0.0f;
  float m_fLightShaftsMaxBrightness = 0.0f;
  float m_fLightShaftsBrightnessThreshold = 0.0f;
  float m_fLightShaftsDiskMaskRadius = 0.0f;
  ezColorGammaUB m_LightShaftsTintColor = ezColor::White;
};

/// Extracts lights, decals, and reflection probes into a clustered data structure.
///
/// Divides the view frustum into a 3D grid of clusters and assigns visible lights, decals,
/// and reflection probes to each cluster. This enables efficient per-pixel light lookup during
/// rendering. Runs after visibility determination in PostSortAndBatch().
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
  ezDynamicArray<TempCluster<ezClusteredDataCPU::MAX_NUM_LIGHTS>> m_TempLightsClusters;
  ezDynamicArray<TempCluster<ezClusteredDataCPU::MAX_NUM_DECALS>> m_TempDecalsClusters;
  ezDynamicArray<TempCluster<ezClusteredDataCPU::MAX_NUM_REFLECTION_PROBES>> m_TempReflectionProbeClusters;
  ezDynamicArray<ezUInt32> m_TempClusterItemList;

  ezDynamicArray<ezSimdBSphere, ezAlignedAllocatorWrapper> m_ClusterBoundingSpheres;
  ezDynamicArray<ezSimdBSphere, ezAlignedAllocatorWrapper> m_ClusterBoundingSpheresRightEye;
  ezMat4 m_mProjection = ezMat4::MakeZero();
  ezMat4 m_mProjectionRightEye = ezMat4::MakeZero();
};
