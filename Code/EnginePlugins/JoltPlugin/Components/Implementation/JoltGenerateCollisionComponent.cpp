#include <JoltPlugin/JoltPluginPCH.h>

#include <JoltPlugin/Actors/JoltStaticActorComponent.h>
#include <JoltPlugin/Components/JoltGenerateCollisionComponent.h>
#include <JoltPlugin/Resources/JoltMeshResourceWriter.h>

#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <RendererCore/Components/SplineComponent.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/SplineMeshComponent.h>

class SplineCollisionGenerationTask : public ezTask
{
public:
  SplineCollisionGenerationTask(const ezComponentHandle& hOwnerComponent, const ezStringView sCollisionMeshPath, const ezSpline& spline, ezArrayMap<float, float> distanceToKey, ezArrayPtr<ezCpuMeshResourceHandle> meshes,
    ezArrayPtr<ezVec2> scaleOffsets, float fLocalOffsetY, float fLocalOffsetZ)
    : m_hOwnerComponent(hOwnerComponent)
    , m_sCollisionMeshPath(sCollisionMeshPath)
    , m_Spline(spline)
    , m_DistanceToKey(distanceToKey)
    , m_Meshes(meshes)
    , m_ScaleOffsets(scaleOffsets)
    , m_fLocalOffsetY(fLocalOffsetY)
    , m_fLocalOffsetZ(fLocalOffsetZ)
  {
  }

  virtual void Execute() override
  {
    ezHybridArray<ezCpuMeshResource*, 16> cpuMeshes(ezFrameAllocator::GetCurrentAllocator());

    for (auto& hMeshCpu : m_Meshes)
    {
      ezCpuMeshResource* pMeshCpu = ezResourceManager::BeginAcquireResource<ezCpuMeshResource>(hMeshCpu, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
      EZ_ASSERT_DEV(pMeshCpu != nullptr, "Failed to load cpu mesh resource for spline mesh generation");
      cpuMeshes.PushBack(pMeshCpu);
    }

    EZ_SCOPE_EXIT(
      for (auto pMeshCpu : cpuMeshes) {
        ezResourceManager::EndAcquireResource(pMeshCpu);
      });

    ezMeshResourceDescriptor splineMeshDesc;
    if (ezSplineMeshComponent::GenerateSplineMeshDesc(m_Spline, m_DistanceToKey, cpuMeshes, m_ScaleOffsets, m_fLocalOffsetY, m_fLocalOffsetZ, splineMeshDesc).Failed())
      return;

    ezJoltMeshDesc joltMeshDesc;
    {
      const auto& splineMeshBufferDesc = splineMeshDesc.MeshBufferDesc();

      joltMeshDesc.m_Type = ezJoltMeshDesc::Type::Triangle;
      joltMeshDesc.m_Vertices = splineMeshBufferDesc.GetPositionData();

      const ezUInt32 uiNumIndices = splineMeshBufferDesc.GetPrimitiveCount() * 3;
      if (splineMeshBufferDesc.Uses32BitIndices())
      {
        ezArrayPtr<const ezUInt32> indices = ezMakeArrayPtr(reinterpret_cast<const ezUInt32*>(splineMeshBufferDesc.GetIndexBufferData().GetPtr()), uiNumIndices);
        joltMeshDesc.m_TriangleIndices = indices;
      }
      else
      {
        joltMeshDesc.m_TriangleIndices.SetCountUninitialized(uiNumIndices);
        const ezUInt16* pIndices = reinterpret_cast<const ezUInt16*>(splineMeshBufferDesc.GetIndexBufferData().GetPtr());

        for (ezUInt32 tri = 0; tri < uiNumIndices; ++tri)
        {
          joltMeshDesc.m_TriangleIndices[tri] = pIndices[tri];
        }
      }

      joltMeshDesc.m_TriangleSurfaceID.SetCount(splineMeshDesc.MeshBufferDesc().GetPrimitiveCount());
      for (auto& subMesh : splineMeshDesc.GetSubMeshes())
      {
        const ezUInt32 uiLastTriangle = subMesh.m_uiFirstPrimitive + subMesh.m_uiPrimitiveCount;
        const ezUInt16 uiSurface = subMesh.m_uiMaterialIndex;

        for (ezUInt32 t = subMesh.m_uiFirstPrimitive; t < uiLastTriangle; ++t)
        {
          joltMeshDesc.m_TriangleSurfaceID[t] = uiSurface;
        }
      }

      for (auto& mat : splineMeshDesc.GetMaterials())
      {
        joltMeshDesc.m_Surfaces.PushBack(mat.m_sPath);
      }
    }

    ezDeferredFileWriter fileWriter;
    fileWriter.SetOutput(m_sCollisionMeshPath);

    if (ezJoltMeshResourceWriter::WriteMeshResource(std::move(joltMeshDesc), fileWriter).Failed())
    {
      ezLog::Error("Could not write spline collision mesh file to '{}'", m_sCollisionMeshPath);
      return;
    }

    if (fileWriter.Close().Failed())
    {
      ezLog::Error("Could not write spline collision mesh file to '{}'", m_sCollisionMeshPath);
    }

    ezMsgComponentInternalTrigger msg;
    msg.m_sMessage.Assign("GenerationDone");

    ezWorld::GetWorld(m_hOwnerComponent)->PostMessage(m_hOwnerComponent, msg, ezTime::MakeZero());
  }

private:
  ezComponentHandle m_hOwnerComponent;

  ezString m_sCollisionMeshPath;
  ezSpline m_Spline;
  ezArrayMap<float, float> m_DistanceToKey;
  ezDynamicArray<ezCpuMeshResourceHandle> m_Meshes;
  ezDynamicArray<ezVec2> m_ScaleOffsets;
  float m_fLocalOffsetY = 0;
  float m_fLocalOffsetZ = 0;
};

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezJoltMeshMapping, ezNoBase, 1, ezRTTIDefaultAllocator<ezJoltMeshMapping>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_MEMBER_PROPERTY("RenderMesh", m_hRenderMesh)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Mesh_Static", ezDependencyFlags::None)),
    EZ_RESOURCE_MEMBER_PROPERTY("CollisionMesh", m_hCollisionMesh)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Jolt_Colmesh_Triangle", ezDependencyFlags::None)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezResult ezJoltMeshMapping::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream << m_hRenderMesh;
  inout_stream << m_hCollisionMesh;

  return EZ_SUCCESS;
}

ezResult ezJoltMeshMapping::Deserialize(ezStreamReader& inout_stream)
{
  inout_stream >> m_hRenderMesh;
  inout_stream >> m_hCollisionMesh;

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJoltGenerateCollisionComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_ACCESSOR_PROPERTY("MeshMappings", Reflection_GetMeshMappingCount, Reflection_GetMeshMapping, Reflection_SetMeshMapping, Reflection_InsertMeshMapping, Reflection_RemoveMeshMapping),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgGenerateSplineMeshCollision, OnMsgGenerateSplineMeshCollision),
    EZ_MESSAGE_HANDLER(ezMsgComponentInternalTrigger, OnMsgComponentInternalTrigger),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_FUNCTION_PROPERTY(OnObjectCreated),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics/Jolt/Misc"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezJoltGenerateCollisionComponent::ezJoltGenerateCollisionComponent() = default;
ezJoltGenerateCollisionComponent::~ezJoltGenerateCollisionComponent() = default;

void ezJoltGenerateCollisionComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s.WriteArray(m_MeshMappings).IgnoreResult();
  s << m_uiStableId;
}

void ezJoltGenerateCollisionComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s.ReadArray(m_MeshMappings).IgnoreResult();
  s >> m_uiStableId;
}

void ezJoltGenerateCollisionComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  ezTaskSystem::WaitForGroup(m_TaskGroupID);
}

void ezJoltGenerateCollisionComponent::Reflection_SetMeshMapping(ezUInt32 uiIndex, const ezJoltMeshMapping& mapping)
{
  m_MeshMappings.EnsureCount(uiIndex + 1);
  m_MeshMappings[uiIndex] = mapping;
}

void ezJoltGenerateCollisionComponent::Reflection_InsertMeshMapping(ezUInt32 uiIndex, const ezJoltMeshMapping& mapping)
{
  m_MeshMappings.InsertAt(uiIndex, mapping);
}

void ezJoltGenerateCollisionComponent::Reflection_RemoveMeshMapping(ezUInt32 uiIndex)
{
  m_MeshMappings.RemoveAtAndCopy(uiIndex);
}

ezCpuMeshResourceHandle ezJoltGenerateCollisionComponent::GetCollisionCpuMeshForRenderMesh(ezMeshResourceHandle hRenderMesh) const
{
  for (const auto& mapping : m_MeshMappings)
  {
    if (mapping.m_hRenderMesh == hRenderMesh)
    {
      ezResourceLock<ezJoltMeshResource> pCollisionMesh(mapping.m_hCollisionMesh, ezResourceAcquireMode::BlockTillLoaded);
      if (pCollisionMesh.GetAcquireResult() == ezResourceAcquireResult::Final)
      {
        return pCollisionMesh->ConvertToCpuMesh();
      }
    }
  }

  return ezCpuMeshResourceHandle();
}

void ezJoltGenerateCollisionComponent::OnObjectCreated(const ezAbstractObjectNode& node)
{
  m_uiStableId = ezHashingUtils::xxHash64(&node.GetGuid(), sizeof(ezUuid));
}

void ezJoltGenerateCollisionComponent::OnMsgGenerateSplineMeshCollision(ezMsgGenerateSplineMeshCollision& ref_msg)
{
  // Only generate in the editor
  if (GetUniqueID() == ezInvalidIndex)
    return;

  m_sCollisionMeshPath.Clear();

  ezHybridArray<ezCpuMeshResourceHandle, 16> cpuMeshes;
  ezHybridArray<ezVec2, 16> scaleOffsets;
  for (ezUInt32 i = 0; i < ref_msg.m_RenderMeshes.GetCount(); ++i)
  {
    auto hCpuMesh = GetCollisionCpuMeshForRenderMesh(ref_msg.m_RenderMeshes[i]);
    if (hCpuMesh.IsValid())
    {
      cpuMeshes.PushBack(hCpuMesh);
      scaleOffsets.PushBack(ref_msg.m_ScaleOffsets[i]);
    }
  }

  if (cpuMeshes.IsEmpty())
    return;

  const ezSplineComponent* pSplineComponent = nullptr;
  if (!GetWorld()->TryGetComponent(ref_msg.m_hSplineComponent, pSplineComponent))
    return;

  const ezUInt64 uiStableSplineId = ezHashingUtils::xxHash64(&pSplineComponent->GetUuid(), sizeof(ezUuid));

  ezStringBuilder sb;
  sb.SetFormat(":project/AssetCache/Generated/GenCol_{}_{}.ezJoltMesh", ezArgU(m_uiStableId, 16, true, 16, true), ezArgU(uiStableSplineId, 16, true, 16, true));
  m_sCollisionMeshPath.Assign(sb);

  auto pTask = EZ_DEFAULT_NEW(SplineCollisionGenerationTask, GetHandle(), sb, pSplineComponent->GetSpline(), pSplineComponent->GetDistanceToKeyRemapping(), cpuMeshes, scaleOffsets, ref_msg.m_fLocalOffsetY, ref_msg.m_fLocalOffsetZ);
  pTask->ConfigureTask("Generate Spline Collision Mesh", ezTaskNesting::Maybe);

  StartGenerateTask(pTask);
}

void ezJoltGenerateCollisionComponent::OnMsgComponentInternalTrigger(ezMsgComponentInternalTrigger& ref_msg)
{
  if (ref_msg.m_sMessage == "GenerationDone")
  {
    if (m_pNextGenerationTask != nullptr)
    {
      StartGenerateTask(std::move(m_pNextGenerationTask));
      m_pNextGenerationTask = nullptr;
    }
  }
}

void ezJoltGenerateCollisionComponent::StartGenerateTask(ezSharedPtr<ezTask>&& pTask)
{
  if (m_pGenerationTask != nullptr && !m_pGenerationTask->IsTaskFinished())
  {
    m_pNextGenerationTask = pTask;
    return;
  }

  m_pGenerationTask = pTask;
  m_TaskGroupID = ezTaskSystem::StartSingleTask(m_pGenerationTask, ezTaskPriority::LongRunning);
}

void ezJoltGenerateCollisionComponent::FinalizeGeneration()
{
  if (m_sCollisionMeshPath.IsEmpty())
    return;

  ezTaskSystem::WaitForGroup(m_TaskGroupID);

  ezGameObject* pObject = GetOwner();
  while (pObject->WasCreatedByPrefab())
  {
    pObject = pObject->GetParent();
  }

  ezJoltStaticActorComponent* pStaticActorComponent = nullptr;
  if (!pObject->TryGetComponentOfBaseType(pStaticActorComponent))
  {
    ezJoltStaticActorComponent::CreateComponent(pObject, pStaticActorComponent);
  }

  EZ_ASSERT_DEV(!pStaticActorComponent->WasCreatedByPrefab(), "Should have been handled above");
  ezJoltMeshResourceHandle hCollisionMesh = ezResourceManager::LoadResource<ezJoltMeshResource>(m_sCollisionMeshPath);
  pStaticActorComponent->SetMesh(hCollisionMesh);
}


EZ_STATICLINK_FILE(JoltPlugin, JoltPlugin_Components_Implementation_JoltGenerateCollisionComponent);
