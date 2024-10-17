#pragma once

#include <RendererCore/Pipeline/Extractor.h>

class ezSimplifiedDataCPU : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSimplifiedDataCPU, ezRenderData);

public:
  ezSimplifiedDataCPU();
  ~ezSimplifiedDataCPU();

  ezUInt32 m_uiSkyIrradianceIndex = 0;
  ezEnum<ezCameraUsageHint> m_cameraUsageHint = ezCameraUsageHint::Default;
};

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
};
