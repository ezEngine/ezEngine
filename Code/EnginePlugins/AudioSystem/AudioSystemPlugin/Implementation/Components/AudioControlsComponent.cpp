#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/Components/AudioControlsComponent.h>
#include <AudioSystemPlugin/Core/AudioSystemRequests.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

constexpr ezTypeVersion kVersion_AudioControlsComponent = 1;

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezAudioControlsComponent, kVersion_AudioControlsComponent, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ControlsAsset", m_sControlsAsset)->AddAttributes(new ezAssetBrowserAttribute("Audio Control Collection")),
  }
  EZ_END_PROPERTIES;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

void ezAudioControlsComponent::Initialize()
{
  SUPER::Initialize();

  if (m_sControlsAsset.IsEmpty())
    return;

  m_hControlsResource = ezResourceManager::LoadResource<ezAudioControlCollectionResource>(m_sControlsAsset);
  ezResourceLock resource(m_hControlsResource, ezResourceAcquireMode::BlockTillLoaded);
  if (resource->GetLoadingState() != ezResourceState::Loaded)
  {
    ezLog::Error("Failed to load audio control collection '{0}'", m_sControlsAsset);
  }
}

void ezAudioControlsComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();
  s.WriteVersion(kVersion_AudioControlsComponent);
  s << m_sControlsAsset;
}

void ezAudioControlsComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);

  auto& s = stream.GetStream();
  s.ReadVersion(kVersion_AudioControlsComponent);
  s >> m_sControlsAsset;
}

ezAudioControlsComponent::ezAudioControlsComponent() = default;
ezAudioControlsComponent::~ezAudioControlsComponent() = default;

EZ_STATICLINK_FILE(AudioSystemPlugin, AudioSystemPlugin_Implementation_Components_AudioSystemComponent);
