#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/FrameDataProvider.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>

/// GPU-side data for clustered rendering.
///
/// Contains GPU buffers for lights, decals, probes, cluster assignments, and related resources.
/// Uploaded from ezClusteredDataCPU by the data provider and bound to shaders for rendering.
struct EZ_RENDERERCORE_DLL ezClusteredDataGPU
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezClusteredDataGPU);

public:
  ezClusteredDataGPU();
  ~ezClusteredDataGPU();

  ezUInt32 m_uiSkyIrradianceIndex = 0;
  ezEnum<ezCameraUsageHint> m_cameraUsageHint = ezCameraUsageHint::Default;

  ezGALBufferHandle m_hLightDataBuffer;
  ezGALBufferHandle m_hDecalDataBuffer;
  ezGALBufferHandle m_hReflectionProbeDataBuffer;
  ezGALBufferHandle m_hClusterDataBuffer;
  ezGALBufferHandle m_hClusterItemBuffer;

  ezConstantBufferStorageHandle m_hConstantBuffer;

  ezGALSamplerStateHandle m_hShadowSampler;

  ezDecalAtlasResourceHandle m_hDecalAtlas;
  ezGALSamplerStateHandle m_hDecalAtlasSampler;

  void BindResources(ezRenderContext* pRenderContext);
};

/// Provides GPU buffers for clustered rendering each frame.
///
/// Converts CPU-side clustered data from ezClusteredDataExtractor into GPU buffers.
/// The buffers contain lights, decals, reflection probes, and per-cluster item lists
/// used by shaders to efficiently look up which items affect each pixel.
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
