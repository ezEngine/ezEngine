#pragma once

#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderData.h>

struct ezMsgSetColor;
struct ezMsgSetCustomData;

class EZ_RENDERERCORE_DLL ezMeshRenderData : public ezInstanceableRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshRenderData, ezInstanceableRenderData);

public:
  EZ_FORCE_INLINE void Fill(ezInstanceDataOffset instanceDataOffset, ezGALDynamicBufferHandle hInstanceDataBuffer, ezMaterialResourceHandle hMaterial, ezMeshResourceHandle hMesh, ezUInt32 uiMaterialSlotIndex = 0, ezUInt32 uiSubMeshIndex = 0, ezUInt32 uiNumInstances = 1)
  {
    m_uiNumInstances = uiNumInstances;
    m_DataOffsets.m_uiInstance = instanceDataOffset.m_uiOffset;
    m_DataOffsets.m_uiMaterial = uiMaterialSlotIndex << 24; // Encode the material slot index into the upper byte for picking purposes.
    m_hInstanceDataBuffer = hInstanceDataBuffer;

    m_hMaterial = hMaterial;
    m_hMesh = hMesh;
    m_uiSubMeshIndex = uiSubMeshIndex;

    FillSortingKey();
  }

  EZ_ALWAYS_INLINE void SetFallbackGlobalBounds(const ezBoundingBoxSphere& globalBounds)
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    if (globalBounds.IsValid())
    {
      m_FallbackGlobalBBox = globalBounds.GetBox();
    }
#endif
  }

  void FillSortingKey();
  virtual bool CanBatch(const ezRenderData& other) const override;

  ezMaterialResourceHandle m_hMaterial;
  ezMeshResourceHandle m_hMesh;
  ezUInt32 m_uiSubMeshIndex = 0;

  ezGALDynamicBufferHandle m_hCustomInstanceDataBuffer;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezBoundingBox m_FallbackGlobalBBox = ezBoundingBox::MakeInvalid();
#endif
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
  virtual void OnDeactivated() override;

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
  void SetColor(const ezColor& color);                // [ property ]
  const ezColor& GetColor() const { return m_Color; } // [ property ]

  /// \brief An additional vec4 passed to the renderer that can be used by custom material shaders for effects.
  void SetCustomData(const ezVec4& vData);                      // [ property ]
  const ezVec4& GetCustomData() const { return m_vCustomData; } // [ property ]

  /// \brief The sorting depth offset allows to tweak the order in which this mesh is rendered relative to other meshes.
  ///
  /// This is mainly useful for transparent objects to render them before or after other meshes.
  void SetSortingDepthOffset(float fOffset);                            // [ property ]
  float GetSortingDepthOffset() const { return m_fSortingDepthOffset; } // [ property ]

  void OnMsgSetMeshMaterial(ezMsgSetMeshMaterial& ref_msg);             // [ msg handler ]
  void OnMsgSetColor(ezMsgSetColor& ref_msg);                           // [ msg handler ]
  void OnMsgSetCustomData(ezMsgSetCustomData& ref_msg);                 // [ msg handler ]

  /// \brief Set custom instance data for this mesh component which can be used by custom material shaders when
  /// the simple custom data vector is not sufficient.
  ///
  /// Typically another component besides this mesh component would create and manage the buffer that holds the custom instance data.
  /// See ezRenderDataManager how to create and fill such a buffer.
  /// The renderer will bind the buffer to the 'perInstanceDataCustom' shader resource slot, so add something like this to your shader code:
  /// StructuredBuffer<MyCustomDataStruct> perInstanceDataCustom BIND_GROUP(BG_DRAW_CALL);
  /// and access the data with the corresponding data offset:
  /// perInstanceDataCustom[G.Input.DataOffsets.y]
  void SetCustomInstanceData(ezCustomInstanceDataOffset offset, ezGALDynamicBufferHandle hBuffer);
  ezCustomInstanceDataOffset GetCustomInstanceDataOffset() const { return m_CustomInstanceDataOffset; }
  ezGALDynamicBufferHandle GetCustomInstanceDataBuffer() const { return m_hCustomInstanceDataBuffer; }

protected:
  virtual ezTransform GetFinalGlobalTransform() const;
  virtual ezMeshRenderData* CreateRenderData(const ezRenderDataManager* pRenderDataManager) const;

  // TODO: Using ezStringView for the array accessors doesn't work (currently)

  ezUInt32 Materials_GetCount() const;                        // [ property ]
  ezString Materials_GetValue(ezUInt32 uiIndex) const;        // [ property ]
  void Materials_SetValue(ezUInt32 uiIndex, ezString sValue); // [ property ]
  void Materials_Insert(ezUInt32 uiIndex, ezString sValue);   // [ property ]
  void Materials_Remove(ezUInt32 uiIndex);                    // [ property ]

  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;
  void DeleteInstanceData();

  ezMeshResourceHandle m_hMesh;
  ezSmallArray<ezMaterialResourceHandle, 2> m_Materials;
  ezColor m_Color = ezColor::White;
  ezVec4 m_vCustomData = ezVec4(0, 1, 0, 1);
  float m_fSortingDepthOffset = 0.0f;

  mutable ezInstanceDataOffset m_InstanceDataOffset;

  ezCustomInstanceDataOffset m_CustomInstanceDataOffset;
  ezGALDynamicBufferHandle m_hCustomInstanceDataBuffer;
};
