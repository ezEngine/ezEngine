#pragma once

#include <RendererCore/Pipeline/Extractor.h>

/// Minimal CPU-side lighting data for simplified rendering.
///
/// Used when clustered rendering is not needed or available. Contains only basic
/// lighting information like sky irradiance.
class ezSimplifiedDataCPU : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSimplifiedDataCPU, ezRenderData);

public:
  ezSimplifiedDataCPU();
  ~ezSimplifiedDataCPU();

  ezUInt32 m_uiSkyIrradianceIndex = 0;
  ezEnum<ezCameraUsageHint> m_cameraUsageHint = ezCameraUsageHint::Default;
};

/// Minimal GPU-side lighting data for simplified rendering.
///
/// Contains only essential lighting data uploaded to the GPU. Used with ezSimplifiedDataExtractor
/// for rendering paths that don't require full clustered lighting.
struct EZ_RENDERERCORE_DLL ezSimplifiedDataGPU
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezSimplifiedDataGPU);

public:
  ezSimplifiedDataGPU();
  ~ezSimplifiedDataGPU();

  ezUInt32 m_uiSkyIrradianceIndex = 0;
  ezEnum<ezCameraUsageHint> m_cameraUsageHint = ezCameraUsageHint::Default;
  ezGALBufferHandle m_hConstantBuffer;
};

/// Extracts minimal lighting data for simplified rendering.
///
/// Alternative to ezClusteredDataExtractor for cases where full clustered rendering
/// is not required. Provides basic lighting information without the overhead of
/// spatial clustering. Used for lower-end rendering paths or specific view types.
class EZ_RENDERERCORE_DLL ezSimplifiedDataExtractor : public ezExtractor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSimplifiedDataExtractor, ezExtractor);

public:
  ezSimplifiedDataExtractor(const char* szName = "SimplifiedDataExtractor");
  ~ezSimplifiedDataExtractor();

  virtual void Extract(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData& ref_extractedRenderData) override {}
  virtual void PostSortAndBatch(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData& ref_extractedRenderData) override;

  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

private:
  void UpdateGpuData(const ezView& view, const ezSimplifiedDataCPU* pData);
  void AddGpuData(const ezView& view, ezExtractedRenderData& ref_extractedRenderData);

private:
  ezSimplifiedDataGPU m_DataGPU;
};
