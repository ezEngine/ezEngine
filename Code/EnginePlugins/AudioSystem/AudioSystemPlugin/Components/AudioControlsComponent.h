#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <AudioSystemPlugin/Components/AudioSystemComponent.h>
#include <AudioSystemPlugin/Core/AudioSystemData.h>
#include <AudioSystemPlugin/Resources/AudioControlCollectionResource.h>

typedef ezComponentManager<class ezAudioControlsComponent, ezBlockStorageType::FreeList> ezAudioControlsComponentManager;

/// \brief Component used to load and unload a set of audio controls.
class EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioControlsComponent : public ezAudioSystemComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAudioControlsComponent, ezAudioSystemComponent, ezAudioControlsComponentManager);

  // ezComponent

public:
  void Initialize() override;
  void SerializeComponent(ezWorldWriter& stream) const override;
  void DeserializeComponent(ezWorldReader& stream) override;

  // ezAudioSystemComponent

private:
  void ezAudioSystemComponentIsAbstract() override {}

  // ezAudioControlsComponent

public:
  ezAudioControlsComponent();
  ~ezAudioControlsComponent() override;

private:
  ezString m_sControlsAsset;

  ezAudioControlCollectionResourceHandle m_hControlsResource;
};
