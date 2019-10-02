#include <ProcGenPluginPCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <ProcGenPlugin/Components/ProcVertexColorComponent.h>
#include <ProcGenPlugin/Tasks/VertexColorTask.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Context/Context.h>
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

ezProcVertexColorComponentManager::~ezProcVertexColorComponentManager() {}

void ezProcVertexColorComponentManager::Initialize()
{
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

  ezRenderWorld::s_BeginRenderEvent.AddEventHandler(ezMakeDelegate(&ezProcVertexColorComponentManager::OnBeginRender, this));
  ezRenderWorld::s_EndExtractionEvent.AddEventHandler(ezMakeDelegate(&ezProcVertexColorComponentManager::OnEndExtraction, this));

  ezResourceManager::GetResourceEvents().AddEventHandler(ezMakeDelegate(&ezProcVertexColorComponentManager::OnResourceEvent, this));
}

void ezProcVertexColorComponentManager::Deinitialize()
{
  ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hVertexColorBuffer);
  m_hVertexColorBuffer.Invalidate();

  ezRenderWorld::s_BeginRenderEvent.RemoveEventHandler(ezMakeDelegate(&ezProcVertexColorComponentManager::OnBeginRender, this));
  ezRenderWorld::s_EndExtractionEvent.RemoveEventHandler(ezMakeDelegate(&ezProcVertexColorComponentManager::OnEndExtraction, this));

  ezResourceManager::GetResourceEvents().RemoveEventHandler(ezMakeDelegate(&ezProcVertexColorComponentManager::OnResourceEvent, this));
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

  {
    ezResourceLock<ezProcGenGraphResource> pResource(pComponent->m_hResource, ezResourceAcquireMode::BlockTillLoaded);
    auto outputs = pResource->GetVertexColorOutputs();

    for (auto& outputName : pComponent->m_OutputNames)
    {
      bool bOutputFound = false;

      if (!outputName.IsEmpty())
      {
        for (auto& pOutput : outputs)
        {
          if (pOutput->m_sName == outputName)
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
  pUpdateTask->SetTaskName(taskName);

  pUpdateTask->Prepare(mbDesc, pComponent->GetOwner()->GetGlobalTransform(), pComponent->m_Outputs,
    m_VertexColorData.GetArrayPtr().GetSubArray(uiBufferOffset, uiVertexColorCount));

  ezTaskSystem::AddTaskToGroup(m_UpdateTaskGroupID, pUpdateTask.Borrow());

  ++m_uiNextTaskIndex;
}

void ezProcVertexColorComponentManager::OnEndExtraction(ezUInt64 uiFrameCounter)
{
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

void ezProcVertexColorComponentManager::OnBeginRender(ezUInt64 uiFrameCounter)
{
  auto& dataCopy = m_DataCopy[ezRenderWorld::GetDataIndexForRendering()];
  if (!dataCopy.m_Data.IsEmpty())
  {
    ezGALContext* pGALContext = ezGALDevice::GetDefaultDevice()->GetPrimaryContext();

    ezUInt32 uiByteOffset = dataCopy.m_uiStart * sizeof(ezUInt32);
    pGALContext->UpdateBuffer(m_hVertexColorBuffer, uiByteOffset, dataCopy.m_Data.ToByteArray(), ezGALUpdateMode::CopyToTempStorage);

    dataCopy = DataCopy();
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

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezProcVertexColorComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
      EZ_ACCESSOR_PROPERTY("Resource", GetResourceFile, SetResourceFile)->AddAttributes(new ezAssetBrowserAttribute("ProcGen Graph")),
      EZ_ARRAY_ACCESSOR_PROPERTY("OutputNames", OutputNames_GetCount, GetOutputName, SetOutputName, OutputNames_Insert, OutputNames_Remove),
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

const char* ezProcVertexColorComponent::GetOutputName(ezUInt32 uiIndex) const
{
  if (uiIndex >= m_OutputNames.GetCount())
    return nullptr;

  return m_OutputNames[uiIndex];
}

void ezProcVertexColorComponent::SetOutputName(ezUInt32 uiIndex, const char* szName)
{
  m_OutputNames.EnsureCount(uiIndex + 1);
  m_OutputNames[uiIndex].Assign(szName);

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
  s.WriteArray(m_OutputNames);
}

void ezProcVertexColorComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_hResource;
  s.ReadArray(m_OutputNames);
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

ezUInt32 ezProcVertexColorComponent::OutputNames_GetCount() const
{
  return m_OutputNames.GetCount();
}

void ezProcVertexColorComponent::OutputNames_Insert(ezUInt32 uiIndex, const char* szName)
{
  ezHashedString sName;
  if (!ezStringUtils::IsNullOrEmpty(szName))
  {
    sName.Assign(szName);
  }
  m_OutputNames.Insert(sName, uiIndex);

  if (IsActiveAndInitialized())
  {
    auto pManager = static_cast<ezProcVertexColorComponentManager*>(GetOwningManager());
    pManager->EnqueueUpdate(this);
  }
}

void ezProcVertexColorComponent::OutputNames_Remove(ezUInt32 uiIndex)
{
  m_OutputNames.RemoveAtAndCopy(uiIndex);

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
