#pragma once

#include <RendererCore/Meshes/MeshRenderer.h>

/// \brief Implements rendering of skinned meshes
class EZ_RENDERERCORE_DLL ezSkinnedMeshRenderer : public ezMeshRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSkinnedMeshRenderer, ezMeshRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezSkinnedMeshRenderer);

public:
  ezSkinnedMeshRenderer();
  ~ezSkinnedMeshRenderer();

  // ezRenderer implementation
  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) const override;

protected:
  virtual void SetAdditionalData(const ezRenderViewContext& renderViewContext, const ezMeshRenderData* pRenderData) const override;
};
