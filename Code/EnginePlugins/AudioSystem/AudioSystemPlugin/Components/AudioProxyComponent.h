#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <AudioSystemPlugin/Components/AudioSystemComponent.h>

typedef ezComponentManagerSimple<class ezAudioProxyComponent, ezComponentUpdateType::WhenSimulating> ezAudioProxyComponentManager;

class EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioProxyComponent : public ezAudioSystemComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAudioProxyComponent, ezAudioSystemComponent, ezAudioProxyComponentManager);

  // ezComponent

public:
  ezAudioProxyComponent();
  ~ezAudioProxyComponent() override;

  // ezAudioSystemComponent

private:
  void ezAudioSystemComponentIsAbstract() override {}

  // ezAudioProxyComponent

protected:
  void Update() {}
};
