#pragma once

#include <Core/ResourceManager/Resource.h>
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

struct ezKrautMaterialDescriptor
{
  ezKrautMaterialType m_MaterialType = ezKrautMaterialType::None;
  ezKrautBranchType m_BranchType = ezKrautBranchType::None;
  ezMaterialResourceHandle m_hMaterial;
};

struct EZ_KRAUTPLUGIN_DLL ezKrautGeneratorResourceDescriptor
{
  Kraut::TreeStructureDesc m_TreeStructureDesc;
  Kraut::LodDesc m_LodDesc[5];

  ezHybridArray<ezKrautMaterialDescriptor, 4> m_Materials;

  ezString m_sSurfaceResource;
  float m_fStaticColliderRadius = 0.5f;
  float m_fUniformScaling = 1.0f;
  float m_fLodDistanceScale = 1.0f;
  float m_fTreeStiffness = 10.0f;

  ezUInt16 m_uiDefaultDisplaySeed = 0;
  ezHybridArray<ezUInt16, 16> m_GoodRandomSeeds;

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);
};

class EZ_KRAUTPLUGIN_DLL ezKrautGeneratorResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezKrautGeneratorResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezKrautGeneratorResource);

public:
  ezKrautGeneratorResource();

  ezKrautTreeResourceHandle GenerateTree(ezUInt32 uiRandomSeed) const;
  ezKrautTreeResourceHandle GenerateTreeWithGoodSeed(ezUInt16 uiGoodSeedIndex) const;

  void GenerateTreeDescriptor(ezKrautTreeResourceDescriptor& ref_dstDesc, ezUInt32 uiRandomSeed) const;

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  ezUniquePtr<ezKrautGeneratorResourceDescriptor> m_pDescriptor;

  struct BranchNodeExtraData
  {
    float m_fSegmentLength = 0.0f;
    float m_fDistanceAlongBranch = 0.0f;
    float m_fBendinessAlongBranch = 0.0f;
  };

  struct BranchExtraData
  {
    ezInt32 m_iParentBranch = -1;        // trunks have parent ID -1
    ezUInt16 m_uiParentBranchNodeID = 0; // at which node of the parent, this branch is attached
    ezUInt8 m_uiBranchLevel = 0;
    ezDynamicArray<BranchNodeExtraData> m_Nodes;
    float m_fDistanceToAnchor = 0;       // this will be zero for level 0 (trunk) and 1 (main branches) and only > 0 starting at level 2 (twigs)
    float m_fBendinessToAnchor = 0;
    ezUInt32 m_uiRandomNumber = 0;
  };

  struct TreeStructureExtraData
  {
    ezDynamicArray<BranchExtraData> m_Branches;
  };

  mutable ezKrautTreeResourceHandle m_hFallbackResource;

  void InitializeExtraData(TreeStructureExtraData& extraData, const Kraut::TreeStructure& treeStructure, ezUInt32 uiRandomSeed) const;
  void ComputeDistancesAlongBranches(TreeStructureExtraData& extraData, const Kraut::TreeStructure& treeStructure) const;
  void ComputeDistancesToAnchors(TreeStructureExtraData& extraData, const Kraut::TreeStructure& treeStructure) const;
  void ComputeBendinessAlongBranches(TreeStructureExtraData& extraData, const Kraut::TreeStructure& treeStructure, float fWoodBendiness, float fTwigBendiness) const;
  void ComputeBendinessToAnchors(TreeStructureExtraData& extraData, const Kraut::TreeStructure& treeStructure) const;
  void GenerateExtraData(TreeStructureExtraData& treeStructureExtraData, const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::TreeStructure& treeStructure, ezUInt32 uiRandomSeed, float fWoodBendiness, float fTwigBendiness) const;
};
