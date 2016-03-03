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
}

ezComponent::Initialization ezPrefabReferenceComponent::Initialize()
{
  if (m_hPrefab.IsValid())
  {
    ezResourceLock<ezPrefabResource> pResource(m_hPrefab);

    ezTransform id;
    id.SetIdentity();

    pResource->InstantiatePrefab(*GetWorld(), id, GetOwner()->GetHandle());
  }

  return ezComponent::Initialization::Done;
}

void ezPrefabReferenceComponent::SerializeComponent(ezWorldWriter& stream) const
{
  auto& s = stream.GetStream();

  s << GetPrefabFile(); /// \todo Store resource handles more efficiently

}

void ezPrefabReferenceComponent::DeserializeComponent(ezWorldReader& stream)
{
  auto& s = stream.GetStream();

  ezStringBuilder sTemp;
  s >> sTemp;
  SetPrefabFile(sTemp);

}

void ezPrefabReferenceComponent::SetPrefabFile(const char* szFile)
{
  ezPrefabResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezPrefabResource>(szFile);
    ezResourceManager::PreloadResource(hResource, ezTime::Seconds(5.0));
  }

  SetPrefab(hResource);
}

const char* ezPrefabReferenceComponent::GetPrefabFile() const
{
  if (!m_hPrefab.IsValid())
    return "";

  ezResourceLock<ezPrefabResource> pResource(m_hPrefab);
  return pResource->GetResourceID();
}

void ezPrefabReferenceComponent::SetPrefab(const ezPrefabResourceHandle& hPrefab)
{
  m_hPrefab = hPrefab;
}
