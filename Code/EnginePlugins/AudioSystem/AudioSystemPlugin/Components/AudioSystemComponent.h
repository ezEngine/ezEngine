#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <AudioSystemPlugin/ATL/AudioTranslationLayer.h>

#include <Core/World/Component.h>
#include <Core/World/World.h>

/// \brief Base class for audio system components.
/// This is used to initialize the ATL world module i
class EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioSystemComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezAudioSystemComponent, ezComponent);

  // ezComponent

public:
  void Initialize() override;
  void SerializeComponent(ezWorldWriter& stream) const override {}
  void DeserializeComponent(ezWorldReader& stream) override {}

  // ezAudioSystemComponent

public:
  ezAudioSystemComponent();
  ~ezAudioSystemComponent() override;

private:
  // Dummy method to hide this component in the editor UI.
  virtual void ezAudioSystemComponentIsAbstract() = 0;
};
