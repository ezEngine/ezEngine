#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <AudioSystemPlugin/Components/AudioSystemComponent.h>
#include <AudioSystemPlugin/Core/AudioSystemData.h>
#include <AudioSystemPlugin/Resources/AudioControlCollectionResource.h>

typedef ezComponentManager<class ezAudioControlsComponent, ezBlockStorageType::FreeList> ezAudioControlsComponentManager;

/// \brief Component used to load and unload a set of audio controls.
///
/// The audio controls are provided by the selected audio control collection.
class EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioControlsComponent : public ezAudioSystemComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAudioControlsComponent, ezAudioSystemComponent, ezAudioControlsComponentManager);

  // ezComponent

public:
  void Initialize() override;
  void Deinitialize() override;
  void SerializeComponent(ezWorldWriter& stream) const override;
  void DeserializeComponent(ezWorldReader& stream) override;

  // ezAudioSystemComponent

private:
  void ezAudioSystemComponentIsAbstract() override {}

  // ezAudioControlsComponent

public:
  ezAudioControlsComponent();
  ~ezAudioControlsComponent() override;

  /// \brief Load the audio controls from the given collection.
  /// This is automatically called on component initialization when
  /// the AutoLoad property is set to true.
  bool Load();

  /// \brief Unloads the audio controls.
  bool Unload();

private:
  ezString m_sControlsAsset;
  bool m_bAutoLoad;

  bool m_bLoaded;
  ezAudioControlCollectionResourceHandle m_hControlsResource;
};
