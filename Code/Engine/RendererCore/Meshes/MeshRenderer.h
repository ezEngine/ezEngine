#pragma once

#include <RendererCore/Pipeline/Declarations.h>

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
  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) override;
  virtual void RenderBatch(const ezRenderViewContext& renderContext, ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) override;

protected:
  virtual void FillPerInstanceData(ezArrayPtr<ezPerInstanceData> instanceData, const ezRenderDataBatch& batch, ezUInt32 uiStartIndex, ezUInt32& out_uiFilteredCount);
};

