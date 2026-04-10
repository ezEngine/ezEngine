#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/TaskSystem.h>
#include <KrautPlugin/KrautDeclarations.h>

#include <KrautGenerator/Description/LodDesc.h>
#include <KrautGenerator/Description/TreeStructureDesc.h>

struct ezKrautTreeResourceDescriptor;

namespace Kraut
{
  struct TreeStructure;
  struct TreeStructureDesc;
}; // namespace Kraut

using ezKrautGeneratorResourceHandle = ezTypedResourceHandle<class ezKrautGeneratorResource>;
using ezKrautTreeResourceHandle = ezTypedResourceHandle<class ezKrautTreeResource>;
using ezMaterialResourceHandle = ezTypedResourceHandle<class ezMaterialResource>;

/// Associates a material resource with the branch type and geometry type it is applied to.
struct ezKrautMaterialDescriptor
{
  ezKrautMaterialType m_MaterialType = ezKrautMaterialType::None;
  ezKrautBranchType m_BranchType = ezKrautBranchType::None;
  ezMaterialResourceHandle m_hMaterial;
};

/// Holds all authoring parameters for a Kraut tree asset.
///
/// This descriptor is loaded from disk and shared across all seed instances.
/// It drives both tree structure generation (via Kraut::TreeStructureDesc) and
/// runtime properties such as collision, wind, and LOD switching distances.
struct EZ_KRAUTPLUGIN_DLL ezKrautGeneratorResourceDescriptor : public ezRefCounted
{
  Kraut::TreeStructureDesc m_TreeStructureDesc;
  Kraut::LodDesc m_LodDesc[5];

  ezHybridArray<ezKrautMaterialDescriptor, 4> m_Materials;

  ezString m_sSurfaceResource;
  float m_fStaticColliderRadius = 0.5f;
  float m_fUniformScaling = 1.0f;
  float m_fLodDistanceScale = 1.0f;
  float m_fTreeStiffness = 10.0f;
  float m_fMinAmbientOcclusion = 0.7f;

  /// Seed shown in the asset editor when no explicit seed is selected.
  ezUInt16 m_uiDefaultDisplaySeed = 0;
  /// Curated list of seeds that produce good-looking tree variations.
  ezHybridArray<ezUInt16, 16> m_GoodRandomSeeds;

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);
};

/// Resource that holds the Kraut tree generator descriptor and manages per-seed ezKrautTreeResource instances.
///
/// A single generator resource is shared by all ezKrautTreeComponent instances that reference the same
/// tree asset. For each random seed in use, the generator creates one ezKrautTreeResource and
/// asynchronously fills it with mesh data, LOD by LOD.
///
/// LOD index 0 is the full-detail preview LOD used in the asset editor and is never auto-selected
/// by distance at runtime. LOD indices 1..N are the runtime LODs selected by camera distance.
class EZ_KRAUTPLUGIN_DLL ezKrautGeneratorResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezKrautGeneratorResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezKrautGeneratorResource);

public:
  ezKrautGeneratorResource();

  const ezSharedPtr<ezKrautGeneratorResourceDescriptor>& GetDescriptor() const
  {
    return m_pGeneratorDesc;
  }

  /// Returns the number of runtime LODs (indices 1..N in the internal representation; excludes LOD0 full-detail).
  ezUInt32 GetLodCount() const;

  /// Returns the max distance for a runtime LOD (0-based: 0 = first runtime LOD).
  float GetLodDistance(ezUInt32 uiLodIndex) const;

  /// Returns (creating if needed) an empty tree resource skeleton for the given seed.
  /// Also queues async generation of the base data (tree structure + bounds).
  ezKrautTreeResourceHandle GetOrCreateTreeResource(ezUInt32 uiSeed);

  /// Requests that a specific LOD mesh be generated.
  ///
  /// uiLodIndex 0 = full-detail LOD, 1..N = runtime LODs.
  /// When bImmediate is true, generation runs synchronously on the calling thread,
  /// which will block until the mesh is ready. Returns true if the LOD is already Ready.
  bool RequestLodMesh(ezKrautTreeResourceHandle hTree, ezUInt32 uiSeed, ezUInt32 uiLodIndex, bool bImmediate) const;

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  mutable ezMutex m_DataMutex;
  ezSharedPtr<ezKrautGeneratorResourceDescriptor> m_pGeneratorDesc;

  /// Per-node wind data computed during extra-data initialization.
  struct BranchNodeExtraData
  {
    float m_fSegmentLength = 0.0f;
    float m_fDistanceAlongBranch = 0.0f;
    float m_fBendinessAlongBranch = 0.0f;
  };

  /// Per-branch wind data computed during extra-data initialization.
  struct BranchExtraData
  {
    ezInt32 m_iParentBranch = -1;
    ezUInt16 m_uiParentBranchNodeID = 0;
    ezUInt8 m_uiBranchLevel = 0;
    ezDynamicArray<BranchNodeExtraData> m_Nodes;
    float m_fDistanceToAnchor = 0;
    float m_fBendinessToAnchor = 0;
    ezUInt32 m_uiRandomNumber = 0;
  };

  /// Extra wind and AO data computed alongside the Kraut tree structure for a single seed.
  struct TreeStructureExtraData
  {
    ezDynamicArray<BranchExtraData> m_Branches;
  };

  void InitializeExtraData(TreeStructureExtraData& extraData, const Kraut::TreeStructure& treeStructure, ezUInt32 uiRandomSeed) const;
  void ComputeDistancesAlongBranches(TreeStructureExtraData& extraData, const Kraut::TreeStructure& treeStructure) const;
  void ComputeDistancesToAnchors(TreeStructureExtraData& extraData, const Kraut::TreeStructure& treeStructure) const;
  void ComputeBendinessAlongBranches(TreeStructureExtraData& extraData, const Kraut::TreeStructure& treeStructure, float fWoodBendiness, float fTwigBendiness) const;
  void ComputeBendinessToAnchors(TreeStructureExtraData& extraData, const Kraut::TreeStructure& treeStructure) const;
  void GenerateExtraData(TreeStructureExtraData& treeStructureExtraData, const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::TreeStructure& treeStructure, ezUInt32 uiRandomSeed, float fWoodBendiness, float fTwigBendiness) const;

public:
  // Forward declarations for internal implementation types defined in KrautGeneratorResource.cpp.
  // Public so that helper free functions in the .cpp can reference the type names.
  struct ezKrautSharedTreeData;
  class ezKrautBaseDataTask;
  class ezKrautLodGenerationTask;

private:
  /// Tracks the async generation state for a single seed value.
  ///
  /// Each seed that has been requested via GetOrCreateTreeResource() gets one SeedState entry.
  /// m_pSharedData is null until the base data task completes; LOD tasks must not start before that.
  struct SeedState
  {
    ~SeedState();                                     // defined in KrautGeneratorResource.cpp where task types are complete

    ezKrautTreeResourceHandle m_hTree;
    ezSharedPtr<ezKrautSharedTreeData> m_pSharedData; ///< Null until the base data task completes.
    ezSharedPtr<ezKrautBaseDataTask> m_pBaseDataTask;
    ezSharedPtr<ezKrautLodGenerationTask> m_PendingLodTasks[6];
  };

  mutable ezMutex m_GenerationMutex;
  mutable ezHashTable<ezUInt32, SeedState> m_SeedStates;

  void GenerateBaseDataImmediate(SeedState& state, ezUInt32 uiSeed, const ezSharedPtr<ezKrautGeneratorResourceDescriptor>& desc) const;
  void GenerateSingleLodMeshImmediate(const ezSharedPtr<ezKrautSharedTreeData>& pSharedData, ezUInt32 uiLodIndex, ezKrautTreeResourceHandle hTree) const;
};
