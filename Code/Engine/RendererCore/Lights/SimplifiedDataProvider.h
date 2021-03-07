#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/FrameDataProvider.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>

struct EZ_RENDERERCORE_DLL ezSimplifiedDataGPU
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezSimplifiedDataGPU);

public:
  ezSimplifiedDataGPU();
  ~ezSimplifiedDataGPU();

  ezUInt32 m_uiSkyIrradianceIndex = 0;
  ezConstantBufferStorageHandle m_hConstantBuffer;

  void BindResources(ezRenderContext* pRenderContext);
};

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
