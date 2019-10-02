#include <CorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/Collection/CollectionComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezCollectionComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Collection", GetCollectionFile, SetCollectionFile)->AddAttributes(new ezAssetBrowserAttribute("Collection")),
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

ezCollectionComponent::ezCollectionComponent() = default;
ezCollectionComponent::~ezCollectionComponent() = default;

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

  if (IsActiveAndSimulating())
  {
    InitiatePreload();
  }
}

void ezCollectionComponent::OnSimulationStarted()
{
  InitiatePreload();
}

void ezCollectionComponent::InitiatePreload()
{
  if (m_hCollection.IsValid())
  {
    ezResourceLock<ezCollectionResource> pCollection(m_hCollection, ezResourceAcquireMode::BlockTillLoaded_NeverFail);

    if (pCollection.GetAcquireResult() == ezResourceAcquireResult::Final)
    {
      pCollection->PreloadResources();
    }
  }
}

EZ_STATICLINK_FILE(Core, Core_Collection_Implementation_CollectionComponent);
