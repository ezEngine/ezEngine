#include <GameUtils/PCH.h>
#include <GameUtils/Components/PrefabReferenceComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_COMPONENT_TYPE(ezPrefabReferenceComponent, 1);
  EZ_BEGIN_PROPERTIES
    EZ_ACCESSOR_PROPERTY("Prefab", GetPrefabFile, SetPrefabFile)->AddAttributes(new ezAssetBrowserAttribute("Prefab")),
  EZ_END_PROPERTIES
  EZ_BEGIN_ATTRIBUTES
    new ezCategoryAttribute("General"),
  EZ_END_ATTRIBUTES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezPrefabReferenceComponent::ezPrefabReferenceComponent()
{
  m_bRequiresInstantiation = true;

  
}

void ezPrefabReferenceComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_hPrefab;

}

void ezPrefabReferenceComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_hPrefab;
}

void ezPrefabReferenceComponent::SetPrefabFile(const char* szFile)
{
  ezPrefabResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezPrefabResource>(szFile);
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

  GetManager()->AddToUpdateList(this);
}


void ezPrefabReferenceComponent::InstantiatePrefab()
{
  if (!m_bRequiresInstantiation)
    return;

  m_bRequiresInstantiation = false;

  // first clear all previous children of this entity
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

    pResource->InstantiatePrefab(*GetWorld(), id, GetOwner()->GetHandle());
  }
}


ezPrefabReferenceComponentManager::ezPrefabReferenceComponentManager(ezWorld* pWorld)
  : ezComponentManager<ComponentType>(pWorld)
{

}

ezResult ezPrefabReferenceComponentManager::Initialize()
{
  auto desc = EZ_CREATE_COMPONENT_UPDATE_FUNCTION_DESC(ezPrefabReferenceComponentManager::SimpleUpdate, this);

  this->RegisterUpdateFunction(desc);

  return EZ_SUCCESS;
}

static void SetEditorPickingID(ezGameObject* pObject, ezUInt32 uiPickingID)
{
  for (auto pComponent : pObject->GetComponents())
  {
    pComponent->m_uiEditorPickingID = uiPickingID;
  }

  
  for (auto itChild = pObject->GetChildren(); itChild.IsValid(); itChild.Next())
  {
    SetEditorPickingID(itChild, uiPickingID);
  }
}

void ezPrefabReferenceComponentManager::SimpleUpdate(ezUInt32 uiStartIndex, ezUInt32 uiCount)
{
  for (auto hComp : m_PrefabComponentsToUpdate)
  {
    ezPrefabReferenceComponent* pPrefab;
    if (!TryGetComponent(hComp, pPrefab))
      continue;

    if (!pPrefab->IsActive())
      continue;

    pPrefab->InstantiatePrefab();

    // if this ID is valid, this prefab is instantiated at editor runtime
    // replicate the same ID across all instantiated sub components to get correct picking behavior
    if (pPrefab->m_uiEditorPickingID != 0xFFFFFFFF)
    {
      SetEditorPickingID(pPrefab->GetOwner(), pPrefab->m_uiEditorPickingID);
    }
  }

  m_PrefabComponentsToUpdate.Clear();
}

void ezPrefabReferenceComponentManager::AddToUpdateList(ezPrefabReferenceComponent* pComponent)
{
  m_PrefabComponentsToUpdate.PushBack(pComponent->GetHandle());
}
