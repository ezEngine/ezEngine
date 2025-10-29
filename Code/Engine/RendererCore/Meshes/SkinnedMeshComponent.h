#pragma once

#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererFoundation/Shader/Types.h>
#include <memory>

class ezShaderTransform;

/// Render data for skinned meshes.
///
/// Extends mesh render data with a GPU buffer handle for bone transformation matrices.
class EZ_RENDERERCORE_DLL ezSkinnedMeshRenderData : public ezMeshRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSkinnedMeshRenderData, ezMeshRenderData);

public:
  virtual bool CanBatch(const ezRenderData& other) const override { return false; }

  ezGALBufferHandle m_hSkinningTransforms; ///< GPU buffer containing bone transformation matrices.
};

/// Manages the skinning state for an animated mesh.
///
/// Maintains both CPU and GPU copies of bone transformation matrices.
/// The CPU copy can be modified and uploaded to the GPU for rendering.
struct EZ_RENDERERCORE_DLL ezSkinningState
{
  ezSkinningState();
  ~ezSkinningState();

  void Clear();

  /// Holds the current CPU-side copy of the skinning matrices.
  ///
  /// Modify these and call TransformsChanged() to send them to the GPU.
  ezDynamicArray<ezShaderTransform, ezAlignedAllocatorWrapper> m_Transforms;

  /// Call this after modifying m_Transforms to upload changes to the GPU.
  void TransformsChanged();

  ezGALBufferHandle m_hGpuBuffer; ///< GPU buffer containing the skinning matrices.
};
