#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/FrameDataProvider.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>

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
  ezConstantBufferStorageHandle m_hConstantBuffer;

  void BindResources(ezRenderContext* pRenderContext);
};

/// Provides minimal GPU lighting data each frame.
///
/// Uploads basic lighting information from ezSimplifiedDataCPU to GPU buffers.
/// Used as a lighter-weight alternative to ezClusteredDataProvider.
class EZ_RENDERERCORE_DLL ezSimplifiedDataProvider : public ezFrameDataProvider<ezSimplifiedDataGPU>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSimplifiedDataProvider, ezFrameDataProviderBase);

public:
  ezSimplifiedDataProvider();
  ~ezSimplifiedDataProvider();

private:
  virtual void* UpdateData(const ezRenderViewContext& renderViewContext, const ezExtractedRenderData& extractedData) override;

  ezSimplifiedDataGPU m_Data;
};
