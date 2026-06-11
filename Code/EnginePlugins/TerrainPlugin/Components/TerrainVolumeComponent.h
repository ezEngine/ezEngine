#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Types/TagSet.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <TerrainPlugin/TerrainPluginDLL.h>
#include <TerrainPlugin/TerrainSystem.h>

struct ezMsgExtractRenderData;
struct ezMsgTransformChanged;

using ezMaterialResourceHandle = ezTypedResourceHandle<class ezMaterialResource>;
using ezSurfaceResourceHandle = ezTypedResourceHandle<class ezSurfaceResource>;

using ezTerrainVolumeComponentManager = ezComponentManager<class ezTerrainVolumeComponent, ezBlockStorageType::Compact>;

/// Renders a voxel volume built from terrain brushes.
///
/// Brushes with ModifyMode != Ignore fill voxels within their sphere (OuterRadius) to solid.
/// Voxels are baked on the GPU and triangulated on the GPU. The resulting mesh is rendered
/// directly from the GPU-side buffers. Physics collision is baked to disk at export time.
class EZ_TERRAINPLUGIN_DLL ezTerrainVolumeComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezTerrainVolumeComponent, ezRenderComponent, ezTerrainVolumeComponentManager);

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
  // ezTerrainVolumeComponent

public:
  ezTerrainVolumeComponent();
  ~ezTerrainVolumeComponent();

  void SetResolution(ezEnum<ezTerrainResolution> resolution);  // [ property ]
  ezEnum<ezTerrainResolution> GetResolution() const { return m_Resolution; }

  void SetSize(float f);                                       // [ property ]
  float GetSize() const { return m_fSize; }

  void SetMaterial(const ezMaterialResourceHandle& hMaterial); // [ property ]
  const ezMaterialResourceHandle& GetMaterial() const { return m_hMaterial; }

  void SetBaseMaterialIndex(ezUInt8 i);                        // [ property ]
  ezUInt8 GetBaseMaterialIndex() const { return m_uiBaseMaterialIndex; }

  void SetFillHeight(float f);                                 // [ property ]
  float GetFillHeight() const { return m_fFillHeight; }

  void SetEnableCollider(bool bEnable);                        // [ property ]
  bool GetEnableCollider() const { return m_bEnableCollider; } // [ property ]

  void SetCleanupIterations(ezUInt8 n);                        // [ property ]
  ezUInt8 GetCleanupIterations() const { return m_uiCleanupIterations; }

  const ezTagSet& GetTags() const { return m_Tags; }           // [ property ]
  void Reflection_SetTag(const char* szTagName);               // [ property ]
  void Reflection_RemoveTag(const char* szTagName);            // [ property ]

  /// Per-material-index physics surface handles. Entry i is the surface used when the vertex material index == i and MaterialStrength > 0.5.
  ezUInt32 Surfaces_GetCount() const;
  ezString Surfaces_GetValue(ezUInt32 uiIndex) const;
  void Surfaces_SetValue(ezUInt32 uiIndex, ezString sValue);
  void Surfaces_Insert(ezUInt32 uiIndex, ezString sValue);
  void Surfaces_Remove(ezUInt32 uiIndex);

  ezUInt32 GetVoxelIndex() const { return m_uiVoxelIndex; }

  /// Stable ID derived from the document GUID at object creation. Used to generate a deterministic cache file name.
  ezUInt64 GetStableId() const { return m_uiStableId; }

  /// Computes a content hash over all voxel-relevant properties of this component.
  ///
  /// Used to detect whether a baked collision mesh is still up to date.
  ezUInt64 ComputeColliderContentHash(ezUInt64 uiBrushOverlapHash) const;

private:
  void OnObjectCreated(const ezAbstractObjectNode& node);

  ezUInt32 m_uiVoxelIndex = ezInvalidIndex;
  ezUInt64 m_uiStableId = 0;
  ezEnum<ezTerrainResolution> m_Resolution;
  bool m_bEnableCollider = true;
  ezUInt8 m_uiBaseMaterialIndex = 0;
  ezUInt8 m_uiCleanupIterations = 2;
  float m_fSize = 64.0f;
  float m_fFillHeight = -1.0f;
  ezMaterialResourceHandle m_hMaterial;
  mutable ezInstanceDataOffset m_InstanceDataOffset;
  ezTagSet m_Tags; ///< Identity tags matched against brush include-tag filters.

  /// Physics surface handles indexed by voxel material index.
  ezDynamicArray<ezSurfaceResourceHandle> m_Surfaces;
};
