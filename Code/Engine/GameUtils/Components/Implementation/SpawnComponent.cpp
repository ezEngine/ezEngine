#include <GameUtils/PCH.h>
#include <GameUtils/Components/SpawnComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_COMPONENT_TYPE(ezSpawnComponent, 1);
  EZ_BEGIN_PROPERTIES
    EZ_ACCESSOR_PROPERTY("Prefab", GetPrefabFile, SetPrefabFile)->AddAttributes(new ezAssetBrowserAttribute("Prefab")),
    EZ_ACCESSOR_PROPERTY("Spawn at Start", GetSpawnAtStart, SetSpawnAtStart),
    EZ_ACCESSOR_PROPERTY("Spawn Continuously", GetSpawnContinuously, SetSpawnContinuously),
  EZ_END_PROPERTIES
  EZ_BEGIN_ATTRIBUTES
    new ezCategoryAttribute("Gameplay"),
  EZ_END_ATTRIBUTES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezSpawnComponent::ezSpawnComponent()
{

}

void ezSpawnComponent::Update()
{
  if (m_Flags.IsAnySet(ezSpawnComponentFlags::SpawnAtStart))
  {
    m_Flags.Remove(ezSpawnComponentFlags::SpawnAtStart);

    if (m_hPrefab.IsValid())
    {
      ezResourceLock<ezPrefabResource> pResource(m_hPrefab);

      /// \todo Attach as child

      pResource->InstantiatePrefab(*GetWorld(), GetOwner()->GetGlobalTransform());
    }
  }

  /// \todo spawn continuously
  /// \todo spawn delay
  /// \todo spawn deviation ?

}

void ezSpawnComponent::SerializeComponent(ezWorldWriter& stream) const
{
  auto& s = stream.GetStream();

  s << m_Flags.GetValue();
  s << GetPrefabFile(); /// \todo Store resource handles more efficiently
}

void ezSpawnComponent::DeserializeComponent(ezWorldReader& stream)
{
  auto& s = stream.GetStream();

  ezSpawnComponentFlags::StorageType flags;
  s >> flags;
  m_Flags.SetValue(flags);

  ezStringBuilder sTemp;
  s >> sTemp;
  SetPrefabFile(sTemp);
}

void ezSpawnComponent::SetPrefabFile(const char* szFile)
{
  ezPrefabResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezPrefabResource>(szFile);
  }

  SetPrefab(hResource);
}

const char* ezSpawnComponent::GetPrefabFile() const
{
  if (!m_hPrefab.IsValid())
    return "";

  ezResourceLock<ezPrefabResource> pResource(m_hPrefab);
  return pResource->GetResourceID();
}

void ezSpawnComponent::SetPrefab(const ezPrefabResourceHandle& hPrefab)
{
  m_hPrefab = hPrefab;
}
