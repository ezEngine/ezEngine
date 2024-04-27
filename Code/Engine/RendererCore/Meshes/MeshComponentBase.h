#pragma once

#include <Core/World/World.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderData.h>

struct ezMsgSetColor;
struct ezMsgSetCustomData;
struct ezInstanceData;

class EZ_RENDERERCORE_DLL ezMeshRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshRenderData, ezRenderData);

public:
  virtual void FillBatchIdAndSortingKey();

  ezMeshResourceHandle m_hMesh;
  ezMaterialResourceHandle m_hMaterial;
  ezColor m_Color = ezColor::White;
  ezVec4 m_vCustomData = ezVec4::MakeZero();

  ezUInt32 m_uiSubMeshIndex : 30;
  ezUInt32 m_uiFlipWinding : 1;
  ezUInt32 m_uiUniformScale : 1;

  ezUInt32 m_uiUniqueID = 0;

protected:
  EZ_FORCE_INLINE void FillBatchIdAndSortingKeyInternal(ezUInt32 uiAdditionalBatchData)
  {
    m_uiFlipWinding = m_GlobalTransform.ContainsNegativeScale() ? 1 : 0;
    m_uiUniformScale = m_GlobalTransform.ContainsUniformScale() ? 1 : 0;

    const ezUInt32 uiMeshIDHash = ezHashingUtils::StringHashTo32(m_hMesh.GetResourceIDHash());
    const ezUInt32 uiMaterialIDHash = m_hMaterial.IsValid() ? ezHashingUtils::StringHashTo32(m_hMaterial.GetResourceIDHash()) : 0;

    // Generate batch id from mesh, material and part index.
    ezUInt32 data[] = {uiMeshIDHash, uiMaterialIDHash, m_uiSubMeshIndex, m_uiFlipWinding, uiAdditionalBatchData};
    m_uiBatchId = ezHashingUtils::xxHash32(data, sizeof(data));

    // Sort by material and then by mesh
    m_uiSortingKey = (uiMaterialIDHash << 16) | ((uiMeshIDHash + m_uiSubMeshIndex) & 0xFFFE) | m_uiFlipWinding;
  }
};

/// \brief This message is used to replace the material on a mesh.
struct EZ_RENDERERCORE_DLL ezMsgSetMeshMaterial : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgSetMeshMaterial, ezMessage);

  void SetMaterialFile(const char* szFile);
  const char* GetMaterialFile() const;

  /// The material to be used.
  ezMaterialResourceHandle m_hMaterial;

  /// The slot on the mesh component where the material should be set.
  ezUInt32 m_uiMaterialSlot = 0xFFFFFFFFu;

  virtual void Serialize(ezStreamWriter& inout_stream) const override;
  virtual void Deserialize(ezStreamReader& inout_stream, ezUInt8 uiTypeVersion) override;
};

/// \brief Base class for components that render static or animated meshes.
class EZ_RENDERERCORE_DLL ezMeshComponentBase : public ezRenderComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezMeshComponentBase, ezRenderComponent);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // ezRenderMeshComponent

public:
  ezMeshComponentBase();
  ~ezMeshComponentBase();

  /// \brief Changes which mesh to render.
  void SetMesh(const ezMeshResourceHandle& hMesh);
  EZ_ALWAYS_INLINE const ezMeshResourceHandle& GetMesh() const { return m_hMesh; }

  /// \brief Sets the material that should be used for the sub-mesh with the given index.
  void SetMaterial(ezUInt32 uiIndex, const ezMaterialResourceHandle& hMaterial);
  ezMaterialResourceHandle GetMaterial(ezUInt32 uiIndex) const;

  void SetMeshFile(const char* szFile); // [ property ]
  const char* GetMeshFile() const;      // [ property ]

  /// \brief An additional tint color passed to the renderer to modify the mesh.
  void SetColor(const ezColor& color); // [ property ]
  const ezColor& GetColor() const;     // [ property ]

  /// \brief An additional vec4 passed to the renderer that can be used by custom material shaders for effects.
  void SetCustomData(const ezVec4& vData); // [ property ]
  const ezVec4& GetCustomData() const;     // [ property ]

  /// \brief The sorting depth offset allows to tweak the order in which this mesh is rendered relative to other meshes.
  ///
  /// This is mainly useful for transparent objects to render them before or after other meshes.
  void SetSortingDepthOffset(float fOffset);                // [ property ]
  float GetSortingDepthOffset() const;                      // [ property ]

  void OnMsgSetMeshMaterial(ezMsgSetMeshMaterial& ref_msg); // [ msg handler ]
  void OnMsgSetColor(ezMsgSetColor& ref_msg);               // [ msg handler ]
  void OnMsgSetCustomData(ezMsgSetCustomData& ref_msg);     // [ msg handler ]

protected:
  virtual ezMeshRenderData* CreateRenderData() const;

  ezUInt32 Materials_GetCount() const;                          // [ property ]
  const char* Materials_GetValue(ezUInt32 uiIndex) const;       // [ property ]
  void Materials_SetValue(ezUInt32 uiIndex, const char* value); // [ property ]
  void Materials_Insert(ezUInt32 uiIndex, const char* value);   // [ property ]
  void Materials_Remove(ezUInt32 uiIndex);                      // [ property ]

  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  ezMeshResourceHandle m_hMesh;
  ezDynamicArray<ezMaterialResourceHandle> m_Materials;
  ezColor m_Color = ezColor::White;
  ezVec4 m_vCustomData = ezVec4::MakeZero();
  float m_fSortingDepthOffset = 0.0f;
};
