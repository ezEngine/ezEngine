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
  void FillSortingKey();
  virtual bool CanBatch(const ezRenderData& other) const override;

  ezMeshResourceHandle m_hMesh;
  ezMaterialResourceHandle m_hMaterial;
  ezColor m_Color = ezColor::White;
  ezVec4 m_vCustomData = ezVec4(0, 1, 0, 1);

  ezUInt32 m_uiSubMeshIndex : 30;
  ezUInt32 m_uiFlipWinding : 1;
  ezUInt32 m_uiUniformScale : 1;

  ezUInt32 m_uiUniqueID = 0;
};

/// \brief This message is used to replace the material on a mesh.
struct EZ_RENDERERCORE_DLL ezMsgSetMeshMaterial : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgSetMeshMaterial, ezMessage);

  // adds SetMaterialFile() and GetMaterialFile() for convenience
  EZ_ADD_RESOURCEHANDLE_ACCESSORS(Material, m_hMaterial);

  /// The material to be used.
  ezMaterialResourceHandle m_hMaterial; // [ property ]

  /// The slot on the mesh component where the material should be set.
  ezUInt32 m_uiMaterialSlot = 0; // [ property ]

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
  void SetMesh(const ezMeshResourceHandle& hMesh);                                 // [ property ]
  EZ_ALWAYS_INLINE const ezMeshResourceHandle& GetMesh() const { return m_hMesh; } // [ property ]

  // adds SetMeshFile() and GetMeshFile() for convenience
  EZ_ADD_RESOURCEHANDLE_ACCESSORS_WITH_SETTER(Mesh, m_hMesh, SetMesh);

  /// \brief Sets the material that should be used for the sub-mesh with the given index.
  void SetMaterial(ezUInt32 uiIndex, const ezMaterialResourceHandle& hMaterial); // [ property ]
  ezMaterialResourceHandle GetMaterial(ezUInt32 uiIndex) const;                  // [ property ]

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

  // TODO: Using ezStringView for the array accessors doesn't work (currently)

  ezUInt32 Materials_GetCount() const;                        // [ property ]
  ezString Materials_GetValue(ezUInt32 uiIndex) const;        // [ property ]
  void Materials_SetValue(ezUInt32 uiIndex, ezString sValue); // [ property ]
  void Materials_Insert(ezUInt32 uiIndex, ezString sValue);   // [ property ]
  void Materials_Remove(ezUInt32 uiIndex);                    // [ property ]

  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  ezMeshResourceHandle m_hMesh;
  ezDynamicArray<ezMaterialResourceHandle> m_Materials;
  ezColor m_Color = ezColor::White;
  ezVec4 m_vCustomData = ezVec4(0, 1, 0, 1);
  float m_fSortingDepthOffset = 0.0f;
};
