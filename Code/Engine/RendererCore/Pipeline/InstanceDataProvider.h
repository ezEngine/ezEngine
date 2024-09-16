#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/FrameDataProvider.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>

struct ezPerInstanceData;
class ezInstanceDataProvider;
class ezInstancedMeshComponent;

/// \brief Stores a couple of ezPerInstanceData structs inside a buffer. Used to do instanced rendering.
struct EZ_RENDERERCORE_DLL ezInstanceData
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezInstanceData);

public:
  ezInstanceData(ezUInt32 uiMaxInstanceCount = 1024);
  ~ezInstanceData();

  ezGALBufferHandle m_hInstanceDataBuffer;

  ezConstantBufferStorageHandle m_hConstantBuffer;

  void BindResources(ezRenderContext* pRenderContext);

  ezArrayPtr<ezPerInstanceData> GetInstanceData(ezUInt32 uiCount, ezUInt32& out_uiOffset);
  void UpdateInstanceData(ezRenderContext* pRenderContext, ezUInt32 uiCount);

private:
  friend ezInstanceDataProvider;
  friend ezInstancedMeshComponent;

  void CreateBuffer(ezUInt32 uiSize);
  void Reset();

  ezUInt32 m_uiBufferSize = 0;
  ezUInt32 m_uiBufferOffset = 0;
  ezDynamicArray<ezPerInstanceData, ezAlignedAllocatorWrapper> m_PerInstanceData;
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
