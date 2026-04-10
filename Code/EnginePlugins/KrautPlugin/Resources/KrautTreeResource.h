#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Threading/Mutex.h>
#include <KrautPlugin/KrautDeclarations.h>

using ezMeshResourceHandle = ezTypedResourceHandle<class ezMeshResource>;
using ezKrautTreeResourceHandle = ezTypedResourceHandle<class ezKrautTreeResource>;
using ezMaterialResourceHandle = ezTypedResourceHandle<class ezMaterialResource>;
using ezSurfaceResourceHandle = ezTypedResourceHandle<class ezSurfaceResource>;

/// Tree-level properties that are seed-independent and become available once the base data task completes.
struct EZ_KRAUTPLUGIN_DLL ezKrautTreeResourceDetails
{
  ezBoundingBoxSphere m_Bounds;
  ezVec3 m_vLeafCenter = ezVec3::MakeZero();
  float m_fStaticColliderRadius = 0.0f;
  ezString m_sSurfaceResource;
};

/// Full serializable descriptor for a single per-seed tree resource, including all LOD mesh data.
///
/// This descriptor is produced by the Kraut generator and can be saved to and loaded from a stream.
/// It is also the input to ezKrautTreeResource::CreateResource().
struct EZ_KRAUTPLUGIN_DLL ezKrautTreeResourceDescriptor
{
  void Save(ezStreamWriter& inout_stream) const;
  ezResult Load(ezStreamReader& inout_stream);

  /// Per-vertex data for a LOD mesh.
  struct VertexData
  {
    EZ_DECLARE_POD_TYPE();

    ezVec3 m_vPosition;
    ezVec3 m_vTexCoord; ///< U, V texture coordinates and Q (a third coordinate used for branch-along-UV mapping).
    float m_fAmbientOcclusion = 1.0f;
    ezVec3 m_vNormal;
    ezVec3 m_vTangent;
    ezUInt8 m_uiColorVariation = 0;

    // to compute wind
    ezUInt8 m_uiBranchLevel = 0;  ///< 0 = trunk, 1 = main branches, 2 = twigs, etc. Used by the wind shader to scale bend strength per level.
    ezUInt8 m_uiFlutterPhase = 0; ///< Phase shift for the per-leaf flutter animation, randomized per leaf.
    /// World-space position of the wind-simulation anchor point for this vertex.
    /// The wind shader divides by the length of (vertex - anchor), so this must never equal the vertex position (i.e. must not be zero after subtraction).
    ezVec3 m_vBendAnchor;
    float m_fAnchorBendStrength = 0;     ///< Controls how strongly the global wind force bends the branch toward its anchor.
    float m_fBendAndFlutterStrength = 0; ///< Controls the local bend and flutter magnitude for this vertex.
  };

  struct TriangleData
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt32 m_uiVertexIndex[3];
  };

  /// Defines a draw call within a LOD: a contiguous range of triangles sharing a single material.
  struct SubMeshData
  {
    ezUInt16 m_uiFirstTriangle = 0;
    ezUInt16 m_uiNumTriangles = 0;
    ezUInt8 m_uiMaterialIndex = 0xFF; ///< Index into the LOD's material array; 0xFF means no material assigned.
  };

  /// All mesh data for one LOD level.
  struct LodData
  {
    float m_fMinLodDistance = 0;
    float m_fMaxLodDistance = 0;
    ezKrautLodType m_LodType = ezKrautLodType::None;

    ezUInt32 m_uiNumBones = 0;

    ezDynamicArray<VertexData> m_Vertices;
    ezDynamicArray<TriangleData> m_Triangles;
    ezDynamicArray<SubMeshData> m_SubMeshes;
  };

  struct MaterialData
  {
    ezKrautMaterialType m_MaterialType;
    ezKrautBranchType m_BranchType = ezKrautBranchType::None;
    ezString m_sMaterial;
    ezColorGammaUB m_VariationColor = ezColor::White; // currently done through the material
  };

  ezKrautTreeResourceDetails m_Details;
  ezStaticArray<LodData, 6> m_Lods;
  ezHybridArray<MaterialData, 8> m_Materials;
};

/// Holds the runtime mesh data for one Kraut tree instance (a specific random seed).
///
/// Instances are created and owned by ezKrautGeneratorResource, one per seed value that is
/// in active use. Mesh data is populated asynchronously: first the bounds and materials are
/// set via SetDetails(), then each LOD slot is filled individually via SetLodMesh() as the
/// corresponding generation task finishes.
///
/// The LOD state machine per slot goes: NotGenerated -> Generating -> Ready.
/// All state-mutating methods are thread-safe.
class EZ_KRAUTPLUGIN_DLL ezKrautTreeResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezKrautTreeResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezKrautTreeResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezKrautTreeResource, ezKrautTreeResourceDescriptor);

public:
  ezKrautTreeResource();

  const ezKrautTreeResourceDetails& GetDetails() const { return m_Details; }
  ezArrayPtr<const ezKrautTreeResourceDescriptor::MaterialData> GetMaterials() const { return m_Materials; }

  /// Runtime representation of a single LOD level, including its mesh handle and generation state.
  struct TreeLod
  {
    ezMeshResourceHandle m_hMesh;
    float m_fMinLodDistance = 0;
    float m_fMaxLodDistance = 0;
    ezKrautLodType m_LodType = ezKrautLodType::None;
    ezKrautLodState m_State = ezKrautLodState::NotGenerated;

    ezUInt32 m_uiNumBones = 0;
    ezUInt32 m_uiNumTrianglesBranch = 0;
    ezUInt32 m_uiNumTrianglesFrond = 0;
    ezUInt32 m_uiNumTrianglesLeaf = 0;

    /// The full material list for this LOD, including branch-type/geometry-type metadata.
    /// Sub-mesh material indices reference this array.
    ezHybridArray<ezKrautTreeResourceDescriptor::MaterialData, 8> m_Materials;
  };

  ezArrayPtr<const TreeLod> GetTreeLODs() const { return m_TreeLODs.GetArrayPtr(); }

  /// Sets bounds and materials. Called by the generator once tree structure is known, before any mesh is generated.
  void SetDetails(const ezKrautTreeResourceDetails& details, ezArrayPtr<const ezKrautTreeResourceDescriptor::MaterialData> materials);

  /// Creates the mesh resource for a LOD slot and sets its state to Ready. Thread-safe.
  void SetLodMesh(ezUInt32 uiLodIndex, const ezKrautTreeResourceDescriptor::LodData& lodData, ezArrayPtr<const ezKrautTreeResourceDescriptor::MaterialData> materials);

  /// Sets a LOD slot's state (e.g. NotGenerated -> Generating). Thread-safe.
  void SetLodState(ezUInt32 uiLodIndex, ezKrautLodState state);

  /// Returns the current state of a LOD slot. Thread-safe.
  ezKrautLodState GetLodState(ezUInt32 uiLodIndex) const;

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  mutable ezMutex m_LodMutex;
  ezKrautTreeResourceDetails m_Details;
  ezStaticArray<TreeLod, 6> m_TreeLODs;
  ezHybridArray<ezKrautTreeResourceDescriptor::MaterialData, 8> m_Materials;
};
