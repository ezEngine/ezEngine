#include <Core/CorePCH.h>

#include <Core/Collection/CollectionComponent.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezCollectionComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Collection", GetCollectionFile, SetCollectionFile)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_AssetCollection", ezDependencyFlags::Package)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("General"),
    new ezColorAttribute(ezColorScheme::GetGroupColor(ezColorScheme::Utilities)),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezCollectionComponent::ezCollectionComponent() = default;
ezCollectionComponent::~ezCollectionComponent() = default;

void ezCollectionComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_hCollection;
}

void ezCollectionComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_hCollection;
}

void ezCollectionComponent::SetCollectionFile(ezStringView sFile)
{
  ezCollectionResourceHandle hResource;

  if (!sFile.IsEmpty())
  {
    hResource = ezResourceManager::LoadResource<ezCollectionResource>(sFile);
    ezResourceManager::PreloadResource(hResource);
  }

  SetCollection(hResource);
}

ezStringView ezCollectionComponent::GetCollectionFile() const
{
  if (!m_hCollection.IsValid())
    return {};

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
