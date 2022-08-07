#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <AudioSystemPlugin/Components/AudioSystemComponent.h>
#include <AudioSystemPlugin/Core/AudioSystemData.h>
#include <AudioSystemPlugin/Core/AudioSystemMessages.h>
#include <AudioSystemPlugin/Resources/AudioControlCollectionResource.h>

typedef ezComponentManager<class ezAudioRtpcComponent, ezBlockStorageType::FreeList> ezAudioRtpcComponentManager;

/// \brief Component used to set the value of a real-time parameter in the audio middleware.
class EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioRtpcComponent : public ezAudioSystemComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAudioRtpcComponent, ezAudioSystemComponent, ezAudioRtpcComponentManager);

  // ezComponent

public:
  void Initialize() override;
  void SerializeComponent(ezWorldWriter& stream) const override;
  void DeserializeComponent(ezWorldReader& stream) override;

  // ezAudioSystemComponent

private:
  void ezAudioSystemComponentIsAbstract() override {}

  // ezAudioRtpcComponent

public:
  ezAudioRtpcComponent();
  ~ezAudioRtpcComponent() override;

  /// \brief Sets the value of the parameter. This will send a request to the Audio System.
  ///
  /// \param fValue The new value the parameter should have.
  /// \param bSync Whether the request should be sent synchronously or asynchronously.
  void SetValue(float fValue, bool bSync = false);

  /// \brief Gets the current value of the parameter.
  /// \returns The current value of the parameter.
  [[nodiscard]] float GetValue() const;

  /// \brief Event that is triggered when the component receives a
  /// SetRtpcValue message.
  void OnSetValue(ezMsgAudioSystemSetRtpcValue& msg);

private:
  ezString m_sRtpcName;
  float m_fInitialValue;
  float m_fValue;

  ezEventMessageSender<ezMsgAudioSystemRtpcValueChanged> m_ValueChangedEventSender;
};
