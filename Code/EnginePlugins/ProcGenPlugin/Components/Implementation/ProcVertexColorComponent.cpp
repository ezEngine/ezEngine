#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <ProcGenPlugin/Components/ProcVertexColorComponent.h>
#include <ProcGenPlugin/Components/ProcVolumeComponent.h>
#include <ProcGenPlugin/Tasks/VertexColorTask.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcVertexColorRenderData, 1, ezRTTIDefaultAllocator<ezProcVertexColorRenderData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezProcVertexColorRenderData::FillBatchIdAndSortingKey()
{
  FillBatchIdAndSortingKeyInternal(m_hVertexColorBuffer.GetInternalID().m_Data);
}

//////////////////////////////////////////////////////////////////////////

using namespace ezProcGenInternal;

ezProcVertexColorComponentManager::ezProcVertexColorComponentManager(ezWorld* pWorld)
  : ezComponentManager<ezProcVertexColorComponent, ezBlockStorageType::Compact>(pWorld)
{
}

ezProcVertexColorComponentManager::~ezProcVertexColorComponentManager() = default;

void ezProcVertexColorComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    constexpr ezUInt32 uiInitialBufferSize = 1024 * 16;

    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(ezUInt32);
    desc.m_uiTotalSize = uiInitialBufferSize;
    desc.m_ResourceAccess.m_bImmutable = false;

    if (ezGALDevice::GetDefaultDevice()->GetCapabilities().m_bSupportsTexelBuffer)
    {
      desc.m_BufferFlags = ezGALBufferUsageFlags::TexelBuffer | ezGALBufferUsageFlags::ShaderResource;
      desc.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
    }
    else
    {
      desc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ShaderResource;
    }
    m_hVertexColorBuffer = ezGALDevice::GetDefaultDevice()->CreateDynamicBuffer(desc, "ProcVertexColor");
  }

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezProcVertexColorComponentManager::UpdateVertexColors, this);
    desc.m_Phase = ezWorldUpdatePhase::PreAsync;
    desc.m_fPriority = 10000.0f;

    this->RegisterUpdateFunction(desc);
  }

  ezRenderWorld::GetExtractionEvent().AddEventHandler(ezMakeDelegate(&ezProcVertexColorComponentManager::OnExtractionEvent, this));

  ezResourceManager::GetResourceEvents().AddEventHandler(ezMakeDelegate(&ezProcVertexColorComponentManager::OnResourceEvent, this));

  // TODO: also do this in ezProcPlacementComponentManager
  ezProcVolumeComponent::GetAreaInvalidatedEvent().AddEventHandler(ezMakeDelegate(&ezProcVertexColorComponentManager::OnAreaInvalidated, this));
}

void ezProcVertexColorComponentManager::Deinitialize()
{
  ezGALDevice::GetDefaultDevice()->DestroyDynamicBuffer(m_hVertexColorBuffer);

  ezRenderWorld::GetExtractionEvent().RemoveEventHandler(ezMakeDelegate(&ezProcVertexColorComponentManager::OnExtractionEvent, this));

  ezResourceManager::GetResourceEvents().RemoveEventHandler(ezMakeDelegate(&ezProcVertexColorComponentManager::OnResourceEvent, this));

  ezProcVolumeComponent::GetAreaInvalidatedEvent().RemoveEventHandler(ezMakeDelegate(&ezProcVertexColorComponentManager::OnAreaInvalidated, this));

  SUPER::Deinitialize();
}

void ezProcVertexColorComponentManager::UpdateVertexColors(const ezWorldModule::UpdateContext& context)
{
  auto pBuffer = ezGALDevice::GetDefaultDevice()->GetDynamicBuffer(m_hVertexColorBuffer);

  // Buffer compaction
  {
    constexpr ezUInt32 uiMaxSteps = 16;

    ezHybridArray<ezGALDynamicBuffer::ChangedAllocation, uiMaxSteps> changedAllocations;
    pBuffer->RunCompactionSteps(changedAllocations, uiMaxSteps);

    for (const auto& changedAllocation : changedAllocations)
    {
      ezComponentHandle hComponent(ezComponentId(changedAllocation.m_uiUserData));
      ezProcVertexColorComponent* pComponent = nullptr;
      EZ_VERIFY(TryGetComponent(hComponent, pComponent), "Invalid component handle");

      pComponent->SetBufferOffset(changedAllocation.m_uiNewOffset);
    }
  }

  // New allocations first to ensure that the buffer is large enough and the memory is not invalidated by reallocations
  {
    for (const auto& componentToUpdate : m_ComponentsToUpdate)
    {
      ezProcVertexColorComponent* pComponent = nullptr;
      if (!TryGetComponent(componentToUpdate, pComponent))
        continue;

      if (!UpdateComponentOutputs(pComponent))
        continue;

      if (pComponent->m_uiBufferAccessData != 0)
        continue;

      auto pCpuMesh = pComponent->GetCpuMeshResource();
      if (pCpuMesh == nullptr)
        continue;

      const auto& meshBufferDescriptor = pCpuMesh->GetDescriptor().MeshBufferDesc();
      const ezUInt32 uiNumOutputs = pComponent->m_Outputs.GetCount();
      const ezUInt32 uiVertexColorCount = meshBufferDescriptor.GetVertexCount() * uiNumOutputs;

      const ezUInt32 uiOffset = pBuffer->Allocate(pComponent->GetHandle(), uiVertexColorCount);
      pComponent->SetBufferOffset(uiOffset);
      pComponent->m_hVertexColorBuffer = m_hVertexColorBuffer;
    }
  }

  // Update
  {
    m_UpdateTaskGroupID = ezTaskSystem::CreateTaskGroup(ezTaskPriority::EarlyThisFrame);

    for (const auto& componentToUpdate : m_ComponentsToUpdate)
    {
      ezProcVertexColorComponent* pComponent = nullptr;
      if (!TryGetComponent(componentToUpdate, pComponent))
        continue;

      UpdateComponentVertexColors(pComponent, pBuffer);
    }

    m_ComponentsToUpdate.Clear();

    ezTaskSystem::StartTaskGroup(m_UpdateTaskGroupID);
  }
}

bool ezProcVertexColorComponentManager::UpdateComponentOutputs(ezProcVertexColorComponent* pComponent)
{
  pComponent->m_Outputs.Clear();

  {
    ezResourceLock<ezProcGenGraphResource> pResource(pComponent->m_hResource, ezResourceAcquireMode::BlockTillLoaded);
    auto outputs = pResource->GetVertexColorOutputs();

    for (auto& outputDesc : pComponent->m_OutputDescs)
    {
      if (!outputDesc.m_sName.IsEmpty())
      {
        bool bOutputFound = false;
        for (auto& pOutput : outputs)
        {
          if (pOutput->m_sName == outputDesc.m_sName)
          {
            pComponent->m_Outputs.PushBack(pOutput);
            bOutputFound = true;
            break;
          }
        }

        if (!bOutputFound)
        {
          pComponent->m_Outputs.PushBack(nullptr);
          ezLog::Error("Vertex Color Output with name '{}' not found in Proc Gen Graph '{}'", outputDesc.m_sName, pResource->GetResourceID());
        }
      }
      else
      {
        pComponent->m_Outputs.PushBack(nullptr);
      }
    }
  }

  return pComponent->HasValidOutputs();
}

void ezProcVertexColorComponentManager::UpdateComponentVertexColors(ezProcVertexColorComponent* pComponent, ezGALDynamicBuffer* pBuffer)
{
  if (pComponent->m_uiBufferAccessData == 0 || !pComponent->HasValidOutputs())
    return;

  auto pCpuMesh = pComponent->GetCpuMeshResource();
  if (pCpuMesh == nullptr)
    return;

  if (m_uiNextTaskIndex >= m_UpdateTasks.GetCount())
  {
    m_UpdateTasks.PushBack(EZ_DEFAULT_NEW(ezProcGenInternal::VertexColorTask));
  }

  auto& pUpdateTask = m_UpdateTasks[m_uiNextTaskIndex];

  ezStringBuilder taskName = "VertexColor ";
  taskName.Append(pCpuMesh->GetResourceIdOrDescription());
  pUpdateTask->ConfigureTask(taskName, ezTaskNesting::Never);

  const auto& meshBufferDescriptor = pCpuMesh->GetDescriptor().MeshBufferDesc();
  auto vertexColorData = pBuffer->MapForWriting<ezUInt32>(pComponent->GetBufferOffset());

  ezHybridArray<ezProcVertexColorMapping, 2> outputMappings;
  for (auto& outputDesc : pComponent->m_OutputDescs)
  {
    outputMappings.PushBack(outputDesc.m_Mapping);
  }

  pUpdateTask->Prepare(*GetWorld(), meshBufferDescriptor, pComponent->GetOwner()->GetGlobalTransform(), pComponent->m_Outputs, outputMappings, vertexColorData);

  ezTaskSystem::AddTaskToGroup(m_UpdateTaskGroupID, pUpdateTask);

  ++m_uiNextTaskIndex;
}

void ezProcVertexColorComponentManager::OnExtractionEvent(const ezRenderWorldExtractionEvent& e)
{
  if (e.m_Type != ezRenderWorldExtractionEvent::Type::EndExtraction)
    return;

  ezTaskSystem::WaitForGroup(m_UpdateTaskGroupID);
  m_UpdateTaskGroupID.Invalidate();
  m_uiNextTaskIndex = 0;

  ezGALDevice::GetDefaultDevice()->GetDynamicBuffer(m_hVertexColorBuffer)->UploadChangesForNextFrame();
}

void ezProcVertexColorComponentManager::EnqueueUpdate(ezProcVertexColorComponent* pComponent)
{
  auto& hResource = pComponent->GetResource();
  if (!hResource.IsValid())
  {
    return;
  }

  if (!m_ComponentsToUpdate.Contains(pComponent->GetHandle()))
  {
    m_ComponentsToUpdate.PushBack(pComponent->GetHandle());
  }
}

void ezProcVertexColorComponentManager::RemoveComponent(ezProcVertexColorComponent* pComponent)
{
  m_ComponentsToUpdate.RemoveAndSwap(pComponent->GetHandle());

  if (pComponent->m_uiBufferAccessData != 0)
  {
    ezGALDevice::GetDefaultDevice()->GetDynamicBuffer(m_hVertexColorBuffer)->Deallocate(pComponent->GetBufferOffset());
    pComponent->m_uiBufferAccessData = 0;
  }
}

void ezProcVertexColorComponentManager::OnResourceEvent(const ezResourceEvent& resourceEvent)
{
  if (resourceEvent.m_Type != ezResourceEvent::Type::ResourceContentUnloading || resourceEvent.m_pResource->GetReferenceCount() == 0)
    return;

  if (auto pResource = ezDynamicCast<ezProcGenGraphResource*>(resourceEvent.m_pResource))
  {
    ezProcGenGraphResourceHandle hResource(pResource);

    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      if (it->m_hResource == hResource)
      {
        EnqueueUpdate(it);
      }
    }
  }
}

void ezProcVertexColorComponentManager::OnAreaInvalidated(const ezProcGenInternal::InvalidatedArea& area)
{
  if (area.m_pWorld != GetWorld())
    return;

  ezSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = ezDefaultSpatialDataCategories::RenderStatic.GetBitmask() | ezDefaultSpatialDataCategories::RenderDynamic.GetBitmask();

  GetWorld()->GetSpatialSystem()->FindObjectsInBox(area.m_Box, queryParams, [this](ezGameObject* pObject)
    {
    ezHybridArray<ezProcVertexColorComponent*, 8> components;
    pObject->TryGetComponentsOfBaseType(components);

    for (auto pComponent : components)
    {
      EnqueueUpdate(pComponent);
    }

    return ezVisitorExecution::Continue; });
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezProcVertexColorOutputDesc, ezNoBase, 1, ezRTTIDefaultAllocator<ezProcVertexColorOutputDesc>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Name", GetName, SetName),
    EZ_MEMBER_PROPERTY("Mapping", m_Mapping),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

void ezProcVertexColorOutputDesc::SetName(const char* szName)
{
  m_sName.Assign(szName);
}

static ezTypeVersion s_ProcVertexColorOutputDescVersion = 1;
ezResult ezProcVertexColorOutputDesc::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_ProcVertexColorOutputDescVersion);
  inout_stream << m_sName;
  EZ_SUCCEED_OR_RETURN(m_Mapping.Serialize(inout_stream));

  return EZ_SUCCESS;
}

ezResult ezProcVertexColorOutputDesc::Deserialize(ezStreamReader& inout_stream)
{
  /*ezTypeVersion version =*/inout_stream.ReadVersion(s_ProcVertexColorOutputDescVersion);
  inout_stream >> m_sName;
  EZ_SUCCEED_OR_RETURN(m_Mapping.Deserialize(inout_stream));

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezProcVertexColorComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Resource", GetResourceFile, SetResourceFile)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_ProcGen_Graph")),
    EZ_ARRAY_ACCESSOR_PROPERTY("OutputDescs", OutputDescs_GetCount, GetOutputDesc, SetOutputDesc, OutputDescs_Insert, OutputDescs_Remove),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgTransformChanged, OnTransformChanged)
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Construction/Procedural Generation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezProcVertexColorComponent::ezProcVertexColorComponent() = default;
ezProcVertexColorComponent::~ezProcVertexColorComponent() = default;

void ezProcVertexColorComponent::OnActivated()
{
  SUPER::OnActivated();

  auto pManager = static_cast<ezProcVertexColorComponentManager*>(GetOwningManager());
  pManager->EnqueueUpdate(this);

  if (GetUniqueID() != ezInvalidIndex)
  {
    GetOwner()->EnableStaticTransformChangesNotifications();
  }
}

void ezProcVertexColorComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  auto pManager = static_cast<ezProcVertexColorComponentManager*>(GetOwningManager());
  pManager->RemoveComponent(this);

  // Don't disable notifications as other components attached to the owner game object might need them too.
  // GetOwner()->DisableStaticTransformChangesNotifications();
}

void ezProcVertexColorComponent::SetResourceFile(ezStringView sFile)
{
  ezProcGenGraphResourceHandle hResource;

  if (!sFile.IsEmpty())
  {
    hResource = ezResourceManager::LoadResource<ezProcGenGraphResource>(sFile);
    ezResourceManager::PreloadResource(hResource);
  }

  SetResource(hResource);
}

ezStringView ezProcVertexColorComponent::GetResourceFile() const
{
  return m_hResource.GetResourceID();
}

void ezProcVertexColorComponent::SetResource(const ezProcGenGraphResourceHandle& hResource)
{
  m_hResource = hResource;

  if (IsActiveAndInitialized())
  {
    auto pManager = static_cast<ezProcVertexColorComponentManager*>(GetOwningManager());
    pManager->EnqueueUpdate(this);
  }
}

const ezProcVertexColorOutputDesc& ezProcVertexColorComponent::GetOutputDesc(ezUInt32 uiIndex) const
{
  return m_OutputDescs[uiIndex];
}

void ezProcVertexColorComponent::SetOutputDesc(ezUInt32 uiIndex, const ezProcVertexColorOutputDesc& outputDesc)
{
  m_OutputDescs.EnsureCount(uiIndex + 1);
  m_OutputDescs[uiIndex] = outputDesc;

  if (IsActiveAndInitialized())
  {
    auto pManager = static_cast<ezProcVertexColorComponentManager*>(GetOwningManager());
    pManager->EnqueueUpdate(this);
  }
}

void ezProcVertexColorComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  ezStreamWriter& s = inout_stream.GetStream();

  s << m_hResource;
  s.WriteArray(m_OutputDescs).IgnoreResult();
}

void ezProcVertexColorComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = inout_stream.GetStream();

  s >> m_hResource;
  if (uiVersion >= 2)
  {
    s.ReadArray(m_OutputDescs).IgnoreResult();
  }
  else
  {
    ezHybridArray<ezHashedString, 2> outputNames;
    s.ReadArray(outputNames).IgnoreResult();

    for (auto& outputName : outputNames)
    {
      auto& outputDesc = m_OutputDescs.ExpandAndGetRef();
      outputDesc.m_sName = outputName;
    }
  }
}

void ezProcVertexColorComponent::OnTransformChanged(ezMsgTransformChanged& ref_msg)
{
  auto pManager = static_cast<ezProcVertexColorComponentManager*>(GetOwningManager());
  pManager->EnqueueUpdate(this);
}

ezMeshRenderData* ezProcVertexColorComponent::CreateRenderData() const
{
  auto pRenderData = ezCreateRenderDataForThisFrame<ezProcVertexColorRenderData>(GetOwner());

  if (HasValidOutputs() && m_uiBufferAccessData != 0)
  {
    pRenderData->m_hVertexColorBuffer = m_hVertexColorBuffer;
    pRenderData->m_uiBufferAccessData = m_uiBufferAccessData;
  }

  return pRenderData;
}

ezUInt32 ezProcVertexColorComponent::OutputDescs_GetCount() const
{
  return m_OutputDescs.GetCount();
}

void ezProcVertexColorComponent::OutputDescs_Insert(ezUInt32 uiIndex, const ezProcVertexColorOutputDesc& outputDesc)
{
  m_OutputDescs.InsertAt(uiIndex, outputDesc);

  if (IsActiveAndInitialized())
  {
    auto pManager = static_cast<ezProcVertexColorComponentManager*>(GetOwningManager());
    pManager->EnqueueUpdate(this);
  }
}

void ezProcVertexColorComponent::OutputDescs_Remove(ezUInt32 uiIndex)
{
  m_OutputDescs.RemoveAtAndCopy(uiIndex);

  if (IsActiveAndInitialized())
  {
    auto pManager = static_cast<ezProcVertexColorComponentManager*>(GetOwningManager());
    pManager->EnqueueUpdate(this);
  }
}

bool ezProcVertexColorComponent::HasValidOutputs() const
{
  for (auto& pOutput : m_Outputs)
  {
    if (pOutput != nullptr && pOutput->m_pByteCode != nullptr)
    {
      return true;
    }
  }

  return false;
}

const ezCpuMeshResource* ezProcVertexColorComponent::GetCpuMeshResource() const
{
  const ezStringView sMesh = GetMesh().GetResourceID();
  if (sMesh.IsEmpty())
    return nullptr;

  ezCpuMeshResourceHandle hCpuMesh = ezResourceManager::LoadResource<ezCpuMeshResource>(sMesh);
  ezResourceLock<ezCpuMeshResource> pCpuMesh(hCpuMesh, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pCpuMesh.GetAcquireResult() != ezResourceAcquireResult::Final)
  {
    ezLog::Warning("Failed to retrieve CPU mesh '{}'", sMesh);
    return nullptr;
  }

  return pCpuMesh.GetPointer();
}

void ezProcVertexColorComponent::SetBufferOffset(ezUInt32 uiOffset)
{
  const ezUInt32 uiNumOutputs = m_Outputs.GetCount();
  m_uiBufferAccessData = uiNumOutputs << BUFFER_ACCESS_OFFSET_BITS | (uiOffset & ezProcVertexColorComponent::BUFFER_ACCESS_OFFSET_MASK);

  // Invalidate all cached render data so the new offset is propagated to the render data
  ezRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
}


EZ_STATICLINK_FILE(ProcGenPlugin, ProcGenPlugin_Components_Implementation_ProcVertexColorComponent);
