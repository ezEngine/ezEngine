#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/Components/AudioSystemComponent.h>
#include <AudioSystemPlugin/Core/AudioSystem.h>
#include <AudioSystemPlugin/Core/AudioSystemRequests.h>

// clang-format off
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezAudioSystemComponent, 1)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Sound"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

void ezAudioSystemComponent::Initialize()
{
  SUPER::Initialize();

  ezAudioSystemRequestRegisterEntity request;

  request.m_uiEntityId = GetOwner()->GetHandle().GetInternalID().m_Data;

  request.m_Callback = [](const ezAudioSystemRequestRegisterEntity& m)
  {
    if (m.m_eStatus.Failed())
      return;

    ezLog::Info("[AudioSystem] Registered entity '{0}' in the audio system.", m.m_uiEntityId);
  };

  ezVariant v(request);

  ezAudioSystem::GetSingleton()->SendRequestSync(std::move(v));

  ezLog::Info("AudioSystem Component Initialized");
}

ezAudioSystemComponent::ezAudioSystemComponent() = default;
ezAudioSystemComponent::~ezAudioSystemComponent() = default;

EZ_STATICLINK_FILE(AudioSystemPlugin, AudioSystemPlugin_Implementation_Components_AudioSystemComponent);
