#include <KrautPlugin/KrautPluginPCH.h>

#include <KrautPlugin/Resources/KrautGeneratorResource.h>
#include <KrautPlugin/Resources/KrautTreeResource.h>

#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Containers/StaticRingBuffer.h>
#include <Foundation/Math/BoundingSphere.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <KrautGenerator/Description/Physics.h>
#include <KrautGenerator/Lod/TreeStructureLod.h>
#include <KrautGenerator/Lod/TreeStructureLodGenerator.h>
#include <KrautGenerator/Mesh/TreeMesh.h>
#include <KrautGenerator/Mesh/TreeMeshGenerator.h>
#include <KrautGenerator/Serialization/SerializeTree.h>
#include <KrautGenerator/TreeStructure/TreeStructure.h>
#include <KrautGenerator/TreeStructure/TreeStructureGenerator.h>
#include <Utilities/DataStructures/DynamicOctree.h>

using namespace AE_NS_FOUNDATION;

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezKrautGeneratorResource, 1, ezRTTIDefaultAllocator<ezKrautGeneratorResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezKrautGeneratorResource);
// clang-format on

ezKrautGeneratorResource::ezKrautGeneratorResource()
  : ezResource(DoUpdate::OnAnyThread, 1)
{
}

static EZ_ALWAYS_INLINE ezVec3 ToEz(const aeVec3& v)
{
  return ezVec3(v.x, v.y, v.z);
}

static EZ_ALWAYS_INLINE ezVec3 ToEzSwizzle(const aeVec3& v)
{
  return ezVec3(v.x, v.z, v.y);
}

struct AoData
{
  ezDynamicArray<ezDynamicArray<ezBoundingSphere>>* m_pOcclusionSpheres;
  float m_fAO;
  ezUInt32 m_uiBranch;
  ezVec3 m_vPosition;
  ezUInt32* m_pOccChecks;
};

struct AoPositionResult
{
  ezVec3I32 m_iPos; // snapped to a regular grid at some fixed resolution
  float m_fResult = 1.0f;
};

static void GenerateAmbientOcclusionSpheres(ezDynamicOctree& ref_octree, const ezBoundingBox& bbox, ezDynamicArray<ezDynamicArray<ezBoundingSphere>>& ref_occlusionSpheres, const Kraut::TreeStructure& treeStructure)
{
  ezStopwatch swAO;

  ref_octree.CreateTree(bbox.GetCenter(), bbox.GetHalfExtents() + ezVec3(1.0f), 0.1f);

  ref_occlusionSpheres.SetCount(treeStructure.m_BranchStructures.size());
  ezUInt32 uiNumSpheres = 0;

  for (ezUInt32 b = 0; b < treeStructure.m_BranchStructures.size(); ++b)
  {
    auto& spheres = ref_occlusionSpheres[b];
    const auto& branch = treeStructure.m_BranchStructures[b];

    if (branch.m_Type >= Kraut::BranchType::SubBranches1 || branch.m_Nodes.size() < 5)
      continue;

    float fRequiredDistance = 0;

    for (ezUInt32 n = 4; n < branch.m_Nodes.size(); ++n)
    {
      fRequiredDistance -= (branch.m_Nodes[n].m_vPosition - branch.m_Nodes[n - 1].m_vPosition).GetLength();

      if (fRequiredDistance <= 0)
      {
        const float fThickness = branch.m_Nodes[n].m_fThickness;

        if (fThickness < 0.07f)
          break;

        const ezVec3 pos = reinterpret_cast<const ezVec3&>(branch.m_Nodes[n].m_vPosition);

        ++uiNumSpheres;
        spheres.PushBack(ezBoundingSphere::MakeFromCenterAndRadius(pos, fThickness * 1.5f));

        ref_octree.InsertObject(pos, ezVec3(fThickness * 2.0f), b, spheres.GetCount() - 1, nullptr, true).IgnoreResult();

        fRequiredDistance = fThickness;
      }
    }
  }

  ezLog::Debug("Building Kraut AO data structure: {} ({} spheres)", swAO.GetRunningTotal(), uiNumSpheres);
}

static bool FindAoSpheres(void* pPassThrough, ezDynamicTreeObjectConst object)
{
  AoData* ocd = static_cast<AoData*>(pPassThrough);
  const auto& val = object.Value();

  if (ocd->m_uiBranch == val.m_iObjectType)
    return true;

  (*ocd->m_pOccChecks)++;

  const auto& sphere = (*ocd->m_pOcclusionSpheres)[val.m_iObjectType][val.m_iObjectInstance];

  const float dist = sphere.GetDistanceTo(ocd->m_vPosition);

  if (dist < 0)
  {
    ocd->m_fAO *= 0.9f;
  }
  else if (dist < 0.5f)
  {
    ocd->m_fAO *= 0.9f + 0.1f * (dist * 2.0f);
  }

  return true;
};

ezKrautTreeResourceHandle ezKrautGeneratorResource::GenerateTreeWithGoodSeed(ezUInt16 uiGoodSeedIndex) const
{
  if (uiGoodSeedIndex == 0xFFFF || m_pDescriptor->m_GoodRandomSeeds.IsEmpty())
  {
    return GenerateTree(m_pDescriptor->m_uiDefaultDisplaySeed);
  }

  uiGoodSeedIndex %= m_pDescriptor->m_GoodRandomSeeds.GetCount();
  return GenerateTree(m_pDescriptor->m_GoodRandomSeeds[uiGoodSeedIndex]);
}

class ezKrautResourceLoader : public ezResourceTypeLoader
{
public:
  struct LoadedData
  {
    LoadedData()
      : m_Reader(&m_Storage)
    {
    }

    ezDefaultMemoryStreamStorage m_Storage;
    ezMemoryStreamReader m_Reader;
  };

  virtual ezResourceLoadData OpenDataStream(const ezResource* pResource) override
  {
    LoadedData* pData = EZ_DEFAULT_NEW(LoadedData);

    ezResourceLock<ezKrautGeneratorResource> pGenerator(m_hGeneratorResource, ezResourceAcquireMode::BlockTillLoaded);

    ezKrautTreeResourceDescriptor desc;

    pGenerator->GenerateTreeDescriptor(desc, m_uiRandomSeed);

    ezMemoryStreamWriter writer(&pData->m_Storage);

    writer << pResource->GetResourceID();

    ezAssetFileHeader assetHash;
    assetHash.Write(writer).IgnoreResult();

    desc.Save(writer);

    ezResourceLoadData ld;
    ld.m_pDataStream = &pData->m_Reader;
    ld.m_pCustomLoaderData = pData;

    return ld;
  }

  virtual void CloseDataStream(const ezResource* pResource, const ezResourceLoadData& loaderData) override
  {
    LoadedData* pData = (LoadedData*)loaderData.m_pCustomLoaderData;

    EZ_DEFAULT_DELETE(pData);

    // not needed anymore
    m_hGeneratorResource.Invalidate();
  }

  ezUInt32 m_uiRandomSeed = 0;
  ezKrautGeneratorResourceHandle m_hGeneratorResource;
};

ezKrautTreeResourceHandle ezKrautGeneratorResource::GenerateTree(ezUInt32 uiRandomSeed) const
{
  EZ_PROFILE_SCOPE("Kraut: GenerateTree");

  ezStringBuilder sResourceID = GetResourceID();
  ezStringBuilder sResourceDesc = GetResourceDescription();
  sResourceID.AppendFormat(":{}@{}", GetCurrentResourceChangeCounter(), uiRandomSeed);
  sResourceDesc.AppendFormat(":{}@{}", GetCurrentResourceChangeCounter(), uiRandomSeed);

  ezKrautTreeResourceHandle hTree = ezResourceManager::GetExistingResource<ezKrautTreeResource>(sResourceID);
  if (hTree.IsValid())
  {
    return hTree;
  }

  ezUniquePtr<ezKrautResourceLoader> pLoader = EZ_DEFAULT_NEW(ezKrautResourceLoader);
  pLoader->m_hGeneratorResource = ezKrautGeneratorResourceHandle(const_cast<ezKrautGeneratorResource*>(this));
  pLoader->m_uiRandomSeed = uiRandomSeed;

  auto hRes = ezResourceManager::GetExistingResourceOrCreateAsync<ezKrautTreeResource>(sResourceID, std::move(pLoader), m_hFallbackResource);

  if (!m_hFallbackResource.IsValid())
  {
    m_hFallbackResource = hRes;
    ezResourceManager::PreloadResource(m_hFallbackResource);
  }

  return hRes;
}

void ezKrautGeneratorResource::GenerateTreeDescriptor(ezKrautTreeResourceDescriptor& ref_dstDesc, ezUInt32 uiRandomSeed) const
{
  EZ_LOG_BLOCK("Generate Kraut Tree");

  Kraut::TreeStructure treeStructure;

  Kraut::TreeStructureGenerator gen;
  gen.m_pTreeStructureDesc = &m_pDescriptor->m_TreeStructureDesc;
  gen.m_pTreeStructure = &treeStructure;

  gen.GenerateTreeStructure(uiRandomSeed);

  const float fWoodBendiness = 0.1f / m_pDescriptor->m_fTreeStiffness;
  const float fTwigBendiness = 0.1f * fWoodBendiness;

  TreeStructureExtraData extraData;
  GenerateExtraData(extraData, m_pDescriptor->m_TreeStructureDesc, treeStructure, uiRandomSeed, fWoodBendiness, fTwigBendiness);

  auto bbox = treeStructure.ComputeBoundingBox();
  ezBoundingBox bbox2 = ezBoundingBox::MakeFromMinMax(ToEzSwizzle(bbox.m_vMin), ToEzSwizzle(bbox.m_vMax));

  // data for ambient occlusion computation
  ezDynamicArray<ezDynamicArray<ezBoundingSphere>> occlusionSpheres;
  ezDynamicOctree octree;
  ezUInt32 uiOccVertices = 0;
  ezUInt32 uiOccChecks = 0;
  ezStaticRingBuffer<AoPositionResult, 16> aoResults;

  // store spheres for a 'cheap' ambient occlusion computation
  GenerateAmbientOcclusionSpheres(octree, bbox2, occlusionSpheres, treeStructure);

  auto CheckOcclusion = [&](ezUInt32 uiBranch, const ezVec3& vPos) -> float
  {
    constexpr float fCluster = 4.0f;
    constexpr float fDivCluster = 1.0f / fCluster;

    ezVec3I32 ipos;
    ipos.x = ezMath::FloatToInt(vPos.x * fCluster);
    ipos.y = ezMath::FloatToInt(vPos.y * fCluster);
    ipos.z = ezMath::FloatToInt(vPos.z * fCluster);

    for (ezUInt32 i = aoResults.GetCount(); i > 0; --i)
    {
      if (aoResults[i - 1].m_iPos == ipos)
      {
        return aoResults[i - 1].m_fResult;
      }
    }

    AoData ocd;
    ocd.m_pOcclusionSpheres = &occlusionSpheres;
    ocd.m_fAO = 1.0f;
    ocd.m_uiBranch = uiBranch;
    ocd.m_vPosition.Set(ipos.x * fDivCluster, ipos.y * fDivCluster, ipos.z * fDivCluster);
    ocd.m_pOccChecks = &uiOccChecks;

    ++uiOccVertices;
    octree.FindObjectsInRange(vPos, FindAoSpheres, &ocd);

    if (!aoResults.CanAppend())
    {
      aoResults.PopFront();
    }

    AoPositionResult e;
    e.m_iPos = ipos;
    e.m_fResult = ocd.m_fAO;

    aoResults.PushBack(e);

    return ocd.m_fAO;
  };

  float fPrevMaxLodDistance = 0;

  ezVec3 vLeafCenter(0);

  for (ezUInt32 lodIdx = 0; lodIdx < ref_dstDesc.m_Lods.GetCapacity(); ++lodIdx)
  {
    const auto& lodDesc = m_pDescriptor->m_LodDesc[lodIdx];

    if (lodDesc.m_Mode != Kraut::LodMode::Full)
    {
      // stop at the first type of LOD that we don't support
      break;
    }

    Kraut::TreeStructureLod treeLod;

    Kraut::TreeStructureLodGenerator lodGen;
    lodGen.m_pLodDesc = &lodDesc;
    lodGen.m_pTreeStructure = &treeStructure;
    lodGen.m_pTreeStructureDesc = &m_pDescriptor->m_TreeStructureDesc;
    lodGen.m_pTreeStructureLod = &treeLod;

    lodGen.GenerateTreeStructureLod();

    Kraut::TreeMesh mesh;

    Kraut::TreeMeshGenerator meshGen;
    meshGen.m_pLodDesc = lodGen.m_pLodDesc;
    meshGen.m_pTreeStructure = lodGen.m_pTreeStructure;
    meshGen.m_pTreeStructureDesc = lodGen.m_pTreeStructureDesc;
    meshGen.m_pTreeStructureLod = lodGen.m_pTreeStructureLod;
    meshGen.m_pTreeMesh = &mesh;

    meshGen.GenerateTreeMesh();

    auto& dstMesh = ref_dstDesc.m_Lods.ExpandAndGetRef();
    dstMesh.m_LodType = ezKrautLodType::Mesh;

    dstMesh.m_fMinLodDistance = fPrevMaxLodDistance;
    dstMesh.m_fMaxLodDistance = lodDesc.m_uiLodDistance * m_pDescriptor->m_fLodDistanceScale * m_pDescriptor->m_fUniformScaling;
    fPrevMaxLodDistance = dstMesh.m_fMaxLodDistance;

    const float fVertexScale = m_pDescriptor->m_fUniformScaling;

    ezUInt32 uiMaxVertices = 0;
    ezUInt32 uiMaxTriangles = 0;
    ezUInt32 uiTriangleInSubmeshes = 0;

    // reserve enough memory
    {
      for (ezUInt32 branchIdx = 0; branchIdx < mesh.m_BranchMeshes.size(); ++branchIdx)
      {
        for (ezUInt32 geometryType = 0; geometryType < Kraut::BranchGeometryType::ENUM_COUNT; ++geometryType)
        {
          const auto& srcMesh = mesh.m_BranchMeshes[branchIdx].m_Mesh[geometryType];

          uiMaxVertices += srcMesh.m_Vertices.size();
          uiMaxTriangles += srcMesh.m_Triangles.size();
        }
      }

      dstMesh.m_Vertices.Reserve(uiMaxVertices);
      dstMesh.m_Triangles.Reserve(uiMaxTriangles);

      ezLog::Debug("Vertices = {}, Triangles = {}", uiMaxVertices, uiMaxTriangles);
    }

    for (ezUInt32 geometryType = 0; geometryType < Kraut::BranchGeometryType::ENUM_COUNT; ++geometryType)
    {
      for (ezUInt32 branchType = 0; branchType < Kraut::BranchType::ENUM_COUNT; ++branchType)
      {
        const ezUInt32 uiFirstTriangleIdx = dstMesh.m_Triangles.GetCount();

        for (ezUInt32 branchIdx = 0; branchIdx < mesh.m_BranchMeshes.size(); ++branchIdx)
        {
          // yes, we iterate multiple times over the same array to find all the branches of the same type
          // so that we can map all them to the same material
          // a bit idiotic, but that's how the data structure currently works :-|
          if (branchType != treeStructure.m_BranchStructures[branchIdx].m_Type)
            continue;

          const auto& srcMesh = mesh.m_BranchMeshes[branchIdx].m_Mesh[geometryType];

          if (srcMesh.m_Triangles.empty())
            continue;

          aoResults.Clear();

          const ezUInt32 uiVertexOffset = dstMesh.m_Vertices.GetCount();

          for (ezUInt32 vidx = 0; vidx < srcMesh.m_Vertices.size(); ++vidx)
          {
            const auto& srcVtx = srcMesh.m_Vertices[vidx];

            auto& dstVtx = dstMesh.m_Vertices.ExpandAndGetRef();
            dstVtx.m_vPosition = ToEzSwizzle(srcVtx.m_vPosition) * fVertexScale;
            dstVtx.m_uiColorVariation = srcVtx.m_uiColorVariation;
            dstVtx.m_vNormal = ToEzSwizzle(srcVtx.m_vNormal);
            dstVtx.m_vTexCoord = ToEz(srcVtx.m_vTexCoord);
            dstVtx.m_vTangent = ToEzSwizzle(srcVtx.m_vTangent);
            dstVtx.m_fAmbientOcclusion = CheckOcclusion(branchIdx, reinterpret_cast<const ezVec3&>(srcVtx.m_vPosition));

            if (geometryType == Kraut::BranchGeometryType::Leaf)
            {
              const float fSize = dstVtx.m_vTexCoord.z * 0.7f;

              dstVtx.m_fAmbientOcclusion += CheckOcclusion(branchIdx, reinterpret_cast<const ezVec3&>(srcVtx.m_vPosition) + ezVec3(fSize, 0, 0));
              dstVtx.m_fAmbientOcclusion += CheckOcclusion(branchIdx, reinterpret_cast<const ezVec3&>(srcVtx.m_vPosition) - ezVec3(fSize, 0, 0));
              dstVtx.m_fAmbientOcclusion += CheckOcclusion(branchIdx, reinterpret_cast<const ezVec3&>(srcVtx.m_vPosition) + ezVec3(0, fSize, 0));
              dstVtx.m_fAmbientOcclusion += CheckOcclusion(branchIdx, reinterpret_cast<const ezVec3&>(srcVtx.m_vPosition) - ezVec3(0, fSize, 0));
              dstVtx.m_fAmbientOcclusion += CheckOcclusion(branchIdx, reinterpret_cast<const ezVec3&>(srcVtx.m_vPosition) + ezVec3(0, 0, fSize));
              dstVtx.m_fAmbientOcclusion += CheckOcclusion(branchIdx, reinterpret_cast<const ezVec3&>(srcVtx.m_vPosition) - ezVec3(0, 0, fSize));

              dstVtx.m_fAmbientOcclusion /= 7.0f;
            }

            const auto& branchExtra = extraData.m_Branches[branchIdx];

            float fBranchDist = 0;

            if (srcVtx.m_uiBranchNodeIdx >= treeStructure.m_BranchStructures[branchIdx].m_Nodes.size())
            {
              fBranchDist = branchExtra.m_Nodes.PeekBack().m_fBendinessAlongBranch;

              ezUInt32 nodeIdx = srcVtx.m_uiBranchNodeIdx;
              nodeIdx -= treeStructure.m_BranchStructures[branchIdx].m_Nodes.size();

              const ezVec3 lastPos = ToEzSwizzle(treeStructure.m_BranchStructures[branchIdx].m_Nodes.back().m_vPosition);
              const ezVec3 tipPos = ToEzSwizzle(treeLod.m_BranchLODs[branchIdx].m_TipNodes[nodeIdx].m_vPosition);

              fBranchDist += (tipPos - lastPos).GetLength() * fTwigBendiness;
            }
            else
            {
              fBranchDist = branchExtra.m_Nodes[srcVtx.m_uiBranchNodeIdx].m_fBendinessAlongBranch;
            }

            if (branchExtra.m_iParentBranch < 0)
            {
              // trunk
              dstVtx.m_fBendAndFlutterStrength = ezMath::Square(fBranchDist);
              dstVtx.m_uiBranchLevel = 0;
              dstVtx.m_uiFlutterPhase = 0;
              dstVtx.m_fAnchorBendStrength = 0;
              dstVtx.m_vBendAnchor.Set(0, 0, 0.05f); // this mustn't be zero, otherwise the shader has a division by zero
            }
            else
            {
              const auto& parentBranch = extraData.m_Branches[branchExtra.m_iParentBranch];

              dstVtx.m_fBendAndFlutterStrength = ezMath::Square(branchExtra.m_fBendinessToAnchor + fBranchDist);
              dstVtx.m_uiBranchLevel = 1; // or 2 etc, but currently not used

              ezInt32 iMainBranchIdx = branchIdx;

              ezInt32 iTrunkIdx = branchExtra.m_iParentBranch;
              ezUInt32 uiTrunkNodeIdx = branchExtra.m_uiParentBranchNodeID;

              // find the trunk
              while (extraData.m_Branches[iTrunkIdx].m_iParentBranch >= 0)
              {
                iMainBranchIdx = iTrunkIdx;

                uiTrunkNodeIdx = extraData.m_Branches[iTrunkIdx].m_uiParentBranchNodeID;
                iTrunkIdx = extraData.m_Branches[iTrunkIdx].m_iParentBranch;
              }

              const auto& trunkBranch = extraData.m_Branches[iTrunkIdx];
              const auto& mainBranch = extraData.m_Branches[iMainBranchIdx];

              dstVtx.m_fAnchorBendStrength = ezMath::Square(trunkBranch.m_Nodes[uiTrunkNodeIdx].m_fBendinessAlongBranch);

              const aeVec3 pos = treeStructure.m_BranchStructures[iTrunkIdx].m_Nodes[uiTrunkNodeIdx].m_vPosition;
              dstVtx.m_vBendAnchor.Set(pos.x, pos.z, pos.y);
              dstVtx.m_uiFlutterPhase = mainBranch.m_uiRandomNumber % 256;
            }
          }

          for (ezUInt32 tidx = 0; tidx < srcMesh.m_Triangles.size(); ++tidx)
          {
            const auto& srcTri = srcMesh.m_Triangles[tidx];
            auto& dstTri = dstMesh.m_Triangles.ExpandAndGetRef();

            // flip triangle winding
            dstTri.m_uiVertexIndex[0] = uiVertexOffset + srcTri.m_uiVertexIDs[0];
            dstTri.m_uiVertexIndex[1] = uiVertexOffset + srcTri.m_uiVertexIDs[2];
            dstTri.m_uiVertexIndex[2] = uiVertexOffset + srcTri.m_uiVertexIDs[1];
          }
        }

        if (uiFirstTriangleIdx == dstMesh.m_Triangles.GetCount())
        {
          // this tree doesn't have geometry of this type -> don't add materials etc
          continue;
        }

        auto& subMesh = dstMesh.m_SubMeshes.ExpandAndGetRef();
        subMesh.m_uiFirstTriangle = static_cast<ezUInt16>(uiFirstTriangleIdx);
        subMesh.m_uiNumTriangles = static_cast<ezUInt16>(dstMesh.m_Triangles.GetCount() - uiFirstTriangleIdx);

        uiTriangleInSubmeshes += subMesh.m_uiNumTriangles;

        for (const auto& srcMat : m_pDescriptor->m_Materials)
        {
          if ((ezUInt32)srcMat.m_BranchType == branchType && (ezUInt32)srcMat.m_MaterialType == geometryType)
          {
            if (srcMat.m_hMaterial.IsValid())
            {
              subMesh.m_uiMaterialIndex = static_cast<ezUInt8>(ref_dstDesc.m_Materials.GetCount());

              auto& mat = ref_dstDesc.m_Materials.ExpandAndGetRef();
              mat.m_MaterialType = static_cast<ezKrautMaterialType>(geometryType);
              mat.m_VariationColor = ezColor::White;
              mat.m_sMaterial = srcMat.m_hMaterial.GetResourceID(); // TODO: could just pass on the material handle
            }

            break;
          }
        }

        if (subMesh.m_uiMaterialIndex == 255)
        {
          dstMesh.m_SubMeshes.PopBack();
        }
      }
    }

    EZ_ASSERT_DEV(uiTriangleInSubmeshes == uiMaxTriangles, "Number of triangles is incorrect.");

    // compute the leaf center
    if (lodIdx == 0)
    {
      ezBoundingBox leafBox;
      leafBox = ezBoundingBox::MakeInvalid();

      for (ezUInt32 branchIdx = 0; branchIdx < mesh.m_BranchMeshes.size(); ++branchIdx)
      {
        const auto& srcMesh = mesh.m_BranchMeshes[branchIdx].m_Mesh[Kraut::BranchGeometryType::Leaf];

        for (ezUInt32 vtxIdx = 0; vtxIdx < srcMesh.m_Vertices.size(); ++vtxIdx)
        {
          leafBox.ExpandToInclude(ToEzSwizzle(srcMesh.m_Vertices[vtxIdx].m_vPosition));
        }
      }

      if (leafBox.IsValid())
      {
        vLeafCenter = leafBox.GetCenter();
      }
    }
  }

  ezLog::Debug("AO vertices: {}, checks: {}", uiOccVertices, uiOccChecks);

  ref_dstDesc.m_Details.m_Bounds = ezBoundingBoxSphere::MakeFromBox(bbox2);
  ref_dstDesc.m_Details.m_fStaticColliderRadius = m_pDescriptor->m_fStaticColliderRadius;
  ref_dstDesc.m_Details.m_sSurfaceResource = m_pDescriptor->m_sSurfaceResource;
  ref_dstDesc.m_Details.m_vLeafCenter = ref_dstDesc.m_Details.m_Bounds.m_vCenter;

  if (!vLeafCenter.IsZero())
  {
    ref_dstDesc.m_Details.m_vLeafCenter = vLeafCenter;
  }
}

ezResourceLoadDesc ezKrautGeneratorResource::UnloadData(Unload WhatToUnload)
{
  m_pDescriptor.Clear();

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

class KrautStreamIn : public aeStreamIn
{
public:
  ezStreamReader* m_pStream = nullptr;

private:
  virtual aeUInt32 ReadFromStream(void* pData, aeUInt32 uiSize) override { return (aeUInt32)m_pStream->ReadBytes(pData, uiSize); }
};

class KrautStreamOut : public aeStreamOut
{
public:
  ezStreamWriter* m_pStream = nullptr;

private:
  virtual void WriteToStream(const void* pData, aeUInt32 uiSize) override { m_pStream->WriteBytes(pData, uiSize).IgnoreResult(); }
};

ezResourceLoadDesc ezKrautGeneratorResource::UpdateContent(ezStreamReader* Stream)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  if (Stream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    ezStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;

    if (ezPathUtils::HasExtension(sAbsFilePath, ".tree"))
    {
      return res;
    }
  }

  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  if (AssetHash.GetFileVersion() < 4)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  m_pDescriptor = EZ_DEFAULT_NEW(ezKrautGeneratorResourceDescriptor);
  if (m_pDescriptor->Deserialize(*Stream).Failed())
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  return res;
}

void ezKrautGeneratorResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = sizeof(*this);
  out_NewMemoryUsage.m_uiMemoryCPU = 0;

  if (m_pDescriptor != nullptr)
  {
    out_NewMemoryUsage.m_uiMemoryCPU += sizeof(ezKrautGeneratorResourceDescriptor) + m_pDescriptor->m_Materials.GetHeapMemoryUsage();
  }
}

static ezUInt8 GetBranchLevel(const Kraut::TreeStructure& treeStructure, ezUInt32 uiBranchIdx)
{
  ezUInt8 uiLevel = 0;

  while (treeStructure.m_BranchStructures[uiBranchIdx].m_iParentBranchID >= 0)
  {
    ++uiLevel;
    uiBranchIdx = treeStructure.m_BranchStructures[uiBranchIdx].m_iParentBranchID;
  }

  return uiLevel;
}

void ezKrautGeneratorResource::InitializeExtraData(TreeStructureExtraData& extraData, const Kraut::TreeStructure& treeStructure, ezUInt32 uiRandomSeed) const
{
  extraData.m_Branches.Clear();
  extraData.m_Branches.SetCount(treeStructure.m_BranchStructures.size());

  Kraut::RandomNumberGenerator rng;
  rng.m_uiSeedValue = uiRandomSeed;

  for (ezUInt32 branchIdx = 0; branchIdx < treeStructure.m_BranchStructures.size(); ++branchIdx)
  {
    const auto& srcBranch = treeStructure.m_BranchStructures[branchIdx];
    auto& dstData = extraData.m_Branches[branchIdx];

    dstData.m_uiRandomNumber = rng.GetRandomNumber();

    dstData.m_iParentBranch = srcBranch.m_iParentBranchID;
    dstData.m_uiParentBranchNodeID = static_cast<ezUInt16>(srcBranch.m_uiParentBranchNodeID);
    dstData.m_uiBranchLevel = GetBranchLevel(treeStructure, branchIdx);

    dstData.m_Nodes.SetCount(srcBranch.m_Nodes.size());
  }
}

void ezKrautGeneratorResource::ComputeDistancesAlongBranches(TreeStructureExtraData& extraData, const Kraut::TreeStructure& treeStructure) const
{
  for (ezUInt32 branchIdx = 0; branchIdx < treeStructure.m_BranchStructures.size(); ++branchIdx)
  {
    const auto& srcBranch = treeStructure.m_BranchStructures[branchIdx];
    auto& dstData = extraData.m_Branches[branchIdx];

    float fTotalDistance = 0.0f;

    for (ezUInt32 nodeIdx = 1; nodeIdx < srcBranch.m_Nodes.size(); ++nodeIdx)
    {
      const float fSegmentLength = (srcBranch.m_Nodes[nodeIdx].m_vPosition - srcBranch.m_Nodes[nodeIdx - 1].m_vPosition).GetLength();

      fTotalDistance += fSegmentLength;

      dstData.m_Nodes[nodeIdx].m_fSegmentLength = fSegmentLength;
      dstData.m_Nodes[nodeIdx].m_fDistanceAlongBranch = fTotalDistance;
    }
  }
}

void ezKrautGeneratorResource::ComputeDistancesToAnchors(TreeStructureExtraData& extraData, const Kraut::TreeStructure& treeStructure) const
{
  for (ezUInt32 branchIdx = 0; branchIdx < treeStructure.m_BranchStructures.size(); ++branchIdx)
  {
    auto& thisBranch = extraData.m_Branches[branchIdx];

    // trunks and main branches have their own anchors, so they are at distance 0
    if (thisBranch.m_uiBranchLevel < 2)
      continue;

    const auto& parentBranch = extraData.m_Branches[thisBranch.m_iParentBranch];

    // the distance of the parent branch to its anchor, plus the distance along the parent branch where THIS branch is attached
    thisBranch.m_fDistanceToAnchor = parentBranch.m_fDistanceToAnchor + parentBranch.m_Nodes[thisBranch.m_uiParentBranchNodeID].m_fDistanceAlongBranch;
  }
}

void ezKrautGeneratorResource::ComputeBendinessAlongBranches(TreeStructureExtraData& extraData, const Kraut::TreeStructure& treeStructure, float fWoodBendiness, float fTwigBendiness) const
{
  for (ezUInt32 branchIdx = 0; branchIdx < treeStructure.m_BranchStructures.size(); ++branchIdx)
  {
    const auto& srcBranch = treeStructure.m_BranchStructures[branchIdx];
    auto& dstData = extraData.m_Branches[branchIdx];

    if (dstData.m_Nodes.IsEmpty())
      continue;

    float fTotalBendiness = 0.0f;

    if (dstData.m_uiBranchLevel >= 2)
    {
      for (ezUInt32 nodeIdx = 1; nodeIdx < srcBranch.m_Nodes.size(); ++nodeIdx)
      {
        fTotalBendiness += dstData.m_Nodes[nodeIdx].m_fSegmentLength * fTwigBendiness;

        dstData.m_Nodes[nodeIdx].m_fBendinessAlongBranch = fTotalBendiness;
      }
    }
    else
    {
      float fRemainingLength = dstData.m_Nodes.PeekBack().m_fDistanceAlongBranch;

      for (ezUInt32 nodeIdx = 1; nodeIdx < srcBranch.m_Nodes.size(); ++nodeIdx)
      {
        const float fSegmentLength = dstData.m_Nodes[nodeIdx].m_fSegmentLength;
        const float fThickness = srcBranch.m_Nodes[nodeIdx].m_fThickness;

        const float fBendinessStep = (fRemainingLength / fThickness) * fSegmentLength * fWoodBendiness;

        fTotalBendiness += fBendinessStep;

        dstData.m_Nodes[nodeIdx].m_fBendinessAlongBranch = fTotalBendiness;

        fRemainingLength -= fSegmentLength;
      }
    }
  }
}

void ezKrautGeneratorResource::ComputeBendinessToAnchors(TreeStructureExtraData& extraData, const Kraut::TreeStructure& treeStructure) const
{
  for (ezUInt32 branchIdx = 0; branchIdx < treeStructure.m_BranchStructures.size(); ++branchIdx)
  {
    auto& thisBranch = extraData.m_Branches[branchIdx];

    // trunks and main branches have their own anchors, so they are at bendiness 0
    if (thisBranch.m_uiBranchLevel < 2)
      continue;

    const auto& parentBranch = extraData.m_Branches[thisBranch.m_iParentBranch];

    // the bendiness of the parent branch to its anchor, plus the bendiness along the parent branch where THIS branch is attached
    thisBranch.m_fBendinessToAnchor = parentBranch.m_fBendinessToAnchor + parentBranch.m_Nodes[thisBranch.m_uiParentBranchNodeID].m_fBendinessAlongBranch;
  }
}

void ezKrautGeneratorResource::GenerateExtraData(TreeStructureExtraData& extraData, const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::TreeStructure& treeStructure, ezUInt32 uiRandomSeed, float fWoodBendiness, float fTwigBendiness) const
{
  InitializeExtraData(extraData, treeStructure, uiRandomSeed);
  ComputeDistancesAlongBranches(extraData, treeStructure);
  ComputeDistancesToAnchors(extraData, treeStructure);
  ComputeBendinessAlongBranches(extraData, treeStructure, fWoodBendiness, fTwigBendiness);
  ComputeBendinessToAnchors(extraData, treeStructure);
}

ezResult ezKrautGeneratorResourceDescriptor::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(7);

  KrautStreamOut kstream;
  kstream.m_pStream = &inout_stream;

  Kraut::Serializer ts;
  ts.m_pTreeStructure = &m_TreeStructureDesc;
  ts.m_LODs[0] = &m_LodDesc[0];
  ts.m_LODs[1] = &m_LodDesc[1];
  ts.m_LODs[2] = &m_LodDesc[2];
  ts.m_LODs[3] = &m_LodDesc[3];
  ts.m_LODs[4] = &m_LodDesc[4];

  ts.Serialize(kstream);

  const ezUInt8 uiNumMaterials = static_cast<ezUInt8>(m_Materials.GetCount());
  inout_stream << uiNumMaterials;

  for (const auto& mat : m_Materials)
  {
    inout_stream << (ezInt8)mat.m_BranchType;
    inout_stream << (ezInt8)mat.m_MaterialType;
    inout_stream << mat.m_hMaterial;
  }

  inout_stream << m_fStaticColliderRadius;
  inout_stream << m_sSurfaceResource;
  inout_stream << m_fUniformScaling;
  inout_stream << m_fLodDistanceScale;

  inout_stream << m_uiDefaultDisplaySeed;
  EZ_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_GoodRandomSeeds));

  inout_stream << m_fTreeStiffness;

  return EZ_SUCCESS;
}

ezResult ezKrautGeneratorResourceDescriptor::Deserialize(ezStreamReader& inout_stream)
{
  auto version = inout_stream.ReadVersion(7);

  KrautStreamIn kstream;
  kstream.m_pStream = &inout_stream;

  Kraut::Deserializer ts;
  ts.m_pTreeStructure = &m_TreeStructureDesc;
  ts.m_LODs[0] = &m_LodDesc[0];
  ts.m_LODs[1] = &m_LodDesc[1];
  ts.m_LODs[2] = &m_LodDesc[2];
  ts.m_LODs[3] = &m_LodDesc[3];
  ts.m_LODs[4] = &m_LodDesc[4];

  if (!ts.Deserialize(kstream))
  {
    return EZ_FAILURE;
  }

  ezUInt8 uiNumMaterials = 0;
  inout_stream >> uiNumMaterials;
  m_Materials.SetCount(uiNumMaterials);

  for (auto& mat : m_Materials)
  {
    if (version >= 4)
    {
      ezInt8 type;
      inout_stream >> type;
      mat.m_BranchType = (ezKrautBranchType)type;
    }

    if (version >= 3)
    {
      ezInt8 type;
      inout_stream >> type;
      mat.m_MaterialType = (ezKrautMaterialType)type;
    }

    inout_stream >> mat.m_hMaterial;
  }

  if (version >= 2)
  {
    inout_stream >> m_fStaticColliderRadius;
    inout_stream >> m_sSurfaceResource;
    inout_stream >> m_fUniformScaling;
    inout_stream >> m_fLodDistanceScale;
  }

  if (version >= 6)
  {
    inout_stream >> m_uiDefaultDisplaySeed;
    EZ_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_GoodRandomSeeds));
  }
  else if (version == 5)
  {
    ezHybridArray<ezUInt32, 16> dummy;
    EZ_SUCCEED_OR_RETURN(inout_stream.ReadArray(dummy));
  }

  if (version >= 7)
  {
    inout_stream >> m_fTreeStiffness;
  }

  return EZ_SUCCESS;
}
