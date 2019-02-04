#include <PCH.h>

#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Components/PrefabReferenceComponent.h>

namespace
{
  static void SetUniqueIDRecursive(ezGameObject* pObject, ezUInt32 uiUniqueID, const ezTag& tag)
  {
    pObject->GetTags().Set(tag);

    for (auto pComponent : pObject->GetComponents())
    {
      pComponent->SetUniqueID(uiUniqueID);
    }


    for (auto itChild = pObject->GetChildren(); itChild.IsValid(); itChild.Next())
    {
      SetUniqueIDRecursive(itChild, uiUniqueID, tag);
    }
  }
} // namespace

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPrefabReferenceComponent, 3, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Prefab", GetPrefabFile, SetPrefabFile)->AddAttributes(new ezAssetBrowserAttribute("Prefab")),
    EZ_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new ezExposedParametersAttribute("Prefab")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("General"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

bool ezPrefabReferenceComponent::s_bDeleteComponentsAfterInstantiation = true;

ezPrefabReferenceComponent::ezPrefabReferenceComponent() = default;
ezPrefabReferenceComponent::~ezPrefabReferenceComponent() = default;

void ezPrefabReferenceComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_hPrefab;

  // Version 2
  const ezUInt32 numParams = m_Parameters.GetCount();
  s << numParams;
  for (ezUInt32 i = 0; i < numParams; ++i)
  {
    s << m_Parameters.GetKey(i);
    s << m_Parameters.GetValue(i);
  }
}

void ezPrefabReferenceComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_hPrefab;

  if (uiVersion < 3)
  {
    bool bDummy;
    s >> bDummy;
  }

  if (uiVersion >= 2)
  {
    ezUInt32 numParams = 0;
    s >> numParams;

    m_Parameters.Reserve(numParams);

    ezHashedString key;
    ezVariant value;

    for (ezUInt32 i = 0; i < numParams; ++i)
    {
      s >> key;
      s >> value;

      m_Parameters.Insert(key, value);
    }
  }
}

void ezPrefabReferenceComponent::SetPrefabFile(const char* szFile)
{
  ezPrefabResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezPrefabResource>(szFile, ezResourcePriority::High, ezPrefabResourceHandle());
    ezResourceManager::PreloadResource(hResource, ezTime::Seconds(0.0));
  }

  SetPrefab(hResource);
}

const char* ezPrefabReferenceComponent::GetPrefabFile() const
{
  if (!m_hPrefab.IsValid())
    return "";

  return m_hPrefab.GetResourceID();
}

void ezPrefabReferenceComponent::SetPrefab(const ezPrefabResourceHandle& hPrefab)
{
  if (m_hPrefab == hPrefab)
    return;

  m_hPrefab = hPrefab;

  GetWorld()->GetComponentManager<ezPrefabReferenceComponentManager>()->AddToUpdateList(this);
}


void ezPrefabReferenceComponent::InstantiatePrefab()
{
  ClearPreviousInstances();

  // now instantiate the prefab
  if (m_hPrefab.IsValid())
  {
    ezResourceLock<ezPrefabResource> pResource(m_hPrefab, ezResourceAcquireMode::AllowFallback);

    ezTransform id;
    id.SetIdentity();

    // if this ID is valid, this prefab is instantiated at editor runtime
    // replicate the same ID across all instantiated sub components to get correct picking behavior
    if (GetUniqueID() != ezInvalidIndex)
    {
      ezHybridArray<ezGameObject*, 8> createdRootObjects;

      pResource->InstantiatePrefab(*GetWorld(), id, GetOwner()->GetHandle(), &createdRootObjects, &GetOwner()->GetTeamID(), &m_Parameters);

      // while exporting a scene all game objects with this tag are ignored and not exported
      // set this tag on all game objects that were created by instantiating this prefab
      // instead it should be instantiated at runtime again
      // only do this at editor time though, at regular runtime we do want to fully serialize the entire sub tree
      const ezTag& tag = ezTagRegistry::GetGlobalRegistry().RegisterTag("EditorPrefabInstance");

      for (ezGameObject* pChild : createdRootObjects)
      {
        SetUniqueIDRecursive(pChild, GetUniqueID(), tag);
      }
    }
    else
    {
      pResource->InstantiatePrefab(*GetWorld(), id, GetOwner()->GetHandle(), nullptr, &GetOwner()->GetTeamID(), &m_Parameters);
    }
  }
}

void ezPrefabReferenceComponent::OnActivated()
{
  SUPER::OnActivated();

  // instantiate the prefab right away, such that game play code can access it as soon as possible
  // additionally the manager may update the instance later on, to properly enable editor work flows
  InstantiatePrefab();
}

void ezPrefabReferenceComponent::OnDeactivated()
{
  ClearPreviousInstances();

  SUPER::OnDeactivated();
}

void ezPrefabReferenceComponent::ClearPreviousInstances()
{
  if (GetUniqueID() != ezInvalidIndex)
  {
    const ezTag& tag = ezTagRegistry::GetGlobalRegistry().RegisterTag("EditorPrefabInstance");

    for (auto it = GetOwner()->GetChildren(); it.IsValid(); ++it)
    {
      if (it->GetTags().IsSet(tag))
      {
        GetWorld()->DeleteObjectNow(it->GetHandle());
      }
    }
  }
}

void ezPrefabReferenceComponent::Deinitialize()
{
  if (!IsActiveAndSimulating() && IsActive())
  {
    // when the component has been activated and now deleted, without ever starting simulation,
    // remove the children (through Deactivate)
    // this is for the editor use case, where the component is always active, but simulation never starts
    // without this, removing the component would not remove the children

    OnDeactivated();
  }
  else
  {
    // do nothing, ie do not call OnDeactivated()
    // we do want to keep the created child objects around when this component gets destroyed during simulation
    // that's because the component actually deletes itself when simulation starts
  }
}

void ezPrefabReferenceComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (s_bDeleteComponentsAfterInstantiation)
  {
    // remove the prefab reference component, to prevent issues after another serialization/deserialization
    // and also to save some memory
    DeleteComponent();
  }
}

const ezRangeView<const char*, ezUInt32> ezPrefabReferenceComponent::GetParameters() const
{
  return ezRangeView<const char*, ezUInt32>(
      [this]() -> ezUInt32 { return 0; }, [this]() -> ezUInt32 { return m_Parameters.GetCount(); }, [this](ezUInt32& it) { ++it; },
      [this](const ezUInt32& it) -> const char* { return m_Parameters.GetKey(it).GetString().GetData(); });
}

void ezPrefabReferenceComponent::SetParameter(const char* szKey, const ezVariant& value)
{
  ezHashedString hs;
  hs.Assign(szKey);

  auto it = m_Parameters.Find(hs);
  if (it != ezInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;

  GetWorld()->GetComponentManager<ezPrefabReferenceComponentManager>()->AddToUpdateList(this);
}

void ezPrefabReferenceComponent::RemoveParameter(const char* szKey)
{
  if (m_Parameters.RemoveAndCopy(ezTempHashedString(szKey)))
  {
    GetWorld()->GetComponentManager<ezPrefabReferenceComponentManager>()->AddToUpdateList(this);
  }
}

bool ezPrefabReferenceComponent::GetParameter(const char* szKey, ezVariant& out_value) const
{
  ezUInt32 it = m_Parameters.Find(szKey);

  if (it == ezInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}

//////////////////////////////////////////////////////////////////////////

ezPrefabReferenceComponentManager::ezPrefabReferenceComponentManager(ezWorld* pWorld)
    : ezComponentManager<ComponentType, ezBlockStorageType::Compact>(pWorld)
{
  ezResourceManager::s_ResourceEvents.AddEventHandler(ezMakeDelegate(&ezPrefabReferenceComponentManager::ResourceEventHandler, this));
}


ezPrefabReferenceComponentManager::~ezPrefabReferenceComponentManager()
{
  ezResourceManager::s_ResourceEvents.RemoveEventHandler(ezMakeDelegate(&ezPrefabReferenceComponentManager::ResourceEventHandler, this));
}

void ezPrefabReferenceComponentManager::Initialize()
{
  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezPrefabReferenceComponentManager::Update, this);

  this->RegisterUpdateFunction(desc);
}

void ezPrefabReferenceComponentManager::ResourceEventHandler(const ezResourceEvent& e)
{
  if (e.m_Type == ezResourceEvent::Type::ResourceContentUnloading && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<ezPrefabResource>())
  {
    ezPrefabResourceHandle hPrefab((ezPrefabResource*)(e.m_pResource));

    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      if (it->m_hPrefab == hPrefab)
      {
        AddToUpdateList(it);
      }
    }
  }
}

void ezPrefabReferenceComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  for (auto hComp : m_ComponentsToUpdate)
  {
    ezPrefabReferenceComponent* pComponent;
    if (!TryGetComponent(hComp, pComponent))
      continue;

    pComponent->m_bInUpdateList = false;
    if (!pComponent->IsActive())
      continue;

    pComponent->InstantiatePrefab();
  }

  m_ComponentsToUpdate.Clear();
}

void ezPrefabReferenceComponentManager::AddToUpdateList(ezPrefabReferenceComponent* pComponent)
{
  if (!pComponent->m_bInUpdateList)
  {
    m_ComponentsToUpdate.PushBack(pComponent->GetHandle());
    pComponent->m_bInUpdateList = true;
  }
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_Components_Implementation_PrefabReferenceComponent);

