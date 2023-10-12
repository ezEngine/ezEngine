#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Pipeline/Renderer.h>

struct ezPerLensFlareData;
class ezRenderDataBatch;
using ezShaderResourceHandle = ezTypedResourceHandle<class ezShaderResource>;

/// \brief Implements rendering of lens flares
class EZ_RENDERERCORE_DLL ezLensFlareRenderer : public ezRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLensFlareRenderer, ezRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezLensFlareRenderer);

public:
  ezLensFlareRenderer();
  ~ezLensFlareRenderer();

  // ezRenderer implementation
  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& ref_types) const override;
  virtual void GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& ref_categories) const override;
  virtual void RenderBatch(
    const ezRenderViewContext& renderContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const override;

protected:
  ezGALBufferHandle CreateLensFlareDataBuffer(ezUInt32 uiBufferSize) const;
  void DeleteLensFlareDataBuffer(ezGALBufferHandle hBuffer) const;
  virtual void FillLensFlareData(const ezRenderDataBatch& batch) const;

  ezShaderResourceHandle m_hShader;
  mutable ezDynamicArray<ezPerLensFlareData, ezAlignedAllocatorWrapper> m_LensFlareData;
};
