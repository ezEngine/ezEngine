#include <ProcGenPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <ProcGenPlugin/Components/ProcVertexColorComponent.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
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

  ezResourceManager::s_ResourceEvents.AddEventHandler(ezMakeDelegate(&ezProcVertexColorComponentManager::OnResourceEvent, this));
}

void ezProcVertexColorComponentManager::Deinitialize()
{
  ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hVertexColorBuffer);
  m_hVertexColorBuffer.Invalidate();

  ezRenderWorld::s_BeginRenderEvent.RemoveEventHandler(ezMakeDelegate(&ezProcVertexColorComponentManager::OnBeginRender, this));
  ezRenderWorld::s_EndExtractionEvent.RemoveEventHandler(ezMakeDelegate(&ezProcVertexColorComponentManager::OnEndExtraction, this));

  ezResourceManager::s_ResourceEvents.RemoveEventHandler(ezMakeDelegate(&ezProcVertexColorComponentManager::OnResourceEvent, this));
}

void ezProcVertexColorComponentManager::UpdateVertexColors(const ezWorldModule::UpdateContext& context)
{
  ezExpressionVM vm;

  for (const auto& componentToUpdate : m_ComponentsToUpdate)
  {
    ezProcVertexColorComponent* pComponent = nullptr;
    if (!TryGetComponent(componentToUpdate, pComponent))
      continue;

    pComponent->m_iBufferOffset = INT_MIN;
    pComponent->m_pOutput = nullptr;

    {
      ezResourceLock<ezProcGenGraphResource> pResource(pComponent->m_hResource, ezResourceAcquireMode::NoFallback);
      auto outputs = pResource->GetVertexColorOutputs();
      for (auto& pOutput : outputs)
      {
        if (pOutput->m_sName == pComponent->m_sOutputName)
        {
          pComponent->m_pOutput = pOutput;
          break;
        }
      }
    }

    if (pComponent->m_pOutput == nullptr)
      continue;

    const char* szMesh = pComponent->GetMeshFile();
    if (ezStringUtils::IsNullOrEmpty(szMesh))
      continue;

    ezCpuMeshResourceHandle hCpuMesh = ezResourceManager::LoadResource<ezCpuMeshResource>(szMesh);
    ezResourceLock<ezCpuMeshResource> pCpuMesh(hCpuMesh, ezResourceAcquireMode::NoFallbackAllowMissing);
    if (pCpuMesh.GetAcquireResult() != ezResourceAcquireResult::Final)
    {
      ezLog::Warning("Failed to retrieve CPU mesh '{}'", szMesh);
      continue;
    }

    const auto& mbDesc = pCpuMesh->GetDescriptor().MeshBufferDesc();

    ezUInt32 uiVertexCount = mbDesc.GetVertexCount();
    for (ezUInt32 i = 0; i < uiVertexCount; ++i)
    {
      m_VertexColorData[m_uiCurrentBufferOffset + i] = i * 100;
    }

    pComponent->m_hVertexColorBuffer = m_hVertexColorBuffer;
    pComponent->m_iBufferOffset = m_uiCurrentBufferOffset;

    m_uiCurrentBufferOffset += uiVertexCount;

    // Invalidate all cached render data so the new buffer handle and offset are propagated to the render data
    ezRenderWorld::DeleteCachedRenderData(pComponent->GetOwner()->GetHandle(), pComponent->GetHandle());
  }

  m_ComponentsToUpdate.Clear();
}

void ezProcVertexColorComponentManager::OnEndExtraction(ezUInt64 uiFrameCounter) {}

void ezProcVertexColorComponentManager::OnBeginRender(ezUInt64 uiFrameCounter)
{
  ezGALContext* pGALContext = ezGALDevice::GetDefaultDevice()->GetPrimaryContext();

  pGALContext->UpdateBuffer(m_hVertexColorBuffer, 0, m_VertexColorData.GetByteArrayPtr(), ezGALUpdateMode::Discard);
}

void ezProcVertexColorComponentManager::AddComponent(ezProcVertexColorComponent* pComponent)
{
  auto& hResource = pComponent->GetResource();
  if (!hResource.IsValid())
  {
    return;
  }

  m_ComponentsToUpdate.PushBack(pComponent->GetHandle());
}

void ezProcVertexColorComponentManager::RemoveComponent(ezProcVertexColorComponent* pComponent)
{
  auto& hResource = pComponent->GetResource();
  if (!hResource.IsValid())
  {
    return;
  }

  m_ComponentsToUpdate.RemoveAndSwap(pComponent->GetHandle());

  if (pComponent->m_iBufferOffset >= 0)
  {
    /// \todo compact buffer somehow?

    pComponent->m_iBufferOffset = INT_MIN;
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
      if (it->m_hResource == hResource && !m_ComponentsToUpdate.Contains(it->GetHandle()))
      {
        m_ComponentsToUpdate.PushBack(it->GetHandle());
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
      EZ_ACCESSOR_PROPERTY("OutputName", GetOutputName, SetOutputName)
  }
  EZ_END_PROPERTIES;
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
  pManager->AddComponent(this);
}

void ezProcVertexColorComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  auto pManager = static_cast<ezProcVertexColorComponentManager*>(GetOwningManager());
  pManager->RemoveComponent(this);
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
  auto pManager = static_cast<ezProcVertexColorComponentManager*>(GetOwningManager());

  if (IsActiveAndInitialized())
  {
    pManager->RemoveComponent(this);
  }

  m_hResource = hResource;

  if (IsActiveAndInitialized())
  {
    pManager->AddComponent(this);
  }
}

void ezProcVertexColorComponent::SetOutputName(const char* szOutputName)
{
  auto pManager = static_cast<ezProcVertexColorComponentManager*>(GetOwningManager());

  if (IsActiveAndInitialized())
  {
    pManager->RemoveComponent(this);
  }

  m_sOutputName.Assign(szOutputName);

  if (IsActiveAndInitialized())
  {
    pManager->AddComponent(this);
  }
}

const char* ezProcVertexColorComponent::GetOutputName() const
{
  return m_sOutputName.GetData();
}

void ezProcVertexColorComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  ezStreamWriter& s = stream.GetStream();

  s << m_hResource;
  s << m_sOutputName;
}

void ezProcVertexColorComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_hResource;
  s >> m_sOutputName;
}

ezMeshRenderData* ezProcVertexColorComponent::CreateRenderData() const
{
  auto pRenderData = ezCreateRenderDataForThisFrame<ezProcVertexColorRenderData>(GetOwner());

  if (m_iBufferOffset >= 0)
  {
    pRenderData->m_hVertexColorBuffer = m_hVertexColorBuffer;
    pRenderData->m_iBufferOffset = m_iBufferOffset;
  }

  return pRenderData;
}
