#pragma once

#include <RendererCore/RendererCoreDLL.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Pipeline/Renderer.h>

struct ezTextSpriteData;
class ezRenderDataBatch;
typedef ezTypedResourceHandle<class ezShaderResource> ezShaderResourceHandle;

/// \brief Implements rendering of text sprites
class EZ_RENDERERCORE_DLL ezTextSpriteRenderer : public ezRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTextSpriteRenderer, ezRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezTextSpriteRenderer);

  public:
  ezTextSpriteRenderer();
    ~ezTextSpriteRenderer();

  // ezRenderer implementation
  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) const override;
  virtual void GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& categories) const override;
  virtual void RenderBatch(const ezRenderViewContext& renderContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const override;

protected:
  ezGALBufferHandle CreateTextSpriteDataBuffer() const;
  void DeleteTextSpriteDataBuffer(ezGALBufferHandle hBuffer) const;
  virtual void FillTextSpriteData(const ezRenderDataBatch& batch, ezUInt32 uiStartIndex, ezUInt32 uiCount) const;

  ezShaderResourceHandle m_hShader;
  mutable ezDynamicArray<ezTextSpriteData, ezAlignedAllocatorWrapper> m_TextSpriteData;
};
