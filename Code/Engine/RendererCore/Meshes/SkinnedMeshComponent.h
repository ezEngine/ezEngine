#pragma once

#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererFoundation/Shader/Types.h>
#include <memory>

class ezShaderTransform;

class EZ_RENDERERCORE_DLL ezSkinnedMeshRenderData : public ezMeshRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSkinnedMeshRenderData, ezMeshRenderData);

public:
  virtual void FillBatchIdAndSortingKey() override;
  ezGALBufferHandle m_hSkinningTransforms;
};

struct EZ_RENDERERCORE_DLL ezSkinningState
{
  ezSkinningState();
  ~ezSkinningState();

  void Clear();

  /// \brief Holds the current CPU-side copy of the skinning matrices. Modify these and call TransformsChanged() to send them to the GPU.
  ezDynamicArray<ezShaderTransform, ezAlignedAllocatorWrapper> m_Transforms;

  /// \brief Call this, after modifying m_Transforms, to make the renderer apply the update.
  void TransformsChanged();

  ezGALBufferHandle m_hGpuBuffer;
};
