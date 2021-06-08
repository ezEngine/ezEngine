#pragma once

#include <RendererCore/Meshes/MeshComponentBase.h>

class EZ_RENDERERCORE_DLL ezSkinnedMeshRenderData : public ezMeshRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSkinnedMeshRenderData, ezMeshRenderData);

public:
  virtual void FillBatchIdAndSortingKey() override;

  ezGALBufferHandle m_hSkinningMatrices;
  ezArrayPtr<const ezUInt8> m_pNewSkinningMatricesData;
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
  void CreateSkinningTransformBuffer(ezArrayPtr<const ezMat4> skinningMatrices);
  void UpdateSkinningTransformBuffer(ezArrayPtr<const ezMat4> skinningMatrices);

  ezGALBufferHandle m_hSkinningTransformsBuffer;
  mutable ezArrayPtr<const ezMat4> m_SkinningMatrices;
};
