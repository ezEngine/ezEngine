#pragma once

#include <RendererCore/Pipeline/Declarations.h>
#include <Core/ResourceManager/ResourceHandle.h>

struct SpriteData;
class ezRenderDataBatch;
typedef ezTypedResourceHandle<class ezShaderResource> ezShaderResourceHandle;

/// \brief Implements rendering of sprites
class EZ_RENDERERCORE_DLL ezSpriteRenderer : public ezRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSpriteRenderer, ezRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezSpriteRenderer);

public:

  ezSpriteRenderer();
  ~ezSpriteRenderer();

  // ezRenderer implementation
  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) override;
  virtual void RenderBatch(const ezRenderViewContext& renderContext, ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) override;

protected:
  ezGALBufferHandle CreateSpriteDataBuffer();
  void DeleteSpriteDataBuffer(ezGALBufferHandle hBuffer);
  virtual void FillSpriteData(const ezRenderDataBatch& batch, ezUInt32 uiStartIndex, ezUInt32 uiCount);

  ezShaderResourceHandle m_hShader;
  ezDynamicArray<SpriteData, ezAlignedAllocatorWrapper> m_spriteData;
};

