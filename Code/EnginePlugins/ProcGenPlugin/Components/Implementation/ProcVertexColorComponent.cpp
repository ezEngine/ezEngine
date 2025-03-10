#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <ProcGenPlugin/Components/ProcVertexColorComponent.h>
#include <ProcGenPlugin/Components/ProcVolumeComponent.h>
#include <ProcGenPlugin/Tasks/VertexColorTask.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/DynamicBuffer.h>

#include <RendererCore/../../../Data/Plugins/ProcGen/ProcGenVertexColor.h>

namespace
{
  static ezCpuMeshResourceHandle ExtractCpuMeshResource(const ezMeshComponentBase& meshComponent)
  {
    ezWorldGeoExtractionUtil::MeshObjectList meshObjects(ezFrameAllocator::GetCurrentAllocator());

    ezMsgExtractGeometry msg;
    msg.m_pMeshObjects = &meshObjects;

    meshComponent.SendMessage(msg);

    if (meshObjects.IsEmpty())
      return ezCpuMeshResourceHandle();

    return meshObjects[0].m_hMeshResource;
  }

  EZ_ALWAYS_INLINE static ezCustomInstanceDataOffset EncodeOffset(ezCustomInstanceDataOffset offset, ezUInt32 uiNumOutputs)
  {
    ezCustomInstanceDataOffset res;
    res.m_uiOffset = uiNumOutputs << VERTEX_COLOR_ACCESS_OFFSET_BITS | (offset.m_uiOffset & VERTEX_COLOR_ACCESS_OFFSET_MASK);
    return res;
  }
} // namespace

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
    desc.m_uiStructSize = sizeof(ezColorLinearUB);
    desc.m_uiTotalSize = uiInitialBufferSize * desc.m_uiStructSize;
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

    ezRenderDataManager* pRenderDataManager = GetWorld()->GetOrCreateModule<ezRenderDataManager>();
    m_uiCustomDataIndex = pRenderDataManager->RegisterCustomInstanceData(desc, "ProcVertexColors",
      // Before upload callback
      [this]()
      {
        ezTaskSystem::WaitForGroup(m_UpdateTaskGroupID);
        m_UpdateTaskGroupID.Invalidate();
        m_uiNextTaskIndex = 0;
      });
  }

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezProcVertexColorComponentManager::UpdateVertexColors, this);
    desc.m_Phase = ezWorldUpdatePhase::PreAsync;
    desc.m_fPriority = 10000.0f;

    this->RegisterUpdateFunction(desc);
  }

  ezResourceManager::GetResourceEvents().AddEventHandler(ezMakeDelegate(&ezProcVertexColorComponentManager::OnResourceEvent, this));

  ezProcVolumeComponent::GetAreaInvalidatedEvent().AddEventHandler(ezMakeDelegate(&ezProcVertexColorComponentManager::OnAreaInvalidated, this));
}

void ezProcVertexColorComponentManager::Deinitialize()
{
  ezResourceManager::GetResourceEvents().RemoveEventHandler(ezMakeDelegate(&ezProcVertexColorComponentManager::OnResourceEvent, this));

  ezProcVolumeComponent::GetAreaInvalidatedEvent().RemoveEventHandler(ezMakeDelegate(&ezProcVertexColorComponentManager::OnAreaInvalidated, this));

  SUPER::Deinitialize();
}

void ezProcVertexColorComponentManager::UpdateVertexColors(const ezWorldModule::UpdateContext& context)
{
  ezRenderDataManager* pRenderDataManager = GetWorld()->GetModule<ezRenderDataManager>();
  pRenderDataManager->CompactCustomInstanceDataBuffer(m_uiCustomDataIndex);

  m_UpdateContexts.SetCount(m_ComponentsToUpdate.GetCount());

  // New allocations first to ensure that the buffer is large enough and the memory is not invalidated by reallocations
  {
    for (ezUInt32 i = 0; i < m_ComponentsToUpdate.GetCount(); ++i)
    {
      ezProcVertexColorComponent* pComponent = nullptr;
      if (!TryGetComponent(m_ComponentsToUpdate[i], pComponent))
        continue;

      if (!UpdateComponentOutputs(*pComponent))
        continue;

      auto pMeshComponent = pComponent->GetMeshComponent();
      if (pMeshComponent == nullptr || pMeshComponent->GetCustomInstanceDataOffset().IsInvalidated() == false)
        continue;

      auto hCpuMesh = ExtractCpuMeshResource(*pMeshComponent);
      if (hCpuMesh.IsValid() == false)
        continue;

      ezResourceLock<ezCpuMeshResource> pCpuMesh(hCpuMesh, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pCpuMesh.GetAcquireResult() != ezResourceAcquireResult::Final)
        continue;

      const auto& meshBufferDescriptor = pCpuMesh->GetDescriptor().MeshBufferDesc();
      const ezUInt32 uiNumOutputs = pComponent->m_Outputs.GetCount();
      const ezUInt32 uiVertexColorCount = meshBufferDescriptor.GetVertexCount() * uiNumOutputs;

      ezCustomInstanceDataOffset offset = pMeshComponent->GetCustomInstanceDataOffset();
      ezGALDynamicBufferHandle hVertexColorsBuffer;
      pRenderDataManager->GetOrCreateCustomInstanceData<ezColorLinearUB>(m_uiCustomDataIndex, pComponent, hVertexColorsBuffer, offset, uiVertexColorCount);
      pComponent->m_CustomInstanceDataOffset = offset;
      pMeshComponent->SetCustomInstanceData(EncodeOffset(offset, uiNumOutputs), hVertexColorsBuffer);

      auto& updateContext = m_UpdateContexts[i];
      updateContext.m_pComponent = pComponent;
      updateContext.m_hCpuMesh = hCpuMesh;
      updateContext.m_uiVertexColorOffset = offset.m_uiOffset;
    }
  }

  // Update
  if (m_ComponentsToUpdate.IsEmpty() == false)
  {
    m_UpdateTaskGroupID = ezTaskSystem::CreateTaskGroup(ezTaskPriority::EarlyThisFrame);

    ezGALDynamicBuffer* pBuffer = ezGALDevice::GetDefaultDevice()->GetDynamicBuffer(pRenderDataManager->GetCustomInstanceDataBuffer(m_uiCustomDataIndex));
    EZ_ASSERT_DEV(pBuffer != nullptr, "Vertex color buffer not found.");

    for (const auto& updateContext : m_UpdateContexts)
    {
      if (updateContext.m_pComponent == nullptr)
        continue;

      UpdateComponentVertexColors(updateContext, *pBuffer);
    }

    m_ComponentsToUpdate.Clear();

    ezTaskSystem::StartTaskGroup(m_UpdateTaskGroupID);
  }
}

bool ezProcVertexColorComponentManager::UpdateComponentOutputs(ezProcVertexColorComponent& component)
{
  component.m_Outputs.Clear();

  {
    ezResourceLock<ezProcGenGraphResource> pResource(component.m_hResource, ezResourceAcquireMode::BlockTillLoaded);
    auto outputs = pResource->GetVertexColorOutputs();

    for (auto& outputDesc : component.m_OutputDescs)
    {
      if (!outputDesc.m_sName.IsEmpty())
      {
        bool bOutputFound = false;
        for (auto& pOutput : outputs)
        {
          if (pOutput->m_sName == outputDesc.m_sName)
          {
            component.m_Outputs.PushBack(pOutput);
            bOutputFound = true;
            break;
          }
        }

        if (!bOutputFound)
        {
          component.m_Outputs.PushBack(nullptr);
          ezLog::Error("Vertex Color Output with name '{}' not found in Proc Gen Graph '{}'", outputDesc.m_sName, pResource->GetResourceID());
        }
      }
      else
      {
        component.m_Outputs.PushBack(nullptr);
      }
    }
  }

  return component.HasValidOutputs();
}

void ezProcVertexColorComponentManager::UpdateComponentVertexColors(const UpdateContext& context, ezGALDynamicBuffer& buffer)
{
  auto pComponent = context.m_pComponent;
  if (pComponent->HasValidOutputs() == false)
    return;

  ezResourceLock<ezCpuMeshResource> pCpuMesh(context.m_hCpuMesh, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pCpuMesh.GetAcquireResult() != ezResourceAcquireResult::Final)
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
  const auto meshBBox = pCpuMesh->GetDescriptor().GetBounds().GetBox();
  auto vertexColorData = buffer.MapForWriting<ezColorLinearUB>(context.m_uiVertexColorOffset);

  ezHybridArray<ezProcVertexColorMapping, 2> outputMappings;
  for (auto& outputDesc : pComponent->m_OutputDescs)
  {
    outputMappings.PushBack(outputDesc.m_Mapping);
  }

  pUpdateTask->Prepare(*GetWorld(), meshBufferDescriptor, pComponent->GetOwner()->GetGlobalTransform(), meshBBox, pComponent->m_Outputs, outputMappings, vertexColorData);

  ezTaskSystem::AddTaskToGroup(m_UpdateTaskGroupID, pUpdateTask);

  ++m_uiNextTaskIndex;
}

void ezProcVertexColorComponentManager::EnqueueUpdate(ezProcVertexColorComponent& component)
{
  auto& hResource = component.GetResource();
  if (!hResource.IsValid())
  {
    return;
  }

  if (!m_ComponentsToUpdate.Contains(component.GetHandle()))
  {
    m_ComponentsToUpdate.PushBack(component.GetHandle());
  }
}

void ezProcVertexColorComponentManager::RemoveComponent(ezProcVertexColorComponent& component)
{
  m_ComponentsToUpdate.RemoveAndSwap(component.GetHandle());

  ezRenderDataManager* pRenderDataManager = GetWorld()->GetModule<ezRenderDataManager>();
  pRenderDataManager->DeleteCustomInstanceData(m_uiCustomDataIndex, component.m_CustomInstanceDataOffset);
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
        EnqueueUpdate(*it);
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

  GetWorld()->GetSpatialSystem()->FindObjectsInBox(area.m_Box, queryParams,
    [this](ezGameObject* pObject)
    {
      ezHybridArray<ezProcVertexColorComponent*, 4> components;
      pObject->TryGetComponentsOfBaseType(components);

      for (auto pComponent : components)
      {
        EnqueueUpdate(*pComponent);
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
    EZ_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new ezDynamicStringEnumAttribute("ProcGenOutputNameEnum")),
    EZ_MEMBER_PROPERTY("Mapping", m_Mapping),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

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
  pManager->EnqueueUpdate(*this);

  if (GetUniqueID() != ezInvalidIndex)
  {
    GetOwner()->EnableStaticTransformChangesNotifications();
  }
}

void ezProcVertexColorComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  auto pManager = static_cast<ezProcVertexColorComponentManager*>(GetOwningManager());
  pManager->RemoveComponent(*this);

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
    pManager->EnqueueUpdate(*this);
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
    pManager->EnqueueUpdate(*this);
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
  pManager->EnqueueUpdate(*this);
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
    pManager->EnqueueUpdate(*this);
  }
}

void ezProcVertexColorComponent::OutputDescs_Remove(ezUInt32 uiIndex)
{
  m_OutputDescs.RemoveAtAndCopy(uiIndex);

  if (IsActiveAndInitialized())
  {
    auto pManager = static_cast<ezProcVertexColorComponentManager*>(GetOwningManager());
    pManager->EnqueueUpdate(*this);
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

ezMeshComponentBase* ezProcVertexColorComponent::GetMeshComponent()
{
  ezMeshComponentBase* pMeshComponent = nullptr;
  bool _ = GetOwner()->TryGetComponentOfBaseType(pMeshComponent);
  return pMeshComponent;
}


EZ_STATICLINK_FILE(ProcGenPlugin, ProcGenPlugin_Components_Implementation_ProcVertexColorComponent);
