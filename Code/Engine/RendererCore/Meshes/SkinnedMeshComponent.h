#pragma once

#include <RendererCore/Meshes/MeshComponentBase.h>

class ezShaderTransform;

class EZ_RENDERERCORE_DLL ezSkinnedMeshRenderData : public ezMeshRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSkinnedMeshRenderData, ezMeshRenderData);

public:
  virtual void FillBatchIdAndSortingKey() override;

  ezGALBufferHandle m_hSkinningTransforms;
  ezArrayPtr<const ezUInt8> m_pNewSkinningTransformData;
};

//////////////////////////////////////////////////////////////////////////

class EZ_RENDERERCORE_DLL ezSkinnedMeshComponent : public ezMeshComponentBase
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezSkinnedMeshComponent, ezMeshComponentBase);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezMeshComponentBase

protected:
  virtual ezMeshRenderData* CreateRenderData() const override;


  //////////////////////////////////////////////////////////////////////////
  // ezSkinnedMeshComponent

public:
  ezSkinnedMeshComponent();
  ~ezSkinnedMeshComponent();

protected:
  /// \brief Makes a temporary copy of the provided skinning data for later upload to the GPU.
  void UpdateSkinningTransformBuffer(ezArrayPtr<const ezShaderTransform> skinningTransforms);

private:
  ezGALBufferHandle m_hSkinningTransformsBuffer;
  mutable ezArrayPtr<const ezShaderTransform> m_SkinningTransforms;
};
