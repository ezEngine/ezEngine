#include <GameEngine/GameEnginePCH.h>

#include <Core/Prefabs/PrefabReferenceComponent.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/PlayerStartPointComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPlayerStartPointComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("PlayerPrefab", GetPlayerPrefabFile, SetPlayerPrefabFile)->AddAttributes(new ezAssetBrowserAttribute("Prefab")),
    EZ_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new ezExposedParametersAttribute("PlayerPrefab")),
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

  ezPrefabReferenceComponent::SerializePrefabParameters(*GetWorld(), stream, m_Parameters);
}

void ezPlayerStartPointComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_hPlayerPrefab;

  if (uiVersion >= 2)
  {
    ezPrefabReferenceComponent::DeserializePrefabParameters(m_Parameters, stream);
  }
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

const ezRangeView<const char*, ezUInt32> ezPlayerStartPointComponent::GetParameters() const
{
  return ezRangeView<const char*, ezUInt32>([]() -> ezUInt32 { return 0; }, [this]() -> ezUInt32 { return m_Parameters.GetCount(); }, [](ezUInt32& it) { ++it; }, [this](const ezUInt32& it) -> const char* { return m_Parameters.GetKey(it).GetString().GetData(); });
}

void ezPlayerStartPointComponent::SetParameter(const char* szKey, const ezVariant& value)
{
  ezHashedString hs;
  hs.Assign(szKey);

  auto it = m_Parameters.Find(hs);
  if (it != ezInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;
}

void ezPlayerStartPointComponent::RemoveParameter(const char* szKey)
{
  m_Parameters.RemoveAndCopy(ezTempHashedString(szKey));
}

bool ezPlayerStartPointComponent::GetParameter(const char* szKey, ezVariant& out_value) const
{
  ezUInt32 it = m_Parameters.Find(szKey);

  if (it == ezInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_PlayerStartPointComponent);
