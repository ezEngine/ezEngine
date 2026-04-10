#include <KrautPlugin/KrautPluginPCH.h>

#include <KrautPlugin/Resources/KrautGeneratorResource.h>
#include <KrautPlugin/Resources/KrautTreeResource.h>

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
};

struct AoPositionResult
{
  ezVec3I32 m_iPos; // snapped to a regular grid at some fixed resolution
  float m_fResult = 1.0f;
};

static void GenerateAmbientOcclusionSpheres(ezDynamicOctree& ref_octree, const ezBoundingBox& bbox, ezDynamicArray<ezDynamicArray<ezBoundingSphere>>& ref_occlusionSpheres, const Kraut::TreeStructure& treeStructure)
{
  ref_occlusionSpheres.Clear();

  if (!bbox.IsValid())
    return;

  EZ_PROFILE_SCOPE("Kraut::GenerateAmbientOcclusionSpheres");

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
}

static bool FindAoSpheres(void* pPassThrough, ezDynamicTreeObjectConst object)
{
  AoData* ocd = static_cast<AoData*>(pPassThrough);
  const auto& val = object.Value();

  if (ocd->m_uiBranch == val.m_iObjectType)
    return true;

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

/// Shared data generated once per seed: tree structure and AO data needed to produce any LOD mesh.
struct ezKrautGeneratorResource::ezKrautSharedTreeData : public ezRefCounted
{
  Kraut::TreeStructure m_TreeStructure;
  ezKrautGeneratorResource::TreeStructureExtraData m_ExtraData;
  ezDynamicArray<ezDynamicArray<ezBoundingSphere>> m_OcclusionSpheres;
  ezDynamicOctree m_Octree;
  ezSharedPtr<ezKrautGeneratorResourceDescriptor> m_pDescriptor;
  ezUInt32 m_uiRandomSeed = 0;
};

/// Generates one LOD mesh asynchronously using already-computed shared tree data.
class ezKrautGeneratorResource::ezKrautLodGenerationTask final : public ezTask
{
public:
  ezKrautTreeResourceHandle m_hTree;
  ezUInt32 m_uiLodIndex = 0;
  ezSharedPtr<ezKrautGeneratorResource::ezKrautSharedTreeData> m_pSharedData;
  const ezKrautGeneratorResource* m_pGenerator = nullptr;

  virtual void Execute() override
  {
    if (HasBeenCanceled() || m_pSharedData == nullptr || m_pGenerator == nullptr)
    {
      SetLodFailed();
      return;
    }

    m_pGenerator->GenerateSingleLodMeshImmediate(m_pSharedData, m_uiLodIndex, m_hTree);
  }

private:
  void SetLodFailed()
  {
    ezResourceLock<ezKrautTreeResource> pTree(m_hTree, ezResourceAcquireMode::PointerOnly);
    if (pTree.IsValid())
      pTree->SetLodState(m_uiLodIndex, ezKrautLodState::NotGenerated);
  }
};

/// Generates tree structure + AO data asynchronously; does NOT generate any mesh.
///
/// Does not hold a resource handle to the generator to avoid a reference cycle:
/// the generator owns the SeedState which holds a shared_ptr to this task, so if this
/// task also held a handle to the generator the generator's reference count would never
/// reach zero and UnloadData() would never be called.
///
/// Instead, the descriptor is captured by shared_ptr at task-creation time, and the
/// generator raw pointer is used only for calling GenerateExtraData() (a pure computation
/// helper). Safety is guaranteed because UnloadData() calls WaitTillFinished before
/// the generator object is freed.
///
/// The result (m_pResult) is left in place after Execute() completes; the generator
/// picks it up in RequestLodMesh() on the next frame.
class ezKrautGeneratorResource::ezKrautBaseDataTask final : public ezTask
{
public:
  /// Raw pointer — valid for the duration of Execute() because UnloadData() calls
  /// WaitTillFinished before the generator object is freed.
  const ezKrautGeneratorResource* m_pGenerator = nullptr;
  ezSharedPtr<ezKrautGeneratorResourceDescriptor> m_pDesc;
  ezKrautTreeResourceHandle m_hTree;
  ezUInt32 m_uiRandomSeed = 0;
  ezSharedPtr<ezKrautGeneratorResource::ezKrautSharedTreeData> m_pResult;

  virtual void Execute() override
  {
    if (HasBeenCanceled() || m_pDesc == nullptr || m_pGenerator == nullptr)
      return;

    const auto& pDesc = m_pDesc;

    m_pResult = EZ_DEFAULT_NEW(ezKrautSharedTreeData);
    m_pResult->m_pDescriptor = pDesc;
    m_pResult->m_uiRandomSeed = m_uiRandomSeed;

    // Generate tree structure
    {
      Kraut::TreeStructureGenerator gen;
      gen.m_pTreeStructureDesc = &pDesc->m_TreeStructureDesc;
      gen.m_pTreeStructure = &m_pResult->m_TreeStructure;
      gen.GenerateTreeStructure(m_uiRandomSeed);
    }

    const float fWoodBendiness = 0.1f / pDesc->m_fTreeStiffness;
    const float fTwigBendiness = 0.1f * fWoodBendiness;

    // Generate extra data (bendiness, etc.)
    m_pGenerator->GenerateExtraData(m_pResult->m_ExtraData, pDesc->m_TreeStructureDesc, m_pResult->m_TreeStructure, m_uiRandomSeed, fWoodBendiness, fTwigBendiness);

    if (HasBeenCanceled())
      return;

    const auto bbox = m_pResult->m_TreeStructure.ComputeBoundingBox();

    // Generate AO data
    if (pDesc->m_fMinAmbientOcclusion < 1.0f && !bbox.IsInvalid())
    {
      ezBoundingBox bbox2 = ezBoundingBox::MakeFromMinMax(ToEzSwizzle(bbox.m_vMin), ToEzSwizzle(bbox.m_vMax));
      GenerateAmbientOcclusionSpheres(m_pResult->m_Octree, bbox2, m_pResult->m_OcclusionSpheres, m_pResult->m_TreeStructure);
    }

    if (HasBeenCanceled())
      return;

    // Compute bounds and details from tree structure (no mesh needed)
    {
      ezKrautTreeResourceDetails details;
      details.m_fStaticColliderRadius = pDesc->m_fStaticColliderRadius;
      details.m_sSurfaceResource = pDesc->m_sSurfaceResource;

      if (!bbox.IsInvalid())
      {
        ezBoundingBox bbox2 = ezBoundingBox::MakeFromMinMax(ToEzSwizzle(bbox.m_vMin), ToEzSwizzle(bbox.m_vMax));
        details.m_Bounds = ezBoundingBoxSphere::MakeFromBox(bbox2);
        details.m_vLeafCenter = details.m_Bounds.m_vCenter;

        // Refine leaf center estimate from leaf-type branch positions in tree structure
        ezBoundingBox leafBox = ezBoundingBox::MakeInvalid();
        for (ezUInt32 b = 0; b < m_pResult->m_TreeStructure.m_BranchStructures.size(); ++b)
        {
          const auto& branch = m_pResult->m_TreeStructure.m_BranchStructures[b];
          if (branch.m_Type < Kraut::BranchType::Twigs1)
            continue;
          for (ezUInt32 n = 0; n < branch.m_Nodes.size(); ++n)
            leafBox.ExpandToInclude(ToEzSwizzle(branch.m_Nodes[n].m_vPosition));
        }
        if (leafBox.IsValid())
          details.m_vLeafCenter = leafBox.GetCenter();
      }
      else
      {
        details.m_Bounds = ezBoundingBoxSphere::MakeInvalid();
        details.m_vLeafCenter = ezVec3::MakeZero();
      }

      // Build material list for the tree resource
      ezHybridArray<ezKrautTreeResourceDescriptor::MaterialData, 8> materials;
      for (const auto& srcMat : pDesc->m_Materials)
      {
        if (srcMat.m_hMaterial.IsValid())
        {
          auto& dstMat = materials.ExpandAndGetRef();
          dstMat.m_MaterialType = srcMat.m_MaterialType;
          dstMat.m_BranchType = srcMat.m_BranchType;
          dstMat.m_sMaterial = srcMat.m_hMaterial.GetResourceID();
        }
      }

      ezResourceLock<ezKrautTreeResource> pTree(m_hTree, ezResourceAcquireMode::PointerOnly);
      if (pTree.IsValid())
        pTree->SetDetails(details, materials);
    }

    // Result is left in m_pResult; the generator picks it up in RequestLodMesh() on the next frame.
  }
};

// SeedState destructor must be defined here, after ezKrautSharedTreeData/ezKrautBaseDataTask/
// ezKrautLodGenerationTask are complete, so that ezSharedPtr can call their destructors.
ezKrautGeneratorResource::SeedState::~SeedState() = default;

ezUInt32 ezKrautGeneratorResource::GetLodCount() const
{
  EZ_LOCK(m_DataMutex);
  if (m_pGeneratorDesc == nullptr)
    return 0;

  ezUInt32 uiCount = 0;
  for (ezUInt32 i = 0; i < 5; ++i)
  {
    if (m_pGeneratorDesc->m_LodDesc[i].m_Mode == Kraut::LodMode::Full)
      ++uiCount;
    else
      break;
  }
  return uiCount;
}

float ezKrautGeneratorResource::GetLodDistance(ezUInt32 uiLodIndex) const
{
  EZ_LOCK(m_DataMutex);
  if (m_pGeneratorDesc == nullptr || uiLodIndex >= 5)
    return 0.0f;
  return m_pGeneratorDesc->m_LodDesc[uiLodIndex].m_uiLodDistance * m_pGeneratorDesc->m_fLodDistanceScale * m_pGeneratorDesc->m_fUniformScaling;
}

ezKrautTreeResourceHandle ezKrautGeneratorResource::GetOrCreateTreeResource(ezUInt32 uiSeed)
{
  EZ_LOCK(m_GenerationMutex);

  SeedState* pState = m_SeedStates.GetValue(uiSeed);
  if (pState != nullptr && pState->m_hTree.IsValid())
    return pState->m_hTree;

  // Build resource ID from generator ID + change counter + seed
  ezStringBuilder sResourceID = GetResourceID();
  ezStringBuilder sResourceDesc = GetResourceDescription();
  sResourceID.AppendFormat(":{}@{}", GetCurrentResourceChangeCounter(), uiSeed);
  sResourceDesc.AppendFormat(":{}@{}", GetCurrentResourceChangeCounter(), uiSeed);

  // Check if a resource already exists (e.g. from a previous call)
  ezKrautTreeResourceHandle hTree = ezResourceManager::GetExistingResource<ezKrautTreeResource>(sResourceID);

  if (!hTree.IsValid())
  {
    // Build skeleton descriptor with LOD distances (snapshot descriptor under lock)
    ezSharedPtr<ezKrautGeneratorResourceDescriptor> pDesc;
    {
      EZ_LOCK(m_DataMutex);
      pDesc = m_pGeneratorDesc;
    }

    ezKrautTreeResourceDescriptor skelDesc;
    skelDesc.m_Details.m_Bounds = ezBoundingBoxSphere::MakeInvalid();

    if (pDesc != nullptr)
    {
      float fPrevMax = 0.0f;
      // LOD0 slot (full-detail, zero distances = never auto-selected)
      auto& lod0 = skelDesc.m_Lods.ExpandAndGetRef();
      lod0.m_LodType = ezKrautLodType::Mesh;
      lod0.m_fMinLodDistance = 0.0f;
      lod0.m_fMaxLodDistance = 0.0f;

      for (ezUInt32 i = 0; i < 5; ++i)
      {
        const auto& lodDesc = pDesc->m_LodDesc[i];
        if (lodDesc.m_Mode != Kraut::LodMode::Full)
          break;

        auto& lod = skelDesc.m_Lods.ExpandAndGetRef();
        lod.m_LodType = ezKrautLodType::Mesh;
        lod.m_fMinLodDistance = fPrevMax;
        lod.m_fMaxLodDistance = lodDesc.m_uiLodDistance * pDesc->m_fLodDistanceScale * pDesc->m_fUniformScaling;
        fPrevMax = lod.m_fMaxLodDistance;
      }
    }

    hTree = ezResourceManager::CreateResource<ezKrautTreeResource>(sResourceID, std::move(skelDesc), sResourceDesc);
  }

  SeedState& state = m_SeedStates[uiSeed];
  state.m_hTree = hTree;

  // Queue base data task if not already started.
  // Note: the task does NOT hold a resource handle to the generator (which would create a
  // reference cycle); instead it holds a raw pointer and a shared_ptr to the descriptor.
  if (state.m_pBaseDataTask == nullptr || state.m_pBaseDataTask->IsTaskFinished())
  {
    auto pTask = EZ_DEFAULT_NEW(ezKrautBaseDataTask);
    pTask->ConfigureTask("KrautBaseData", ezTaskNesting::Never);
    pTask->m_pGenerator = this;
    {
      EZ_LOCK(m_DataMutex);
      pTask->m_pDesc = m_pGeneratorDesc;
    }
    pTask->m_hTree = hTree;
    pTask->m_uiRandomSeed = uiSeed;
    state.m_pBaseDataTask = pTask;

    ezTaskSystem::StartSingleTask(pTask, ezTaskPriority::LongRunning);
  }

  return hTree;
}

bool ezKrautGeneratorResource::RequestLodMesh(ezKrautTreeResourceHandle hTree, ezUInt32 uiSeed, ezUInt32 uiLodIndex, bool bImmediate) const
{
  // Check if already ready (fast path, no generation mutex needed for the read)
  {
    ezResourceLock<ezKrautTreeResource> pTree(hTree, ezResourceAcquireMode::PointerOnly);
    if (pTree.IsValid() && pTree->GetLodState(uiLodIndex) == ezKrautLodState::Ready)
      return true;
  }

  EZ_LOCK(m_GenerationMutex);

  SeedState* pState = m_SeedStates.GetValue(uiSeed);
  if (pState == nullptr)
    return false;

  if (bImmediate)
  {
    // Ensure shared data is available synchronously
    if (pState->m_pSharedData == nullptr)
    {
      // Cancel async task if running, then generate synchronously
      if (pState->m_pBaseDataTask != nullptr && !pState->m_pBaseDataTask->IsTaskFinished())
        ezTaskSystem::CancelTask(pState->m_pBaseDataTask, ezOnTaskRunning::WaitTillFinished).IgnoreResult();

      ezSharedPtr<ezKrautGeneratorResourceDescriptor> pDesc;
      {
        EZ_LOCK(m_DataMutex);
        pDesc = m_pGeneratorDesc;
      }

      if (pDesc != nullptr)
        GenerateBaseDataImmediate(*pState, uiSeed, pDesc);
    }

    if (pState->m_pSharedData != nullptr)
    {
      // Cancel any pending async LOD task for this slot
      if (pState->m_PendingLodTasks[uiLodIndex] != nullptr && !pState->m_PendingLodTasks[uiLodIndex]->IsTaskFinished())
        ezTaskSystem::CancelTask(pState->m_PendingLodTasks[uiLodIndex], ezOnTaskRunning::WaitTillFinished).IgnoreResult();

      GenerateSingleLodMeshImmediate(pState->m_pSharedData, uiLodIndex, hTree);
    }

    ezResourceLock<ezKrautTreeResource> pTree(hTree, ezResourceAcquireMode::PointerOnly);
    return pTree.IsValid() && pTree->GetLodState(uiLodIndex) == ezKrautLodState::Ready;
  }

  // Async path: check if base data is ready.
  // The base data task no longer writes the result into SeedState directly (to avoid a
  // reference cycle via m_hGenerator). Pick it up here once the task signals completion.
  if (pState->m_pSharedData == nullptr)
  {
    if (pState->m_pBaseDataTask != nullptr && pState->m_pBaseDataTask->IsTaskFinished() && pState->m_pBaseDataTask->m_pResult != nullptr)
    {
      pState->m_pSharedData = pState->m_pBaseDataTask->m_pResult;
    }

    if (pState->m_pSharedData == nullptr)
      return false; // retry next frame once base data task completes
  }

  ezUInt32 uiNumLods = 0;
  {
    ezResourceLock<ezKrautTreeResource> pTree(hTree, ezResourceAcquireMode::PointerOnly);
    if (pTree.IsValid())
      uiNumLods = pTree->GetTreeLODs().GetCount();
  }

  if (uiNumLods == 0)
    return false;

  // Only one LOD task is queued at a time. If any task in the range is still in flight, wait.
  // This keeps the task queue short: the coarsest LOD appears quickly (NextFrame priority),
  // and finer LODs trickle in one by one as LongRunning tasks.
  for (ezUInt32 uiLod = uiLodIndex; uiLod < uiNumLods; ++uiLod)
  {
    const auto& pLodTask = pState->m_PendingLodTasks[uiLod];
    if (pLodTask != nullptr && !pLodTask->IsTaskFinished())
      return false; // a generation task is already in flight; wait for it to finish
  }

  // Find the coarsest LOD in [uiLodIndex, uiNumLods-1] that isn't Ready yet.
  ezUInt32 uiLodToQueue = ezInvalidIndex;
  {
    ezResourceLock<ezKrautTreeResource> pTree(hTree, ezResourceAcquireMode::PointerOnly);
    if (pTree.IsValid())
    {
      for (ezInt32 iLod = (ezInt32)uiNumLods - 1; iLod >= (ezInt32)uiLodIndex; --iLod)
      {
        if (pTree->GetLodState((ezUInt32)iLod) != ezKrautLodState::Ready)
        {
          uiLodToQueue = (ezUInt32)iLod;
          break;
        }
      }
    }
  }

  if (uiLodToQueue == ezInvalidIndex)
    return false; // all LODs already ready (shouldn't reach here; fast path above handles this)

  // Coarsest LOD gets NextFrame so a visible mesh appears as soon as possible.
  // All subsequent (finer) LODs are LongRunning since they take longer to generate.
  const ezTaskPriority::Enum priority = (uiLodToQueue == uiNumLods - 1)
                                          ? ezTaskPriority::NextFrame
                                          : ezTaskPriority::LongRunning;

  {
    ezResourceLock<ezKrautTreeResource> pTree(hTree, ezResourceAcquireMode::PointerOnly);
    if (pTree.IsValid())
      pTree->SetLodState(uiLodToQueue, ezKrautLodState::Generating);
  }

  auto pTask = EZ_DEFAULT_NEW(ezKrautLodGenerationTask);
  pTask->ConfigureTask("KrautLodGeneration", ezTaskNesting::Never);
  pTask->m_hTree = hTree;
  pTask->m_uiLodIndex = uiLodToQueue;
  pTask->m_pSharedData = pState->m_pSharedData;
  pTask->m_pGenerator = this;
  pState->m_PendingLodTasks[uiLodToQueue] = pTask;

  ezTaskSystem::StartSingleTask(pTask, priority);
  return false;
}

void ezKrautGeneratorResource::GenerateBaseDataImmediate(SeedState& state, ezUInt32 uiSeed, const ezSharedPtr<ezKrautGeneratorResourceDescriptor>& pDesc) const
{
  EZ_PROFILE_SCOPE("Kraut: GenerateBaseData");

  auto pData = EZ_DEFAULT_NEW(ezKrautSharedTreeData);
  pData->m_pDescriptor = pDesc;
  pData->m_uiRandomSeed = uiSeed;

  {
    Kraut::TreeStructureGenerator gen;
    gen.m_pTreeStructureDesc = &pDesc->m_TreeStructureDesc;
    gen.m_pTreeStructure = &pData->m_TreeStructure;
    gen.GenerateTreeStructure(uiSeed);
  }

  const float fWoodBendiness = 0.1f / pDesc->m_fTreeStiffness;
  const float fTwigBendiness = 0.1f * fWoodBendiness;
  GenerateExtraData(pData->m_ExtraData, pDesc->m_TreeStructureDesc, pData->m_TreeStructure, uiSeed, fWoodBendiness, fTwigBendiness);

  if (pDesc->m_fMinAmbientOcclusion < 1.0f)
  {
    auto bbox = pData->m_TreeStructure.ComputeBoundingBox();
    if (!bbox.IsInvalid())
    {
      ezBoundingBox bbox2 = ezBoundingBox::MakeFromMinMax(ToEzSwizzle(bbox.m_vMin), ToEzSwizzle(bbox.m_vMax));
      GenerateAmbientOcclusionSpheres(pData->m_Octree, bbox2, pData->m_OcclusionSpheres, pData->m_TreeStructure);
    }
  }

  state.m_pSharedData = pData;

  // Set bounds and details on the tree resource so GetLocalBounds() returns a valid box immediately.
  // This matches what ezKrautBaseDataTask::Execute() does in the async path.
  if (state.m_hTree.IsValid())
  {
    ezKrautTreeResourceDetails details;
    details.m_fStaticColliderRadius = pDesc->m_fStaticColliderRadius;
    details.m_sSurfaceResource = pDesc->m_sSurfaceResource;

    const auto bbox = pData->m_TreeStructure.ComputeBoundingBox();
    if (!bbox.IsInvalid())
    {
      ezBoundingBox bbox2 = ezBoundingBox::MakeFromMinMax(ToEzSwizzle(bbox.m_vMin), ToEzSwizzle(bbox.m_vMax));
      details.m_Bounds = ezBoundingBoxSphere::MakeFromBox(bbox2);
      details.m_vLeafCenter = details.m_Bounds.m_vCenter;

      ezBoundingBox leafBox = ezBoundingBox::MakeInvalid();
      for (ezUInt32 b = 0; b < pData->m_TreeStructure.m_BranchStructures.size(); ++b)
      {
        const auto& branch = pData->m_TreeStructure.m_BranchStructures[b];
        if (branch.m_Type < Kraut::BranchType::Twigs1)
          continue;
        for (ezUInt32 n = 0; n < branch.m_Nodes.size(); ++n)
          leafBox.ExpandToInclude(ToEzSwizzle(branch.m_Nodes[n].m_vPosition));
      }
      if (leafBox.IsValid())
        details.m_vLeafCenter = leafBox.GetCenter();
    }
    else
    {
      details.m_Bounds = ezBoundingBoxSphere::MakeInvalid();
      details.m_vLeafCenter = ezVec3::MakeZero();
    }

    ezHybridArray<ezKrautTreeResourceDescriptor::MaterialData, 8> materials;
    for (const auto& srcMat : pDesc->m_Materials)
    {
      if (srcMat.m_hMaterial.IsValid())
      {
        auto& dstMat = materials.ExpandAndGetRef();
        dstMat.m_MaterialType = srcMat.m_MaterialType;
        dstMat.m_BranchType = srcMat.m_BranchType;
        dstMat.m_sMaterial = srcMat.m_hMaterial.GetResourceID();
      }
    }

    ezResourceLock<ezKrautTreeResource> pTree(state.m_hTree, ezResourceAcquireMode::PointerOnly);
    if (pTree.IsValid())
      pTree->SetDetails(details, materials);
  }
}

static void GenerateLodMeshData(const ezKrautGeneratorResource::ezKrautSharedTreeData& sharedData, ezUInt32 uiLodIndex, ezKrautTreeResourceDescriptor::LodData& out_lodData, ezHybridArray<ezKrautTreeResourceDescriptor::MaterialData, 8>& inout_materials)
{
  const auto& pDesc = *sharedData.m_pDescriptor;
  const auto& treeStructure = sharedData.m_TreeStructure;
  const auto& extraData = sharedData.m_ExtraData;

  const float fWoodBendiness = 0.1f / pDesc.m_fTreeStiffness;
  const float fTwigBendiness = 0.1f * fWoodBendiness;

  Kraut::LodDesc fullDetailMeshDesc;
  const Kraut::LodDesc* pStructureLodDesc = nullptr;
  const Kraut::LodDesc* pMeshLodDesc = nullptr;

  float fMinLodDistance = 0.0f;
  float fMaxLodDistance = 0.0f;

  if (uiLodIndex == 0)
  {
    fullDetailMeshDesc.m_fTipDetail = 0.03f;
    fullDetailMeshDesc.m_fVertexRingDetail = 0.04f;
    pMeshLodDesc = &fullDetailMeshDesc;
    // pStructureLodDesc stays nullptr -> triggers GenerateFullDetailLod()
  }
  else
  {
    const Kraut::LodDesc& lodDesc = pDesc.m_LodDesc[uiLodIndex - 1];
    if (lodDesc.m_Mode != Kraut::LodMode::Full)
      return;

    pStructureLodDesc = &lodDesc;
    pMeshLodDesc = &lodDesc;

    // Compute distance range
    float fPrev = 0.0f;
    for (ezUInt32 i = 0; i < uiLodIndex - 1; ++i)
      fPrev = pDesc.m_LodDesc[i].m_uiLodDistance * pDesc.m_fLodDistanceScale * pDesc.m_fUniformScaling;
    fMinLodDistance = fPrev;
    fMaxLodDistance = lodDesc.m_uiLodDistance * pDesc.m_fLodDistanceScale * pDesc.m_fUniformScaling;
  }

  Kraut::TreeStructureLod treeLod;
  Kraut::TreeStructureLodGenerator lodGen;
  lodGen.m_pLodDesc = pStructureLodDesc;
  lodGen.m_pTreeStructure = &treeStructure;
  lodGen.m_pTreeStructureDesc = &pDesc.m_TreeStructureDesc;
  lodGen.m_pTreeStructureLod = &treeLod;
  lodGen.GenerateTreeStructureLod();

  Kraut::TreeMesh mesh;
  Kraut::TreeMeshGenerator meshGen;
  meshGen.m_pLodDesc = pMeshLodDesc;
  meshGen.m_pTreeStructure = lodGen.m_pTreeStructure;
  meshGen.m_pTreeStructureDesc = lodGen.m_pTreeStructureDesc;
  meshGen.m_pTreeStructureLod = lodGen.m_pTreeStructureLod;
  meshGen.m_pTreeMesh = &mesh;
  meshGen.GenerateTreeMesh();

  out_lodData.m_LodType = ezKrautLodType::Mesh;
  out_lodData.m_uiNumBones = treeLod.GetNumBones();
  out_lodData.m_fMinLodDistance = fMinLodDistance;
  out_lodData.m_fMaxLodDistance = fMaxLodDistance;

  const float fVertexScale = pDesc.m_fUniformScaling;

  ezUInt32 uiMaxTriangles = 0;
  for (ezUInt32 branchIdx = 0; branchIdx < mesh.m_BranchMeshes.size(); ++branchIdx)
    for (ezUInt32 geometryType = 0; geometryType < Kraut::BranchGeometryType::ENUM_COUNT; ++geometryType)
      uiMaxTriangles += mesh.m_BranchMeshes[branchIdx].m_Mesh[geometryType].m_Triangles.size();

  out_lodData.m_Vertices.Reserve(uiMaxTriangles * 3);
  out_lodData.m_Triangles.Reserve(uiMaxTriangles);

  // AO check lambda with a per-task ring buffer to cache recent results (thread-safe, no shared state).
  ezStaticRingBuffer<AoPositionResult, 16> aoResults;

  auto CheckOcclusion = [&](ezUInt32 uiBranch, const ezVec3& vPos) -> float
  {
    if (pDesc.m_fMinAmbientOcclusion >= 1.0f)
      return 1.0f;

    constexpr float fCluster = 4.0f;
    constexpr float fDivCluster = 1.0f / fCluster;

    ezVec3I32 ipos;
    ipos.x = ezMath::FloatToInt32(vPos.x * fCluster);
    ipos.y = ezMath::FloatToInt32(vPos.y * fCluster);
    ipos.z = ezMath::FloatToInt32(vPos.z * fCluster);

    for (ezUInt32 i = aoResults.GetCount(); i > 0; --i)
    {
      if (aoResults[i - 1].m_iPos == ipos)
        return aoResults[i - 1].m_fResult;
    }

    AoData ocd;
    ocd.m_pOcclusionSpheres = const_cast<ezDynamicArray<ezDynamicArray<ezBoundingSphere>>*>(&sharedData.m_OcclusionSpheres);
    ocd.m_fAO = 1.0f;
    ocd.m_uiBranch = uiBranch;
    ocd.m_vPosition.Set(ipos.x * fDivCluster, ipos.y * fDivCluster, ipos.z * fDivCluster);

    const_cast<ezDynamicOctree&>(sharedData.m_Octree).FindObjectsInRange(vPos, FindAoSpheres, &ocd);

    if (!aoResults.CanAppend())
      aoResults.PopFront();

    AoPositionResult e;
    e.m_iPos = ipos;
    e.m_fResult = ocd.m_fAO;
    aoResults.PushBack(e);

    return ocd.m_fAO;
  };

  for (ezUInt32 geometryType = 0; geometryType < Kraut::BranchGeometryType::ENUM_COUNT; ++geometryType)
  {
    for (ezUInt32 branchType = 0; branchType < Kraut::BranchType::ENUM_COUNT; ++branchType)
    {
      const ezUInt32 uiFirstTriangleIdx = out_lodData.m_Triangles.GetCount();

      for (ezUInt32 branchIdx = 0; branchIdx < mesh.m_BranchMeshes.size(); ++branchIdx)
      {
        if (branchType != treeStructure.m_BranchStructures[branchIdx].m_Type)
          continue;

        const auto& srcMesh = mesh.m_BranchMeshes[branchIdx].m_Mesh[geometryType];
        if (srcMesh.m_Triangles.empty())
          continue;

        aoResults.Clear();

        const ezUInt32 uiVertexOffset = out_lodData.m_Vertices.GetCount();

        for (ezUInt32 vidx = 0; vidx < srcMesh.m_Vertices.size(); ++vidx)
        {
          const auto& srcVtx = srcMesh.m_Vertices[vidx];
          auto& dstVtx = out_lodData.m_Vertices.ExpandAndGetRef();

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

          dstVtx.m_fAmbientOcclusion = ezMath::Clamp(dstVtx.m_fAmbientOcclusion, pDesc.m_fMinAmbientOcclusion, 1.0f);

          const auto& branchExtra = extraData.m_Branches[branchIdx];
          float fBranchDist = 0;

          if (srcVtx.m_uiBranchNodeIdx >= treeStructure.m_BranchStructures[branchIdx].m_Nodes.size())
          {
            fBranchDist = branchExtra.m_Nodes.PeekBack().m_fBendinessAlongBranch;
            ezUInt32 nodeIdx = srcVtx.m_uiBranchNodeIdx - (ezUInt32)treeStructure.m_BranchStructures[branchIdx].m_Nodes.size();
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
            dstVtx.m_fBendAndFlutterStrength = ezMath::Square(fBranchDist);
            dstVtx.m_uiBranchLevel = 0;
            dstVtx.m_uiFlutterPhase = 0;
            dstVtx.m_fAnchorBendStrength = 0;
            dstVtx.m_vBendAnchor.Set(0, 0, 0.05f); // must not be zero to avoid division by zero in the wind shader
          }
          else
          {
            dstVtx.m_fBendAndFlutterStrength = ezMath::Square(branchExtra.m_fBendinessToAnchor + fBranchDist);
            dstVtx.m_uiBranchLevel = 1;

            ezInt32 iMainBranchIdx = branchIdx;
            ezInt32 iTrunkIdx = branchExtra.m_iParentBranch;
            ezUInt32 uiTrunkNodeIdx = branchExtra.m_uiParentBranchNodeID;

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
          auto& dstTri = out_lodData.m_Triangles.ExpandAndGetRef();
          dstTri.m_uiVertexIndex[0] = uiVertexOffset + srcTri.m_uiVertexIDs[0];
          dstTri.m_uiVertexIndex[1] = uiVertexOffset + srcTri.m_uiVertexIDs[2];
          dstTri.m_uiVertexIndex[2] = uiVertexOffset + srcTri.m_uiVertexIDs[1];
        }
      }

      if (uiFirstTriangleIdx == out_lodData.m_Triangles.GetCount())
        continue;

      auto& subMesh = out_lodData.m_SubMeshes.ExpandAndGetRef();
      subMesh.m_uiFirstTriangle = static_cast<ezUInt16>(uiFirstTriangleIdx);
      subMesh.m_uiNumTriangles = static_cast<ezUInt16>(out_lodData.m_Triangles.GetCount() - uiFirstTriangleIdx);

      for (const auto& srcMat : pDesc.m_Materials)
      {
        if ((ezUInt32)srcMat.m_BranchType == branchType && (ezUInt32)srcMat.m_MaterialType == geometryType)
        {
          if (srcMat.m_hMaterial.IsValid())
          {
            subMesh.m_uiMaterialIndex = static_cast<ezUInt8>(inout_materials.GetCount());
            auto& mat = inout_materials.ExpandAndGetRef();
            mat.m_MaterialType = static_cast<ezKrautMaterialType>(geometryType);
            mat.m_BranchType = static_cast<ezKrautBranchType>(branchType);
            mat.m_sMaterial = srcMat.m_hMaterial.GetResourceID();
          }
          break;
        }
      }

      if (subMesh.m_uiMaterialIndex == 255)
        out_lodData.m_SubMeshes.PopBack();
    }
  }
}

void ezKrautGeneratorResource::GenerateSingleLodMeshImmediate(const ezSharedPtr<ezKrautSharedTreeData>& pSharedData, ezUInt32 uiLodIndex, ezKrautTreeResourceHandle hTree) const
{
  EZ_PROFILE_SCOPE("Kraut: GenerateSingleLodMesh");

  ezKrautTreeResourceDescriptor::LodData lodData;
  ezHybridArray<ezKrautTreeResourceDescriptor::MaterialData, 8> materials;

  // Snapshot the current material list from the tree resource (it was set during base data generation)
  {
    ezResourceLock<ezKrautTreeResource> pTree(hTree, ezResourceAcquireMode::PointerOnly);
    if (!pTree.IsValid())
      return;
    materials = pTree->GetMaterials();
  }

  // Generate LOD mesh data
  GenerateLodMeshData(*pSharedData, uiLodIndex, lodData, materials);

  // Push to tree resource
  {
    ezResourceLock<ezKrautTreeResource> pTree(hTree, ezResourceAcquireMode::PointerOnly);
    if (pTree.IsValid())
      pTree->SetLodMesh(uiLodIndex, lodData, materials);
  }
}


ezResourceLoadDesc ezKrautGeneratorResource::UnloadData(Unload WhatToUnload)
{
  // Cancel all pending generation tasks before clearing state, to avoid tasks writing into
  // freed resources after unload.
  {
    ezHashTable<ezUInt32, SeedState> copy;

    {
      EZ_LOCK(m_GenerationMutex);
      m_SeedStates.Swap(copy);
    }

    for (auto it = copy.GetIterator(); it.IsValid(); ++it)
    {
      auto& state = it.Value();
      if (state.m_pBaseDataTask != nullptr && !state.m_pBaseDataTask->IsTaskFinished())
      {
        // WaitTillFinished is required: the base data task holds a raw m_pGenerator pointer that
        // becomes dangling once UnloadData returns and the resource is freed. Waiting ensures the
        // task cannot access the generator after this point.
        ezTaskSystem::CancelTask(state.m_pBaseDataTask, ezOnTaskRunning::WaitTillFinished).IgnoreResult();
      }

      for (auto& pLodTask : state.m_PendingLodTasks)
      {
        if (pLodTask != nullptr && !pLodTask->IsTaskFinished())
        {
          // Same reasoning as above: LOD tasks also hold a raw m_pGenerator pointer.
          ezTaskSystem::CancelTask(pLodTask, ezOnTaskRunning::WaitTillFinished).IgnoreResult();
        }
      }
    }
  }

  {
    EZ_LOCK(m_DataMutex);
    m_pGeneratorDesc.Clear();
  }

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

  // the standard file reader writes the absolute file path into the stream
  ezStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  if (ezPathUtils::HasExtension(sAbsFilePath, ".tree"))
  {
    return res;
  }

  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  if (AssetHash.GetFileVersion() < 4)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  auto desc = EZ_DEFAULT_NEW(ezKrautGeneratorResourceDescriptor);
  if (desc->Deserialize(*Stream).Failed())
  {
    EZ_LOCK(m_DataMutex);
    m_pGeneratorDesc.Clear();
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  {
    EZ_LOCK(m_DataMutex);
    m_pGeneratorDesc = desc;
  }

  return res;
}

void ezKrautGeneratorResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = sizeof(*this);
  out_NewMemoryUsage.m_uiMemoryCPU = 0;

  auto desc = m_pGeneratorDesc;

  if (desc != nullptr)
  {
    out_NewMemoryUsage.m_uiMemoryCPU += sizeof(ezKrautGeneratorResourceDescriptor) + desc->m_Materials.GetHeapMemoryUsage();
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
  inout_stream.WriteVersion(8);

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
  inout_stream << m_fMinAmbientOcclusion;

  return EZ_SUCCESS;
}

ezResult ezKrautGeneratorResourceDescriptor::Deserialize(ezStreamReader& inout_stream)
{
  auto version = inout_stream.ReadVersion(8);

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
    ezTempHybridArray<ezUInt32, 16> dummy;
    EZ_SUCCEED_OR_RETURN(inout_stream.ReadArray(dummy));
  }

  if (version >= 7)
  {
    inout_stream >> m_fTreeStiffness;
  }

  if (version >= 8)
  {
    inout_stream >> m_fMinAmbientOcclusion;
  }

  return EZ_SUCCESS;
}


EZ_STATICLINK_FILE(KrautPlugin, KrautPlugin_Resources_KrautGeneratorResource);
