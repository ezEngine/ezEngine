#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/FrameDataProvider.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>

struct ezPerInstanceData;

struct ezInstanceData
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezInstanceData);
public:

  ezInstanceData();
  ~ezInstanceData();

  ezGALBufferHandle m_hInstanceDataBuffer;

  ezConstantBufferStorageHandle m_hConstantBuffer;

  void BindResources(ezRenderContext* pRenderContext);

  ezArrayPtr<ezPerInstanceData> GetInstanceData(ezUInt32 uiCount, ezUInt32& out_uiOffset);
  void UpdateInstanceData(ezRenderContext* pRenderContext, ezUInt32 uiCount);

private:
  friend class ezInstanceDataProvider;

  void CreateBuffer(ezUInt32 uiSize);
  void Reset();

  ezUInt32 m_uiBufferSize;
  ezUInt32 m_uiBufferOffset;
  ezDynamicArray<ezPerInstanceData, ezAlignedAllocatorWrapper> m_perInstanceData;
};

class EZ_RENDERERCORE_DLL ezInstanceDataProvider : public ezFrameDataProvider<ezInstanceData>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezInstanceDataProvider, ezFrameDataProviderBase);

public:
  ezInstanceDataProvider();
  ~ezInstanceDataProvider();

private:

  virtual void* UpdateData(const ezRenderViewContext& renderViewContext, const ezExtractedRenderData& extractedData) override;

  ezInstanceData m_Data;
};

