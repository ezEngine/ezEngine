#include <GameEnginePCH.h>

#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Components/PlayerStartPointComponent.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <GameEngine/Prefabs/PrefabResource.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPlayerStartPointComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("PlayerPrefab", GetPlayerPrefabFile, SetPlayerPrefabFile)->AddAttributes(new ezAssetBrowserAttribute("Prefab")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Gameplay"),
    new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 0.5f, ezColor::DarkSlateBlue),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezPlayerStartPointComponent::ezPlayerStartPointComponent() = default;
 ezPlayerStartPointComponent::~ezPlayerStartPointComponent() = default;

void ezPlayerStartPointComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_hPlayerPrefab;

}

void ezPlayerStartPointComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_hPlayerPrefab;
}

void ezPlayerStartPointComponent::SetPlayerPrefabFile(const char* szFile)
{
  ezPrefabResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezPrefabResource>(szFile);
  }

  SetPlayerPrefab(hResource);
}

const char* ezPlayerStartPointComponent::GetPlayerPrefabFile() const
{
  if (!m_hPlayerPrefab.IsValid())
    return "";

  return m_hPlayerPrefab.GetResourceID();
}

void ezPlayerStartPointComponent::SetPlayerPrefab(const ezPrefabResourceHandle& hPrefab)
{
  m_hPlayerPrefab = hPrefab;
}

const ezPrefabResourceHandle& ezPlayerStartPointComponent::GetPlayerPrefab() const
{
  return m_hPlayerPrefab;
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_Components_Implementation_PlayerStartPointComponent);

