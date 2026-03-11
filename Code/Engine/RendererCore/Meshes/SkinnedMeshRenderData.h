#pragma once

#include <RendererCore/Meshes/MeshComponentBase.h>

class ezShaderTransform;

/// \brief Render data for skinned meshes.
///
/// Extends mesh render data with a GPU buffer handle for bone transformation matrices.
class EZ_RENDERERCORE_DLL ezSkinnedMeshRenderData : public ezMeshRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSkinnedMeshRenderData, ezMeshRenderData);

public:
  virtual bool CanBatch(const ezRenderData& other) const override;

  ezGALDynamicBufferHandle m_hSkinningBuffer;
};

/// \brief Manages the skinning state for an animated mesh.
///
/// Wraps around the ezRenderDataManager functions to manage skinning data for skinned meshes.
struct EZ_RENDERERCORE_DLL ezSkinningState
{
  ezSkinningState();
  ~ezSkinningState();

  void Clear();

  /// \brief Returns a writable array of bone transforms, allocating or reallocating the buffer as needed. Note that existing data is lost on reallocation.
  ezArrayPtr<ezShaderTransform> GetOrCreateBoneTransformsForWriting(ezComponent& ref_ownerComponent, ezUInt32 uiNumBones);

  ezArrayPtr<const ezShaderTransform> GetBoneTransformsForReading() const;

  bool HasBoneTransforms() const { return m_uiNumBones > 0; }

  ezCustomInstanceDataOffset m_DataOffset;
  ezUInt32 m_uiNumBones = 0;
  ezWorld* m_pWorld = nullptr;
};
