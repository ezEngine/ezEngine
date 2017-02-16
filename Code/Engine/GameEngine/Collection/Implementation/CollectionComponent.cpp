#include <PCH.h>
#include <GameEngine/Collection/CollectionComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_COMPONENT_TYPE(ezCollectionComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Collection", GetCollectionFile, SetCollectionFile)->AddAttributes(new ezAssetBrowserAttribute("Collection")),
    EZ_MEMBER_PROPERTY("PreloadAtStart", m_bPreloadAtStart)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("TimeToLoad", m_TimeToLoad)->AddAttributes(new ezDefaultValueAttribute(ezTime::Seconds(10.0)), new ezClampValueAttribute(ezTime::Seconds(0), ezTime::Seconds(120))),
  }
  EZ_END_PROPERTIES
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("General"),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezCollectionComponent::ezCollectionComponent()
{
  m_bPreloadAtStart = true;
}

void ezCollectionComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_hCollection;
}

void ezCollectionComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_hCollection;
}

void ezCollectionComponent::SetCollectionFile(const char* szFile)
{
  ezCollectionResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezCollectionResource>(szFile);
    ezResourceManager::PreloadResource(hResource, ezTime::Seconds(0.0));
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

    pCollection->PreloadResources(m_TimeToLoad);
  }
}
