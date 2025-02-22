#pragma once

#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Pipeline/Renderer.h>

using ezDynamicMeshBufferResourceHandle = ezTypedResourceHandle<class ezDynamicMeshBufferResource>;
using ezCustomMeshComponentManager = ezComponentManager<class ezCustomMeshComponent, ezBlockStorageType::Compact>;

/// \brief This component is used to render custom geometry.
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

  /// \brief Creates a new dynamic mesh buffer.
  ///
  /// The new buffer can hold the given number of vertices and indices (either 16 bit or 32 bit).
  ezDynamicMeshBufferResourceHandle CreateMeshResource(ezGALPrimitiveTopology::Enum topology, ezUInt32 uiMaxVertices, ezUInt32 uiMaxPrimitives, ezGALIndexType::Enum indexType);

  /// \brief Returns the currently set mesh resource.
  ezDynamicMeshBufferResourceHandle GetMeshResource() const { return m_hDynamicMesh; }

  /// \brief Sets which mesh buffer to use.
  ///
  /// This can be used to have multiple ezCustomMeshComponent's reference the same mesh buffer,
  /// such that the object gets instanced in different locations.
  void SetMeshResource(const ezDynamicMeshBufferResourceHandle& hMesh);

  /// \brief Configures the component to render only a subset of the primitives in the mesh buffer.
  void SetUsePrimitiveRange(ezUInt32 uiFirstPrimitive = 0, ezUInt32 uiNumPrimitives = ezMath::MaxValue<ezUInt32>());

  /// \brief Sets the bounds that are used for culling.
  ///
  /// Note: It is very important that this is called whenever the mesh buffer is modified and the size of
  /// the mesh has changed, otherwise the object might not appear or be culled incorrectly.
  void SetBounds(const ezBoundingBoxSphere& bounds);

  /// \brief Sets the material for rendering.
  void SetMaterial(const ezMaterialResourceHandle& hMaterial);

  /// \brief Returns the material that is used for rendering.
  ezMaterialResourceHandle GetMaterial() const;

  // adds SetMaterialFile() and GetMaterialFile() for convenience
  EZ_ADD_RESOURCEHANDLE_ACCESSORS(Material, m_hMaterial);

  /// \brief Sets the mesh instance color.
  void SetColor(const ezColor& color); // [ property ]

  /// \brief Returns the mesh instance color.
  const ezColor& GetColor() const; // [ property ]

  /// \brief An additional vec4 passed to the renderer that can be used by custom material shaders for effects.
  void SetCustomData(const ezVec4& vData);                  // [ property ]
  const ezVec4& GetCustomData() const;                      // [ property ]

  void OnMsgSetMeshMaterial(ezMsgSetMeshMaterial& ref_msg); // [ msg handler ]
  void OnMsgSetColor(ezMsgSetColor& ref_msg);               // [ msg handler ]
  void OnMsgSetCustomData(ezMsgSetCustomData& ref_msg);     // [ msg handler ]

protected:
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  ezMaterialResourceHandle m_hMaterial; // [ property ]
  ezColor m_Color = ezColor::White;
  ezVec4 m_vCustomData = ezVec4(0, 1, 0, 1);
  ezUInt32 m_uiFirstPrimitive = 0;
  ezUInt32 m_uiNumPrimitives = 0xFFFFFFFF;
  ezBoundingBoxSphere m_Bounds;

  ezDynamicMeshBufferResourceHandle m_hDynamicMesh;

  virtual void OnActivated() override;
};

/// \brief Temporary data used to feed the ezCustomMeshRenderer.
class EZ_RENDERERCORE_DLL ezCustomMeshRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCustomMeshRenderData, ezRenderData);

public:
  void FillSortingKey();
  virtual bool CanBatch(const ezRenderData& other) const override;

  ezDynamicMeshBufferResourceHandle m_hMesh;
  ezMaterialResourceHandle m_hMaterial;
  ezColor m_Color = ezColor::White;
  ezVec4 m_vCustomData = ezVec4(0, 1, 0, 1);

  ezUInt32 m_uiFlipWinding : 1;
  ezUInt32 m_uiUniformScale : 1;

  ezUInt32 m_uiFirstPrimitive = 0;
  ezUInt32 m_uiNumPrimitives = 0xFFFFFFFF;

  ezUInt32 m_uiUniqueID = 0;
};

/// \brief A renderer that handles all ezCustomMeshRenderData.
class EZ_RENDERERCORE_DLL ezCustomMeshRenderer : public ezRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCustomMeshRenderer, ezRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezCustomMeshRenderer);

public:
  ezCustomMeshRenderer();
  ~ezCustomMeshRenderer();

  virtual void GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& ref_categories) const override;
  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& ref_types) const override;
  virtual void RenderBatch(const ezRenderViewContext& renderContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const override;
};
