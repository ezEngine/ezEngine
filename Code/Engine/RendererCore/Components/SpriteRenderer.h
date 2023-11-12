#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Pipeline/Renderer.h>

struct ezPerSpriteData;
class ezRenderDataBatch;
using ezShaderResourceHandle = ezTypedResourceHandle<class ezShaderResource>;

/// \brief Implements rendering of sprites
class EZ_RENDERERCORE_DLL ezSpriteRenderer : public ezRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSpriteRenderer, ezRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezSpriteRenderer);

public:
  ezSpriteRenderer();
  ~ezSpriteRenderer();

  // ezRenderer implementation
  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& ref_types) const override;
  virtual void GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& ref_categories) const override;
  virtual void RenderBatch(
    const ezRenderViewContext& renderContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const override;

protected:
  ezGALBufferHandle CreateSpriteDataBuffer(ezUInt32 uiBufferSize) const;
  void DeleteSpriteDataBuffer(ezGALBufferHandle hBuffer) const;
  virtual void FillSpriteData(const ezRenderDataBatch& batch) const;

  ezShaderResourceHandle m_hShader;
  mutable ezDynamicArray<ezPerSpriteData, ezAlignedAllocatorWrapper> m_SpriteData;
};
