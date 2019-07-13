#pragma once

#include <RendererCore/Meshes/MeshRenderer.h>

/// \brief Implements rendering of skinned meshes
class EZ_PROCGENPLUGIN_DLL ezProcVertexColorRenderer : public ezMeshRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProcVertexColorRenderer, ezMeshRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezProcVertexColorRenderer);

public:
  ezProcVertexColorRenderer();
  ~ezProcVertexColorRenderer();

  // ezRenderer implementation
  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) const override;

protected:
  virtual void SetAdditionalData(const ezRenderViewContext& renderViewContext, const ezMeshRenderData* pRenderData) const override;
  virtual void FillPerInstanceData(ezArrayPtr<ezPerInstanceData> instanceData, const ezRenderDataBatch& batch, ezUInt32 uiStartIndex,
    ezUInt32& out_uiFilteredCount) const override;
};
