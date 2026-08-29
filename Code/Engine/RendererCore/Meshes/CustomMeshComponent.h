#pragma once

#include <RendererCore/Meshes/MeshComponentBase.h>

/// Render data used to feed the ezMeshRenderer.
class EZ_RENDERERCORE_DLL ezCustomMeshRenderData : public ezInstanceableRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCustomMeshRenderData, ezInstanceableRenderData);

public:
  void FillSortingKey();
  virtual bool CanBatch(const ezRenderData& other) const override;

  ezMaterialResourceHandle m_hMaterial;
  ezDynamicMeshBufferResourceHandle m_hDynamicMeshBuffer;

  ezUInt32 m_uiFirstPrimitive = 0;
  ezUInt32 m_uiNumPrimitives = 0xFFFFFFFF;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezBoundingBox m_FallbackGlobalBBox = ezBoundingBox::MakeInvalid();
#endif
};

using ezDynamicMeshBufferResourceHandle = ezTypedResourceHandle<class ezDynamicMeshBufferResource>;
using ezCustomMeshComponentManager = ezComponentManager<class ezCustomMeshComponent, ezBlockStorageType::Compact>;

/// This component is used to render custom geometry.
///
/// Sometimes game code needs to build geometry on the fly to visualize dynamic things.
/// The ezDynamicMeshBufferResource is an easy to use resource to build geometry and change it frequently.
/// This component takes such a resource and takes care of rendering it.
/// The same resource can be set on multiple components to instantiate it in different locations.
class EZ_RENDERERCORE_DLL ezCustomMeshComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezCustomMeshComponent, ezRenderComponent, ezCustomMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // ezCustomMeshComponent

public:
  ezCustomMeshComponent();
  ~ezCustomMeshComponent();

  /// Creates a new dynamic mesh buffer.
  ///
  /// The new buffer can hold the given number of vertices and indices (either 16 bit or 32 bit).
  ezDynamicMeshBufferResourceHandle CreateMeshResource(ezGALPrimitiveTopology::Enum topology, ezUInt32 uiMaxVertices, ezUInt32 uiMaxPrimitives, ezGALIndexType::Enum indexType);

  /// Returns the currently set mesh resource.
  ezDynamicMeshBufferResourceHandle GetMeshResource() const { return m_hDynamicMesh; }

  /// Sets which mesh buffer to use.
  ///
  /// This can be used to have multiple ezCustomMeshComponent's reference the same mesh buffer,
  /// such that the object gets instanced in different locations.
  void SetMeshResource(const ezDynamicMeshBufferResourceHandle& hMesh);

  /// Configures the component to render only a subset of the primitives in the mesh buffer.
  void SetUsePrimitiveRange(ezUInt32 uiFirstPrimitive = 0, ezUInt32 uiNumPrimitives = ezMath::MaxValue<ezUInt32>());

  /// Sets the bounds that are used for culling.
  ///
  /// Note: It is very important that this is called whenever the mesh buffer is modified and the size of
  /// the mesh has changed, otherwise the object might not appear or be culled incorrectly.
  void SetBounds(const ezBoundingBoxSphere& bounds);

  /// Sets the material for rendering.
  void SetMaterial(const ezMaterialResourceHandle& hMaterial);

  /// Returns the material that is used for rendering.
  ezMaterialResourceHandle GetMaterial() const;

  // adds SetMaterialFile() and GetMaterialFile() for convenience
  EZ_ADD_RESOURCEHANDLE_ACCESSORS(Material, m_hMaterial);

  /// Sets the mesh instance color.
  void SetColor(const ezColor& color); // [ property ]

  /// Returns the mesh instance color.
  const ezColor& GetColor() const; // [ property ]

  /// An additional vec4 passed to the renderer that can be used by custom material shaders for effects.
  void SetCustomData(const ezVec4& vData); // [ property ]
  const ezVec4& GetCustomData() const;     // [ property ]

  /// Sets the sorting depth offset value.
  ///
  /// The effect of sorting depth depends on the RenderDataCategory that the mesh is rendered in.
  /// E.g. if the sorting function of the render data category is ezRenderSortingFunctions::ByDepthOffsetOnly
  /// the offset is the *only* thing that decides the order, which makes it a way to give overlapping transparent meshes a fixed draw order.
  /// With the typical distance based sorting functions this has little impact.
  void SetSortingDepthOffset(float fOffset);                            // [ property ]
  float GetSortingDepthOffset() const { return m_fSortingDepthOffset; } // [ property ]

  void OnMsgSetMeshMaterial(ezMsgSetMeshMaterial& ref_msg);             // [ msg handler ]
  void OnMsgSetColor(ezMsgSetColor& ref_msg);                           // [ msg handler ]
  void OnMsgSetCustomData(ezMsgSetCustomData& ref_msg);                 // [ msg handler ]

protected:
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  ezMaterialResourceHandle m_hMaterial; // [ property ]
  ezColor m_Color = ezColor::White;
  ezVec4 m_vCustomData = ezVec4(0, 1, 0, 1);
  float m_fSortingDepthOffset = 0.0f;
  ezBoundingBoxSphere m_Bounds = ezBoundingBoxSphere::MakeInvalid();

  mutable ezInstanceDataOffset m_InstanceDataOffset;

  ezUInt32 m_uiFirstPrimitive = 0;
  ezUInt32 m_uiNumPrimitives = 0xFFFFFFFF;
  ezDynamicMeshBufferResourceHandle m_hDynamicMesh;
};
