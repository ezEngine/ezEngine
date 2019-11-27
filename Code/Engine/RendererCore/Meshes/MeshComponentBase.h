#pragma once

#include <Core/World/World.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderData.h>

struct ezMsgSetColor;
struct ezInstanceData;

class EZ_RENDERERCORE_DLL ezMeshRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshRenderData, ezRenderData);

public:
  virtual void FillBatchIdAndSortingKey();

  ezMeshResourceHandle m_hMesh;
  ezMaterialResourceHandle m_hMaterial;
  ezColor m_Color = ezColor::White;

  ezUInt32 m_uiSubMeshIndex : 30;
  ezUInt32 m_uiFlipWinding : 1;
  ezUInt32 m_uiUniformScale : 1;

  ezUInt32 m_uiUniqueID = 0;

protected:
  EZ_FORCE_INLINE void FillBatchIdAndSortingKeyInternal(ezUInt32 uiAdditionalBatchData)
  {
    m_uiFlipWinding = m_GlobalTransform.ContainsNegativeScale() ? 1 : 0;
    m_uiUniformScale = m_GlobalTransform.ContainsUniformScale() ? 1 : 0;

    const ezUInt32 uiMeshIDHash = m_hMesh.GetResourceIDHash();
    const ezUInt32 uiMaterialIDHash = m_hMaterial.IsValid() ? m_hMaterial.GetResourceIDHash() : 0;

    // Generate batch id from mesh, material and part index.
    ezUInt32 data[] = {uiMeshIDHash, uiMaterialIDHash, m_uiSubMeshIndex, m_uiFlipWinding, uiAdditionalBatchData};
    m_uiBatchId = ezHashingUtils::xxHash32(data, sizeof(data));

    // Sort by material and then by mesh
    m_uiSortingKey = (uiMaterialIDHash << 16) | (uiMeshIDHash & 0xFFFE) | m_uiFlipWinding;
  }
};

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

class EZ_RENDERERCORE_DLL ezMeshComponentBase : public ezRenderComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezMeshComponentBase, ezRenderComponent);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;

  //////////////////////////////////////////////////////////////////////////
  // ezRenderMeshComponent

public:
  ezMeshComponentBase();
  ~ezMeshComponentBase();

  void SetMesh(const ezMeshResourceHandle& hMesh);
  EZ_ALWAYS_INLINE const ezMeshResourceHandle& GetMesh() const { return m_hMesh; }

  void SetMaterial(ezUInt32 uiIndex, const ezMaterialResourceHandle& hMaterial);
  ezMaterialResourceHandle GetMaterial(ezUInt32 uiIndex) const;

  EZ_ALWAYS_INLINE void SetRenderDataCategory(ezRenderData::Category category) { m_RenderDataCategory = category; }

  void SetMeshFile(const char* szFile); // [ property ]
  const char* GetMeshFile() const;      // [ property ]

  void SetColor(const ezColor& color); // [ property ]
  const ezColor& GetColor() const;     // [ property ]

  void OnMsgSetMeshMaterial(ezMsgSetMeshMaterial& msg); // [ msg handler ]
  void OnMsgSetColor(ezMsgSetColor& msg);               // [ msg handler ]

protected:
  virtual ezMeshRenderData* CreateRenderData() const;

  ezUInt32 Materials_GetCount() const;                          // [ property ]
  const char* Materials_GetValue(ezUInt32 uiIndex) const;       // [ property ]
  void Materials_SetValue(ezUInt32 uiIndex, const char* value); // [ property ]
  void Materials_Insert(ezUInt32 uiIndex, const char* value);   // [ property ]
  void Materials_Remove(ezUInt32 uiIndex);                      // [ property ]

  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  ezRenderData::Category m_RenderDataCategory = ezInvalidRenderDataCategory;
  ezMeshResourceHandle m_hMesh;
  ezDynamicArray<ezMaterialResourceHandle> m_Materials;
  ezColor m_Color = ezColor::White;
};
