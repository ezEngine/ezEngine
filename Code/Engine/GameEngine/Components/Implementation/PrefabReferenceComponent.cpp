#include <PCH.h>
#include <GameEngine/Components/PrefabReferenceComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>

EZ_BEGIN_COMPONENT_TYPE(ezPrefabReferenceComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Prefab", GetPrefabFile, SetPrefabFile)->AddAttributes(new ezAssetBrowserAttribute("Prefab")),
    EZ_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new ezExposedParametersAttribute("Prefab")),
  }
  EZ_END_PROPERTIES
    EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("General"),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezPrefabReferenceComponent::ezPrefabReferenceComponent()
{
  m_bRequiresInstantiation = true;
}

ezPrefabReferenceComponent::~ezPrefabReferenceComponent()
{

}

void ezPrefabReferenceComponent::Deinitialize()
{
  for (auto it = GetOwner()->GetChildren(); it.IsValid(); ++it)
  {
    GetWorld()->DeleteObjectNow(it->GetHandle());
  }
}

void ezPrefabReferenceComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_hPrefab;

  const bool instantiate = GetUniqueID() != ezInvalidIndex;
  s << instantiate;
}

void ezPrefabReferenceComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_hPrefab;
  s >> m_bRequiresInstantiation;

  if (m_bRequiresInstantiation)
  {
    GetWorld()->GetComponentManager<ezPrefabReferenceComponentManager>()->AddToUpdateList(this);
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
  // both handles invalid -> no change
  if (!m_hPrefab.IsValid() && !hPrefab.IsValid())
    return;

  m_hPrefab = hPrefab;
  m_bRequiresInstantiation = true;

  GetWorld()->GetComponentManager<ezPrefabReferenceComponentManager>()->AddToUpdateList(this);
}


void ezPrefabReferenceComponent::InstantiatePrefab()
{
  if (!m_bRequiresInstantiation)
    return;

  m_bRequiresInstantiation = false;

  // first clear all previous children of this object
  {
    auto itChild = GetOwner()->GetChildren();
    ezWorld* pWorld = GetWorld();

    while (itChild.IsValid())
    {
      pWorld->DeleteObjectNow(itChild->GetHandle());

      itChild.Next();
    }
  }

  // now instantiate the prefab
  if (m_hPrefab.IsValid())
  {
    ezResourceLock<ezPrefabResource> pResource(m_hPrefab);

    ezTransform id;
    id.SetIdentity();

    pResource->InstantiatePrefab(*GetWorld(), id, GetOwner()->GetHandle(), nullptr, &GetOwner()->GetTeamID(), &m_Parameters);
  }
}

void ezPrefabReferenceComponent::Initialize()
{
  SUPER::Initialize();

  // instantiate the prefab right away, such that game play code can access it as soon as possible
  // additionally the manager may update the instance later on, to properly enable editor work flows
  InstantiatePrefab();
}


const ezRangeView<const char*, ezUInt32> ezPrefabReferenceComponent::GetParameters() const
{
  return ezRangeView<const char*, ezUInt32>(
    [this]()-> ezUInt32 { return 0; },
    [this]()-> ezUInt32 { return m_Parameters.GetCount(); },
    [this](ezUInt32& it) { ++it; },
    [this](const ezUInt32& it)-> const char*
  {
    return m_Parameters.GetKey(it).GetString().GetData();
  });
}

void ezPrefabReferenceComponent::SetParameter(const char* szKey, const ezVariant& value)
{
  ezHashedString hs;
  hs.Assign(szKey);

  auto it = m_Parameters.Find(hs);
  if (it != ezInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;

  m_bRequiresInstantiation = true;
  GetWorld()->GetComponentManager<ezPrefabReferenceComponentManager>()->AddToUpdateList(this);
}

void ezPrefabReferenceComponent::RemoveParameter(const char* szKey)
{
  if (m_Parameters.Remove(ezTempHashedString(szKey)))
  {
    m_bRequiresInstantiation = true;
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
  if (e.m_EventType == ezResourceEventType::ResourceContentUnloading && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<ezPrefabResource>())
  {
    ezPrefabResourceHandle hPrefab((ezPrefabResource*)(e.m_pResource));

    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      if (it->m_hPrefab == hPrefab)
      {
        it->m_bRequiresInstantiation = true;
        AddToUpdateList(it);
      }
    }
  }
}

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

void ezPrefabReferenceComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  for (auto hComp : m_PrefabComponentsToUpdate)
  {
    ezPrefabReferenceComponent* pPrefab;
    if (!TryGetComponent(hComp, pPrefab))
      continue;

    // not implemented in all necessary code paths atm
    //if (!pPrefab->IsActive())
    //  continue;

    pPrefab->InstantiatePrefab();

    // if this ID is valid, this prefab is instantiated at editor runtime
    // replicate the same ID across all instantiated sub components to get correct picking behavior
    if (pPrefab->GetUniqueID() != ezInvalidIndex)
    {
      // while exporting a scene all game objects with this tag are ignored and not exported
      // set this tag on all game objects that were created by instantiating this prefab
      // instead it should be instantiated at runtime again
      // only do this at editor time though, at regular runtime we do want to fully serialize the entire sub tree
      const ezTag& tag = ezTagRegistry::GetGlobalRegistry().RegisterTag("EditorPrefabInstance");

      ezGameObject* pOwner = pPrefab->GetOwner();
      for (auto itChild = pOwner->GetChildren(); itChild.IsValid(); itChild.Next())
      {
        SetUniqueIDRecursive(itChild, pPrefab->GetUniqueID(), tag);
      }
    }
  }

  m_PrefabComponentsToUpdate.Clear();
}

void ezPrefabReferenceComponentManager::AddToUpdateList(ezPrefabReferenceComponent* pComponent)
{
  m_PrefabComponentsToUpdate.PushBack(pComponent->GetHandle());
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_Components_Implementation_PrefabReferenceComponent);

