#pragma once

#include <RendererCore/Pipeline/Declarations.h>
#include <Core/ResourceManager/ResourceHandle.h>

struct PerInstanceData;

class EZ_RENDERERCORE_DLL ezMeshRenderer : public ezRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshRenderer, ezRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezMeshRenderer);

public:

  ezMeshRenderer();
  ~ezMeshRenderer();

  // ezRenderer implementation
  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) override;
  virtual void RenderBatch(const ezRenderViewContext& renderContext, ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) override;

protected:
  ezGALBufferHandle CreateInstanceDataBuffer(bool bUseInstancing);
  void DeleteInstanceDataBuffer(ezGALBufferHandle hBuffer);
  virtual void FillPerInstanceData(const ezRenderDataBatch& batch, ezUInt32 uiStartIndex, ezUInt32 uiCount);

  ezInt32 m_iInstancingThreshold;
  ezDynamicArray<PerInstanceData, ezAlignedAllocatorWrapper> m_perInstanceData;
};

