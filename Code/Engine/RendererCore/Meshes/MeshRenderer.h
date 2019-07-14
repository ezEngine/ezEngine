#pragma once

#include <RendererCore/Pipeline/Renderer.h>

class ezMeshRenderData;
struct ezPerInstanceData;

/// \brief Implements rendering of static meshes
class EZ_RENDERERCORE_DLL ezMeshRenderer : public ezRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshRenderer, ezRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezMeshRenderer);

public:
  ezMeshRenderer();
  ~ezMeshRenderer();

  // ezRenderer implementation
  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) const override;
  virtual void GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& categories) const override;
  virtual void RenderBatch(
    const ezRenderViewContext& renderContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const override;

protected:
  virtual void SetAdditionalData(const ezRenderViewContext& renderViewContext, const ezMeshRenderData* pRenderData) const;
  virtual void FillPerInstanceData(
    ezArrayPtr<ezPerInstanceData> instanceData, const ezRenderDataBatch& batch, ezUInt32 uiStartIndex, ezUInt32& out_uiFilteredCount) const;
};
