#include <ProcGenPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <ProcGenPlugin/Components/ProcVertexColorComponent.h>

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

ezProcVertexColorComponentManager::~ezProcVertexColorComponentManager() {}

void ezProcVertexColorComponentManager::Initialize()
{
#if 0
  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezProcVertexColorComponentManager::FindTiles, this);
    desc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PreAsync;
    desc.m_fPriority = 10000.0f;

    this->RegisterUpdateFunction(desc);
  }

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezProcVertexColorComponentManager::PreparePlace, this);
    desc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::Async;

    this->RegisterUpdateFunction(desc);
  }

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezProcVertexColorComponentManager::PlaceObjects, this);
    desc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PostAsync;

    this->RegisterUpdateFunction(desc);
  }
#endif

  ezResourceManager::s_ResourceEvents.AddEventHandler(ezMakeDelegate(&ezProcVertexColorComponentManager::OnResourceEvent, this));
}

void ezProcVertexColorComponentManager::Deinitialize()
{
  ezResourceManager::s_ResourceEvents.RemoveEventHandler(ezMakeDelegate(&ezProcVertexColorComponentManager::OnResourceEvent, this));
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

  //RemoveTilesForComponent(pComponent);
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

void ezProcVertexColorComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  ezStreamWriter& s = stream.GetStream();

  s << m_hResource;
}

void ezProcVertexColorComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_hResource;
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
