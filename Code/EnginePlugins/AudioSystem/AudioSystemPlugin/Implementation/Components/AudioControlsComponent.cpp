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
    EZ_MEMBER_PROPERTY("AutoLoad", m_bAutoLoad),

    EZ_MEMBER_PROPERTY_READ_ONLY("Loaded", m_bLoaded)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(Load),
    EZ_SCRIPT_FUNCTION_PROPERTY(Unload),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

void ezAudioControlsComponent::Initialize()
{
  SUPER::Initialize();

  if (m_bAutoLoad)
    Load();
}

void ezAudioControlsComponent::Deinitialize()
{
  if (m_bLoaded)
    Unload();

  SUPER::Deinitialize();
}

void ezAudioControlsComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s.WriteVersion(kVersion_AudioControlsComponent);

  s << m_sControlsAsset;
  s << m_bAutoLoad;
}

void ezAudioControlsComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);

  auto& s = stream.GetStream();

  const ezTypeVersion uiVersion = s.ReadVersion(kVersion_AudioControlsComponent);
  EZ_ASSERT_DEV(uiVersion <= kVersion_AudioControlsComponent, "Invalid component version.");

  s >> m_sControlsAsset;
  s >> m_bAutoLoad;
}

ezAudioControlsComponent::ezAudioControlsComponent()
  : ezAudioSystemComponent()
  , m_bAutoLoad(false)
  , m_bLoaded(false)
{
}

ezAudioControlsComponent::~ezAudioControlsComponent() = default;

bool ezAudioControlsComponent::Load()
{
  if (m_sControlsAsset.IsEmpty())
    return false;

  if (m_bLoaded)
    return true;

  m_hControlsResource = ezResourceManager::LoadResource<ezAudioControlCollectionResource>(m_sControlsAsset);
  if (const ezResourceLock resource(m_hControlsResource, ezResourceAcquireMode::BlockTillLoaded); resource.GetAcquireResult() == ezResourceAcquireResult::MissingFallback)
  {
    ezLog::Error("Failed to load audio control collection '{0}'", m_sControlsAsset);
    return false;
  }

  m_bLoaded = true;
  return true;
}

bool ezAudioControlsComponent::Unload()
{
  if (!m_bLoaded || !m_hControlsResource.IsValid())
    return false;

  ezResourceManager::FreeAllUnusedResources();
  m_hControlsResource.Invalidate();
  ezResourceManager::FreeAllUnusedResources();

  m_bLoaded = false;
  return true;
}

EZ_STATICLINK_FILE(AudioSystemPlugin, AudioSystemPlugin_Implementation_Components_AudioControlsComponent);
