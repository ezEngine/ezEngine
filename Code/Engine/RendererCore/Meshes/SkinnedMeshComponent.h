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
  ezArrayPtr<const ezUInt8> m_pNewSkinningTransformData;
  std::shared_ptr<bool> m_bTransformsUpdated;
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

  void FillSkinnedMeshRenderData(ezSkinnedMeshRenderData& ref_renderData) const;

private:
  ezGALBufferHandle m_hGpuBuffer;
  std::shared_ptr<bool> m_bTransformsUpdated[2];
};
