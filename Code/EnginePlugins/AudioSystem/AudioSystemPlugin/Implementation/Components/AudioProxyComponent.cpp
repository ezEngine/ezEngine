#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/Components/AudioProxyComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezAudioProxyComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezTitleAttribute("Audio Proxy"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

ezAudioProxyComponent::ezAudioProxyComponent() = default;
ezAudioProxyComponent::~ezAudioProxyComponent() = default;

EZ_STATICLINK_FILE(AudioSystemPlugin, AudioSystemPlugin_Implementation_Components_AudioProxyComponent);
