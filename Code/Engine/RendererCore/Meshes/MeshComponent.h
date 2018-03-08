#pragma once

#include <Core/World/World.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderData.h>

struct ezMsgSetColor;

class EZ_RENDERERCORE_DLL ezMeshRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshRenderData, ezRenderData);

public:
  ezMeshResourceHandle m_hMesh;
  ezMaterialResourceHandle m_hMaterial;
  ezColor m_Color;
  ezGALBufferHandle m_hSkinningMatrices;
  ezArrayPtr<const ezUInt8> m_pNewSkinningMatricesData; // Optional - if set the buffer specified in m_hSkinningMatrices will be updated with this data

  ezUInt32 m_uiPartIndex : 30;
  ezUInt32 m_uiFlipWinding : 1;
  ezUInt32 m_uiUniformScale : 1;

  ezUInt32 m_uiUniqueID;

};

typedef ezComponentManager<class ezMeshComponent, ezBlockStorageType::Compact> ezMeshComponentManager;

struct EZ_RENDERERCORE_DLL ezMsgSetMeshMaterial : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgSetMeshMaterial, ezMessage);

  void SetMaterialFile(const char* szFile);
  const char* GetMaterialFile() const;

  ezMaterialResourceHandle m_hMaterial;
  ezUInt32 m_uiMaterialSlot = 0xFFFFFFFFu;

  virtual void Serialize(ezStreamWriter& stream) const override;
  virtual void Deserialize(ezStreamReader& stream, ezUInt8 uiTypeVersion) override;
};

class EZ_RENDERERCORE_DLL ezMeshComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezMeshComponent, ezRenderComponent, ezMeshComponentManager);

public:
  ezMeshComponent();
  ~ezMeshComponent();

  //////////////////////////////////////////////////////////////////////////
  // ezComponent Interface
public:

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;


  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent Interface

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;
  void OnExtractRenderData(ezMsgExtractRenderData& msg) const;

  //////////////////////////////////////////////////////////////////////////
  // ezMeshComponent Interface

public:

  void SetMesh(const ezMeshResourceHandle& hMesh);
  EZ_ALWAYS_INLINE const ezMeshResourceHandle& GetMesh() const { return m_hMesh; }

  void SetMaterial(ezUInt32 uiIndex, const ezMaterialResourceHandle& hMaterial);
  ezMaterialResourceHandle GetMaterial(ezUInt32 uiIndex) const;

  EZ_ALWAYS_INLINE void SetRenderDataCategory(ezRenderData::Category category) { m_RenderDataCategory = category; }

  void SetMeshFile(const char* szFile);
  const char* GetMeshFile() const;

  void SetColor(const ezColor& color);
  const ezColor& GetColor() const;

  void OnSetMaterial(ezMsgSetMeshMaterial& msg);
  void OnSetColor(ezMsgSetColor& msg);

protected:
  virtual ezMeshRenderData* CreateRenderData(ezUInt32 uiBatchId) const;

  ezGALBufferHandle m_hSkinningTransformsBuffer;
  ezArrayPtr<ezMat4> m_SkinningMatrices;

private:
  ezUInt32 Materials_GetCount() const;
  const char* Materials_GetValue(ezUInt32 uiIndex) const;
  void Materials_SetValue(ezUInt32 uiIndex, const char* value);
  void Materials_Insert(ezUInt32 uiIndex, const char* value);
  void Materials_Remove(ezUInt32 uiIndex);

  ezRenderData::Category m_RenderDataCategory;
  ezMeshResourceHandle m_hMesh;
  ezDynamicArray<ezMaterialResourceHandle> m_Materials;
  ezColor m_Color;
};

