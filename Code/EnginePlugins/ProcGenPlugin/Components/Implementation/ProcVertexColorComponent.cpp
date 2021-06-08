#include <ProcGenPluginPCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <ProcGenPlugin/Components/ProcVertexColorComponent.h>
#include <ProcGenPlugin/Components/ProcVolumeComponent.h>
#include <ProcGenPlugin/Tasks/VertexColorTask.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/Pass.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcVertexColorRenderData, 1, ezRTTIDefaultAllocator<ezProcVertexColorRenderData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezProcVertexColorRenderData::FillBatchIdAndSortingKey()
{
  FillBatchIdAndSortingKeyInternal(m_hVertexColorBuffer.GetInternalID().m_Data);
}

//////////////////////////////////////////////////////////////////////////

enum
{
  BUFFER_ACCESS_OFFSET_BITS = 28,
  BUFFER_ACCESS_OFFSET_MASK = (1 << BUFFER_ACCESS_OFFSET_BITS) - 1,

  VERTEX_COLOR_BUFFER_SIZE = 1024 * 1024
};

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
    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(ezUInt32);
    desc.m_uiTotalSize = desc.m_uiStructSize * VERTEX_COLOR_BUFFER_SIZE;
    desc.m_bAllowShaderResourceView = true;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hVertexColorBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(desc);

    m_VertexColorData.SetCountUninitialized(VERTEX_COLOR_BUFFER_SIZE);
  }

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezProcVertexColorComponentManager::UpdateVertexColors, this);
    desc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PreAsync;
    desc.m_fPriority = 10000.0f;

    this->RegisterUpdateFunction(desc);
  }

  ezRenderWorld::GetRenderEvent().AddEventHandler(ezMakeDelegate(&ezProcVertexColorComponentManager::OnRenderEvent, this));
  ezRenderWorld::GetExtractionEvent().AddEventHandler(ezMakeDelegate(&ezProcVertexColorComponentManager::OnExtractionEvent, this));

  ezResourceManager::GetResourceEvents().AddEventHandler(ezMakeDelegate(&ezProcVertexColorComponentManager::OnResourceEvent, this));

  ezProcVolumeComponent::GetAreaInvalidatedEvent().AddEventHandler(ezMakeDelegate(&ezProcVertexColorComponentManager::OnAreaInvalidated, this));
}

void ezProcVertexColorComponentManager::Deinitialize()
{
  ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hVertexColorBuffer);
  m_hVertexColorBuffer.Invalidate();

  ezRenderWorld::GetRenderEvent().RemoveEventHandler(ezMakeDelegate(&ezProcVertexColorComponentManager::OnRenderEvent, this));
  ezRenderWorld::GetExtractionEvent().RemoveEventHandler(ezMakeDelegate(&ezProcVertexColorComponentManager::OnExtractionEvent, this));

  ezResourceManager::GetResourceEvents().RemoveEventHandler(ezMakeDelegate(&ezProcVertexColorComponentManager::OnResourceEvent, this));

  ezProcVolumeComponent::GetAreaInvalidatedEvent().RemoveEventHandler(ezMakeDelegate(&ezProcVertexColorComponentManager::OnAreaInvalidated, this));

  SUPER::Deinitialize();
}

void ezProcVertexColorComponentManager::UpdateVertexColors(const ezWorldModule::UpdateContext& context)
{
  m_UpdateTaskGroupID = ezTaskSystem::CreateTaskGroup(ezTaskPriority::EarlyThisFrame);
  m_ModifiedDataRange.Reset();

  for (const auto& componentToUpdate : m_ComponentsToUpdate)
  {
    ezProcVertexColorComponent* pComponent = nullptr;
    if (!TryGetComponent(componentToUpdate, pComponent))
      continue;

    UpdateComponentVertexColors(pComponent);

    // Invalidate all cached render data so the new buffer handle and offset are propagated to the render data
    ezRenderWorld::DeleteCachedRenderData(pComponent->GetOwner()->GetHandle(), pComponent->GetHandle());
  }

  m_ComponentsToUpdate.Clear();

  ezTaskSystem::StartTaskGroup(m_UpdateTaskGroupID);
}

void ezProcVertexColorComponentManager::UpdateComponentVertexColors(ezProcVertexColorComponent* pComponent)
{
  pComponent->m_Outputs.Clear();
  ezHybridArray<ezProcVertexColorMapping, 2> outputMappings;

  {
    ezResourceLock<ezProcGenGraphResource> pResource(pComponent->m_hResource, ezResourceAcquireMode::BlockTillLoaded);
    auto outputs = pResource->GetVertexColorOutputs();

    for (auto& outputDesc : pComponent->m_OutputDescs)
    {
      bool bOutputFound = false;

      if (!outputDesc.m_sName.IsEmpty())
      {
        for (auto& pOutput : outputs)
        {
          if (pOutput->m_sName == outputDesc.m_sName)
          {
            pComponent->m_Outputs.PushBack(pOutput);
            bOutputFound = true;
            break;
          }
        }
      }

      if (!bOutputFound)
      {
        pComponent->m_Outputs.PushBack(nullptr);
      }

      outputMappings.PushBack(outputDesc.m_Mapping);
    }
  }

  if (!pComponent->HasValidOutputs())
    return;

  const char* szMesh = pComponent->GetMeshFile();
  if (ezStringUtils::IsNullOrEmpty(szMesh))
    return;

  ezCpuMeshResourceHandle hCpuMesh = ezResourceManager::LoadResource<ezCpuMeshResource>(szMesh);
  ezResourceLock<ezCpuMeshResource> pCpuMesh(hCpuMesh, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pCpuMesh.GetAcquireResult() != ezResourceAcquireResult::Final)
  {
    ezLog::Warning("Failed to retrieve CPU mesh '{}'", szMesh);
    return;
  }

  const auto& mbDesc = pCpuMesh->GetDescriptor().MeshBufferDesc();
  const ezUInt32 uiNumOutputs = pComponent->m_Outputs.GetCount();
  const ezUInt32 uiVertexColorCount = mbDesc.GetVertexCount() * uiNumOutputs;

  pComponent->m_hVertexColorBuffer = m_hVertexColorBuffer;

  if (pComponent->m_uiBufferAccessData == 0)
  {
    pComponent->m_uiBufferAccessData = (uiNumOutputs << BUFFER_ACCESS_OFFSET_BITS) | m_uiCurrentBufferOffset;
    m_uiCurrentBufferOffset += uiVertexColorCount;
  }

  const ezUInt32 uiBufferOffset = pComponent->m_uiBufferAccessData & BUFFER_ACCESS_OFFSET_MASK;
  m_ModifiedDataRange.SetToIncludeRange(uiBufferOffset, uiBufferOffset + uiVertexColorCount - 1);

  if (m_uiNextTaskIndex >= m_UpdateTasks.GetCount())
  {
    m_UpdateTasks.PushBack(EZ_DEFAULT_NEW(ezProcGenInternal::VertexColorTask));
  }

  auto& pUpdateTask = m_UpdateTasks[m_uiNextTaskIndex];

  ezStringBuilder taskName = "VertexColor ";
  taskName.Append(pCpuMesh->GetResourceDescription().GetView());
  pUpdateTask->ConfigureTask(taskName, ezTaskNesting::Never);

  pUpdateTask->Prepare(*GetWorld(), mbDesc, pComponent->GetOwner()->GetGlobalTransform(), pComponent->m_Outputs, outputMappings, m_VertexColorData.GetArrayPtr().GetSubArray(uiBufferOffset, uiVertexColorCount));

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

  if (m_ModifiedDataRange.IsValid())
  {
    auto& dataCopy = m_DataCopy[ezRenderWorld::GetDataIndexForExtraction()];
    dataCopy.m_Data = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezUInt32, m_ModifiedDataRange.GetCount());
    dataCopy.m_Data.CopyFrom(m_VertexColorData.GetArrayPtr().GetSubArray(m_ModifiedDataRange.m_uiMin, m_ModifiedDataRange.GetCount()));
    dataCopy.m_uiStart = m_ModifiedDataRange.m_uiMin;
  }
}

void ezProcVertexColorComponentManager::OnRenderEvent(const ezRenderWorldRenderEvent& e)
{
  if (e.m_Type != ezRenderWorldRenderEvent::Type::BeginRender)
    return;

  auto& dataCopy = m_DataCopy[ezRenderWorld::GetDataIndexForRendering()];
  if (!dataCopy.m_Data.IsEmpty())
  {
    ezGALDevice* pGALDevice = ezGALDevice::GetDefaultDevice();
    ezGALPass* pGALPass = pGALDevice->BeginPass("ProcVertexUpdate");
    ezGALComputeCommandEncoder* pGALCommandEncoder = pGALPass->BeginCompute();

    ezUInt32 uiByteOffset = dataCopy.m_uiStart * sizeof(ezUInt32);
    pGALCommandEncoder->UpdateBuffer(m_hVertexColorBuffer, uiByteOffset, dataCopy.m_Data.ToByteArray(), ezGALUpdateMode::CopyToTempStorage);

    dataCopy = DataCopy();

    pGALPass->EndCompute(pGALCommandEncoder);
    pGALDevice->EndPass(pGALPass);
  }
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
    /// \todo compact buffer somehow?

    pComponent->m_uiBufferAccessData = 0;
  }
}

void ezProcVertexColorComponentManager::OnResourceEvent(const ezResourceEvent& resourceEvent)
{
  if (resourceEvent.m_Type != ezResourceEvent::Type::ResourceContentUnloading)
    return;

  if (auto pResource = ezDynamicCast<const ezProcGenGraphResource*>(resourceEvent.m_pResource))
  {
    ezProcGenGraphResourceHandle hResource = pResource->GetResourceHandle();

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

  ezUInt32 category = ezDefaultSpatialDataCategories::RenderStatic.GetBitmask() | ezDefaultSpatialDataCategories::RenderDynamic.GetBitmask();
  GetWorld()->GetSpatialSystem()->FindObjectsInBox(area.m_Box, category, [this](ezGameObject* pObject) {
    ezHybridArray<ezProcVertexColorComponent*, 8> components;
    pObject->TryGetComponentsOfBaseType(components);

    for (auto pComponent : components)
    {
      EnqueueUpdate(pComponent);
    }

    return ezVisitorExecution::Continue;
  });
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
ezResult ezProcVertexColorOutputDesc::Serialize(ezStreamWriter& stream) const
{
  stream.WriteVersion(s_ProcVertexColorOutputDescVersion);
  stream << m_sName;
  EZ_SUCCEED_OR_RETURN(m_Mapping.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezProcVertexColorOutputDesc::Deserialize(ezStreamReader& stream)
{
  /*ezTypeVersion version =*/stream.ReadVersion(s_ProcVertexColorOutputDescVersion);
  stream >> m_sName;
  EZ_SUCCEED_OR_RETURN(m_Mapping.Deserialize(stream));

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezProcVertexColorComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Resource", GetResourceFile, SetResourceFile)->AddAttributes(new ezAssetBrowserAttribute("ProcGen Graph")),
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
    new ezCategoryAttribute("Procedural Generation"),
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

  GetOwner()->EnableStaticTransformChangesNotifications();
}

void ezProcVertexColorComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  auto pManager = static_cast<ezProcVertexColorComponentManager*>(GetOwningManager());
  pManager->RemoveComponent(this);

  // Don't disable notifications as other components attached to the owner game object might need them too.
  // GetOwner()->DisableStaticTransformChangesNotifications();
}

void ezProcVertexColorComponent::SetResourceFile(const char* szFile)
{
  ezProcGenGraphResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezProcGenGraphResource>(szFile);
    ezResourceManager::PreloadResource(hResource);
  }

  SetResource(hResource);
}

const char* ezProcVertexColorComponent::GetResourceFile() const
{
  if (!m_hResource.IsValid())
    return "";

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

void ezProcVertexColorComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  ezStreamWriter& s = stream.GetStream();

  s << m_hResource;
  s.WriteArray(m_OutputDescs).IgnoreResult();
}

void ezProcVertexColorComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

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

void ezProcVertexColorComponent::OnTransformChanged(ezMsgTransformChanged& msg)
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
  m_OutputDescs.Insert(outputDesc, uiIndex);

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
