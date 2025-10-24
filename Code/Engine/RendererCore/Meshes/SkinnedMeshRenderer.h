#pragma once

#include <RendererCore/Meshes/MeshRenderer.h>

/// Implements rendering of skinned meshes.
///
/// Extends ezMeshRenderer to handle meshes with skeletal animation. Binds bone transformation
/// matrices as shader resources to enable GPU-based vertex skinning.
class EZ_RENDERERCORE_DLL ezSkinnedMeshRenderer : public ezMeshRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSkinnedMeshRenderer, ezMeshRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezSkinnedMeshRenderer);

public:
  ezSkinnedMeshRenderer();
  ~ezSkinnedMeshRenderer();

  // ezRenderer implementation
  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& ref_types) const override;

protected:
  /// Binds skeleton bone matrices for skinned mesh rendering.
  virtual void SetAdditionalData(const ezRenderViewContext& renderViewContext, const ezMeshRenderData* pRenderData) const override;
};
