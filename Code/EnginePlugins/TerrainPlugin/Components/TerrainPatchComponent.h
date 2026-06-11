#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Types/TagSet.h>
#include <GameEngine/Utils/ImageDataResource.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <TerrainPlugin/TerrainPluginDLL.h>
#include <TerrainPlugin/TerrainSystem.h>

struct ezMsgExtractRenderData;
struct ezMsgTransformChanged;
struct ezResourceEvent;
class ezAbstractObjectNode;

using ezMaterialResourceHandle = ezTypedResourceHandle<class ezMaterialResource>;

class EZ_TERRAINPLUGIN_DLL ezTerrainPatchComponentManager : public ezComponentManager<class ezTerrainPatchComponent, ezBlockStorageType::Compact>
{
  using SUPER = ezComponentManager<ezTerrainPatchComponent, ezBlockStorageType::Compact>;

public:
  ezTerrainPatchComponentManager(ezWorld* pWorld);
  ~ezTerrainPatchComponentManager();

  virtual void Initialize() override;

private:
  void Update(const ezWorldModule::UpdateContext& context);
  void ResourceEventHandler(const ezResourceEvent& e);
};

/// Renders a single terrain patch as a procedural grid.
///
/// The vertex shader generates grid geometry from SV_VertexID; no vertex buffer is needed.
/// Heights are stored in a GPU structured buffer baked by a compute shader via ezTerrainSystem.
class EZ_TERRAINPLUGIN_DLL ezTerrainPatchComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezTerrainPatchComponent, ezRenderComponent, ezTerrainPatchComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

protected:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg) override;
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  void OnMsgTransformChanged(ezMsgTransformChanged& msg);

  //////////////////////////////////////////////////////////////////////////
  // ezTerrainPatchComponent

public:
  ezTerrainPatchComponent();
  ~ezTerrainPatchComponent();

  void SetResolution(ezEnum<ezTerrainResolution> resolution);
  ezEnum<ezTerrainResolution> GetResolution() const { return m_Resolution; }

  void SetSize(float fSize);
  float GetSize() const { return m_fSize; }

  void SetMaterial(const ezMaterialResourceHandle& hMaterial);                       // [ property ]
  const ezMaterialResourceHandle& GetMaterial() const { return m_hMaterial; }        // [ property ]

  void SetHeightImage(const ezImageDataResourceHandle& hImage);                      // [ property ]
  const ezImageDataResourceHandle& GetHeightImage() const { return m_hHeightImage; } // [ property ]

  /// UV offset into the height image; selects the top-left corner of the sampled rectangle.
  ezVec2 GetHeightImageOffset() const { return m_vImageOffset; } // [ property ]
  void SetHeightImageOffset(ezVec2 vOffset);                     // [ property ]

  /// UV size of the rectangle sampled from the height image. Values < 1 select a sub-region.
  ezVec2 GetHeightImageSize() const { return m_vImageSize; } // [ property ]
  void SetHeightImageSize(ezVec2 vSize);                     // [ property ]

  /// Multiplier applied to the [0, 1] greyscale sample to produce a world-space height.
  float GetHeightImageScale() const { return m_fHeightScale; }                      // [ property ]
  void SetHeightImageScale(float fScale);                                           // [ property ]

  void SetCollider(ezEnum<ezTerrainPatchColliderMode> mode);                        // [ property ]
  ezEnum<ezTerrainPatchColliderMode> GetCollider() const { return m_ColliderMode; } // [ property ]

  ezUInt8 GetBaseMaterialIndex() const { return m_uiBaseMaterialIndex; }            // [ property ]
  void SetBaseMaterialIndex(ezUInt8 uiIndex);                                       // [ property ]

  const ezTagSet& GetTags() const { return m_Tags; }                                // [ property ]
  void Reflection_SetTag(const char* szTagName);                                    // [ property ]
  void Reflection_RemoveTag(const char* szTagName);                                 // [ property ]

  /// Per-material-index physics surface handles. Entry i is the surface used when dominant material index == i.
  ezUInt32 Surfaces_GetCount() const;
  ezString Surfaces_GetValue(ezUInt32 uiIndex) const;
  void Surfaces_SetValue(ezUInt32 uiIndex, ezString sValue);
  void Surfaces_Insert(ezUInt32 uiIndex, ezString sValue);
  void Surfaces_Remove(ezUInt32 uiIndex);

  /// Returns the index into ezTerrainSystem for this patch, or ezInvalidIndex if not activated.
  ezUInt32 GetHeightfieldIndex() const { return m_uiHeightfieldIndex; }

  /// Returns the stable identifier derived from the component's editor UUID.
  /// Used to construct deterministic file paths for baked collider assets.
  ezUInt64 GetStableId() const { return m_uiStableId; }

  /// Computes the content hash used to detect whether the baked collider file is up to date.
  /// Pass the brush overlap hash from ezTerrainSystem::GetHeightfieldBrushOverlapHash().
  ezUInt64 ComputeColliderContentHash(ezUInt64 uiBrushOverlapHash) const;

private:
  void OnObjectCreated(const ezAbstractObjectNode& node);

  ezUInt32 m_uiHeightfieldIndex = ezInvalidIndex;
  ezUInt64 m_uiStableId = 0;
  mutable ezInstanceDataOffset m_InstanceDataOffset;

  ezUInt8 m_uiBaseMaterialIndex = 0;
  ezEnum<ezTerrainResolution> m_Resolution;
  float m_fSize = 32.0f;
  ezMaterialResourceHandle m_hMaterial;
  ezEnum<ezTerrainPatchColliderMode> m_ColliderMode;
  ezTagSet m_Tags;
  ezDynamicArray<ezSurfaceResourceHandle> m_Surfaces;

  ezImageDataResourceHandle m_hHeightImage;
  ezVec2 m_vImageOffset = ezVec2::MakeZero();
  ezVec2 m_vImageSize = ezVec2(1.0f);
  float m_fHeightScale = 32.0f;
  bool m_bHeightImageDirty = false; ///< Set when the image resource reloads
};
