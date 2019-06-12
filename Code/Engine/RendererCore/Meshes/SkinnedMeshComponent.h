#pragma once

#include <RendererCore/Meshes/MeshComponentBase.h>

class EZ_RENDERERCORE_DLL ezSkinnedMeshRenderData : public ezMeshRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSkinnedMeshRenderData, ezMeshRenderData);

public:
  virtual void FillBatchIdAndSortingKey();

  ezGALBufferHandle m_hSkinningMatrices;
  ezArrayPtr<const ezUInt8> m_pNewSkinningMatricesData;
};

//////////////////////////////////////////////////////////////////////////

class EZ_RENDERERCORE_DLL ezSkinnedMeshComponent : public ezMeshComponentBase
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezSkinnedMeshComponent, ezMeshComponentBase);

public:
  ezSkinnedMeshComponent();
  ~ezSkinnedMeshComponent();

  //////////////////////////////////////////////////////////////////////////
  // ezComponent Interface
  //
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  virtual void OnDeactivated() override;

protected:
  virtual ezMeshRenderData* CreateRenderData() const;

  void CreateSkinningTransformBuffer(ezArrayPtr<const ezMat4> skinningMatrices);
  void UpdateSkinningTransformBuffer(ezArrayPtr<const ezMat4> skinningMatrices);

private:
  ezGALBufferHandle m_hSkinningTransformsBuffer;
  ezArrayPtr<const ezMat4> m_SkinningMatrices;
};
