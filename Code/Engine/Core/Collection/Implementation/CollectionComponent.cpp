#include <Core/CorePCH.h>

#include <Core/Collection/CollectionComponent.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezCollectionComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_ACCESSOR_PROPERTY("Collection", GetCollection, SetCollection)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_AssetCollection", ezDependencyFlags::Package)),
    EZ_MEMBER_PROPERTY("RegisterNames", m_bRegisterNames),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Utilities"),
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
  s << m_bRegisterNames;
}

void ezCollectionComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_hCollection;

  if (uiVersion >= 2)
  {
    s >> m_bRegisterNames;
  }
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

      if (m_bRegisterNames)
      {
        pCollection->RegisterNames();
      }
    }
  }
}

EZ_STATICLINK_FILE(Core, Core_Collection_Implementation_CollectionComponent);
