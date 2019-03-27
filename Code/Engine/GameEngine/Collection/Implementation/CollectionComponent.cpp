#include <GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Collection/CollectionComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezCollectionComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Collection", GetCollectionFile, SetCollectionFile)->AddAttributes(new ezAssetBrowserAttribute("Collection")),
    EZ_MEMBER_PROPERTY("PreloadAtStart", m_bPreloadAtStart)->AddAttributes(new ezDefaultValueAttribute(true)),
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

ezCollectionComponent::ezCollectionComponent()
{
  m_bPreloadAtStart = true;
}

void ezCollectionComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  // Version 1
  s << m_hCollection;

  // Version 2
  s << m_bPreloadAtStart;
}

void ezCollectionComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_hCollection;

  if (uiVersion >= 2)
  {
    s >> m_bPreloadAtStart;
  }
}

void ezCollectionComponent::SetCollectionFile(const char* szFile)
{
  ezCollectionResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezCollectionResource>(szFile);
    ezResourceManager::PreloadResource(hResource);
  }

  SetCollection(hResource);
}

const char* ezCollectionComponent::GetCollectionFile() const
{
  if (!m_hCollection.IsValid())
    return "";

  return m_hCollection.GetResourceID();
}

void ezCollectionComponent::SetCollection(const ezCollectionResourceHandle& hCollection)
{
  m_hCollection = hCollection;
}

void ezCollectionComponent::OnSimulationStarted()
{
  if (m_bPreloadAtStart)
  {
    InitiatePreload();
  }
}

void ezCollectionComponent::InitiatePreload()
{
  if (m_hCollection.IsValid())
  {
    ezResourceLock<ezCollectionResource> pCollection(m_hCollection, ezResourceAcquireMode::NoFallback);

    pCollection->PreloadResources();
  }
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_Collection_Implementation_CollectionComponent);

