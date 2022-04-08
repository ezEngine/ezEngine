#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <JoltPlugin/Actors/JoltStaticActorComponent.h>
#include <JoltPlugin/Components/JoltVisColMeshComponent.h>
#include <JoltPlugin/Shapes/JoltShapeConvexHullComponent.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Pipeline/RenderData.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJoltVisColMeshComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("CollisionMesh", GetMeshFile, SetMeshFile)->AddAttributes(new ezAssetBrowserAttribute("Collision Mesh (Jolt);Collision Mesh (Convex) (Jolt)")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics/Jolt/Misc"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezJoltVisColMeshComponent::ezJoltVisColMeshComponent() = default;
ezJoltVisColMeshComponent::~ezJoltVisColMeshComponent() = default;

void ezJoltVisColMeshComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_hCollisionMesh;
}


void ezJoltVisColMeshComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_hCollisionMesh;

  GetWorld()->GetOrCreateComponentManager<ezJoltVisColMeshComponentManager>()->EnqueueUpdate(GetHandle());
}

ezResult ezJoltVisColMeshComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  // have to assume this isn't thread safe
  // CreateCollisionRenderMesh();

  if (m_hMesh.IsValid())
  {
    ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::BlockTillLoaded);
    bounds = pMesh->GetBounds();
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

void ezJoltVisColMeshComponent::SetMeshFile(const char* szFile)
{
  ezJoltMeshResourceHandle hMesh;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hMesh = ezResourceManager::LoadResource<ezJoltMeshResource>(szFile);
  }

  SetMesh(hMesh);
}

const char* ezJoltVisColMeshComponent::GetMeshFile() const
{
  if (!m_hCollisionMesh.IsValid())
    return "";

  return m_hCollisionMesh.GetResourceID();
}

void ezJoltVisColMeshComponent::SetMesh(const ezJoltMeshResourceHandle& hMesh)
{
  if (m_hCollisionMesh != hMesh)
  {
    m_hCollisionMesh = hMesh;
    m_hMesh.Invalidate();

    GetWorld()->GetOrCreateComponentManager<ezJoltVisColMeshComponentManager>()->EnqueueUpdate(GetHandle());
  }
}

void ezJoltVisColMeshComponent::CreateCollisionRenderMesh()
{
  if (!m_hCollisionMesh.IsValid())
  {
    ezJoltStaticActorComponent* pSibling = nullptr;
    if (GetOwner()->TryGetComponentOfBaseType(pSibling))
    {
      m_hCollisionMesh = pSibling->GetMesh();
    }
  }

  if (!m_hCollisionMesh.IsValid())
  {
    ezJoltShapeConvexHullComponent* pSibling = nullptr;
    if (GetOwner()->TryGetComponentOfBaseType(pSibling))
    {
      m_hCollisionMesh = pSibling->GetMesh();
    }
  }

  if (!m_hCollisionMesh.IsValid())
    return;

  ezResourceLock<ezJoltMeshResource> pMesh(m_hCollisionMesh, ezResourceAcquireMode::BlockTillLoaded);

  if (pMesh.GetAcquireResult() == ezResourceAcquireResult::MissingFallback)
    return;

  ezStringBuilder sColMeshName = pMesh->GetResourceID();
  sColMeshName.AppendFormat("_{0}_JoltVisColMesh",
    pMesh->GetCurrentResourceChangeCounter()); // the change counter allows to react to resource updates

  m_hMesh = ezResourceManager::GetExistingResource<ezMeshResource>(sColMeshName);

  if (m_hMesh.IsValid())
  {
    TriggerLocalBoundsUpdate();
    return;
  }

  ezCpuMeshResourceHandle hCpuMesh = pMesh->ConvertToCpuMesh();

  if (!hCpuMesh.IsValid())
    return;

  ezResourceLock<ezCpuMeshResource> pCpuMesh(hCpuMesh, ezResourceAcquireMode::BlockTillLoaded);

  ezMeshResourceDescriptor md = pCpuMesh->GetDescriptor();

  md.SetMaterial(0, "Materials/Common/ColMesh.ezMaterial");

  m_hMesh = ezResourceManager::GetOrCreateResource<ezMeshResource>(sColMeshName, std::move(md), "Collision Mesh Visualization");

  TriggerLocalBoundsUpdate();
}

void ezJoltVisColMeshComponent::Initialize()
{
  SUPER::Initialize();

  GetWorld()->GetOrCreateComponentManager<ezJoltVisColMeshComponentManager>()->EnqueueUpdate(GetHandle());
}

void ezJoltVisColMeshComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::AllowLoadingFallback);

  ezRenderData::Caching::Enum caching = ezRenderData::Caching::IfStatic;

  if (pMesh.GetAcquireResult() != ezResourceAcquireResult::Final)
  {
    caching = ezRenderData::Caching::Never;
  }

  ezArrayPtr<const ezMeshResourceDescriptor::SubMesh> parts = pMesh->GetSubMeshes();

  for (ezUInt32 uiPartIndex = 0; uiPartIndex < parts.GetCount(); ++uiPartIndex)
  {
    const ezUInt32 uiMaterialIndex = parts[uiPartIndex].m_uiMaterialIndex;
    ezMaterialResourceHandle hMaterial = pMesh->GetMaterials()[uiMaterialIndex];

    ezMeshRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezMeshRenderData>(GetOwner());
    {
      pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
      pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
      pRenderData->m_hMesh = m_hMesh;
      pRenderData->m_hMaterial = hMaterial;
      pRenderData->m_uiSubMeshIndex = uiPartIndex;
      pRenderData->m_uiUniqueID = GetUniqueIdForRendering(uiMaterialIndex);

      pRenderData->FillBatchIdAndSortingKey();
    }

    msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::LitOpaque, caching);
  }
}

//////////////////////////////////////////////////////////////////////////

void ezJoltVisColMeshComponentManager::Initialize()
{
  SUPER::Initialize();

  ezWorldModule::UpdateFunctionDesc desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezJoltVisColMeshComponentManager::Update, this);
  desc.m_Phase = UpdateFunctionDesc::Phase::PreAsync;

  RegisterUpdateFunction(desc);

  ezResourceManager::GetResourceEvents().AddEventHandler(ezMakeDelegate(&ezJoltVisColMeshComponentManager::ResourceEventHandler, this));
}

void ezJoltVisColMeshComponentManager::Deinitialize()
{
  EZ_LOCK(m_Mutex);

  ezResourceManager::GetResourceEvents().RemoveEventHandler(ezMakeDelegate(&ezJoltVisColMeshComponentManager::ResourceEventHandler, this));

  SUPER::Deinitialize();
}

void ezJoltVisColMeshComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  for (const auto& hComp : m_RequireUpdate)
  {
    ezJoltVisColMeshComponent* pComp = nullptr;
    if (!TryGetComponent(hComp, pComp))
      continue;

    pComp->CreateCollisionRenderMesh();
  }

  m_RequireUpdate.Clear();
}

void ezJoltVisColMeshComponentManager::EnqueueUpdate(ezComponentHandle hComponent)
{
  m_RequireUpdate.PushBack(hComponent);
}

void ezJoltVisColMeshComponentManager::ResourceEventHandler(const ezResourceEvent& e)
{
  if ((e.m_Type == ezResourceEvent::Type::ResourceContentUnloading || e.m_Type == ezResourceEvent::Type::ResourceContentUpdated) && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<ezJoltMeshResource>())
  {
    EZ_LOCK(m_Mutex);

    ezJoltMeshResourceHandle hResource((ezJoltMeshResource*)(e.m_pResource));

    for (auto it = m_Components.GetIterator(); it.IsValid(); ++it)
    {
      const ezJoltVisColMeshComponent* pComponent = static_cast<ezJoltVisColMeshComponent*>(it.Value());

      if (pComponent->GetMesh() == hResource)
      {
        m_RequireUpdate.PushBack(pComponent->GetHandle());
      }
    }
  }
}
