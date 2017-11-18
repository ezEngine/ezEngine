#pragma once

#include <Core/Messages/ScriptFunctionMessage.h>
#include <Core/World/World.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderData.h>

class EZ_RENDERERCORE_DLL ezMeshRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshRenderData, ezRenderData);

public:
  ezMeshResourceHandle m_hMesh;
  ezMaterialResourceHandle m_hMaterial;

  ezUInt32 m_uiPartIndex : 30;
  ezUInt32 m_uiFlipWinding : 1;
  ezUInt32 m_uiUniformScale : 1;

  ezUInt32 m_uiUniqueID;
};

typedef ezComponentManager<class ezMeshComponent, ezBlockStorageType::Compact> ezMeshComponentManager;

struct ezMeshComponent_SetMaterialMsg : public ezScriptFunctionMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMeshComponent_SetMaterialMsg, ezScriptFunctionMessage);

  void SetMaterialFile(const char* szFile);
  const char* GetMaterialFile() const;

  ezMaterialResourceHandle m_hMaterial;
  ezUInt32 m_uiMaterialSlot;
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
  void OnExtractRenderData(ezExtractRenderDataMessage& msg) const;

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

  void OnSetMaterialMsg(ezMeshComponent_SetMaterialMsg& msg);

protected:
  virtual ezMeshRenderData* CreateRenderData(ezUInt32 uiBatchId) const;

private:
  ezUInt32 Materials_GetCount() const;
  const char* Materials_GetValue(ezUInt32 uiIndex) const;
  void Materials_SetValue(ezUInt32 uiIndex, const char* value);
  void Materials_Insert(ezUInt32 uiIndex, const char* value);
  void Materials_Remove(ezUInt32 uiIndex);

  ezRenderData::Category m_RenderDataCategory;
  ezMeshResourceHandle m_hMesh;
  ezDynamicArray<ezMaterialResourceHandle> m_Materials;
};

