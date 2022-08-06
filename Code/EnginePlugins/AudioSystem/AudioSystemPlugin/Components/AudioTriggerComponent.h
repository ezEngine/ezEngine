#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <AudioSystemPlugin/Components/AudioSystemComponent.h>
#include <AudioSystemPlugin/Core/AudioSystemData.h>

typedef ezComponentManagerSimple<class ezAudioTriggerComponent, ezComponentUpdateType::WhenSimulating> ezAudioTriggerComponentManager;

/// \brief Audio System Component that triggers an audio event.
///
/// This component takes as properties a mandatory play trigger an an optional stop trigger.
/// The user should specify the name of the triggers as defined in the loaded audio controls collection asset.
/// If the stop trigger is left empty, the component will send a StopEvent request to the audio system, that means
/// the event triggered by the play trigger should be stoppable that way.
///
/// The component also exposes a property that allows to access the internal state through scripting.
class EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioTriggerComponent : public ezAudioSystemComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAudioTriggerComponent, ezAudioSystemComponent, ezAudioTriggerComponentManager);

  // ezComponent

public:
  ezAudioTriggerComponent();
  ~ezAudioTriggerComponent() override;

  void SerializeComponent(ezWorldWriter& stream) const override;
  void DeserializeComponent(ezWorldReader& stream) override;

  // ezAudioSystemComponent

private:
  void ezAudioSystemComponentIsAbstract() override {}

  // ezAudioTriggerComponent

public:
  /// \brief Sets the name of the play trigger. If the provided name is the same than the
  /// current name, nothing will happen.
  ///
  /// When setting a new name, the current event will be stopped if playing, but the new event will
  /// not be triggered automatically.
  ///
  /// \param sName The name of the play trigger.
  void SetPlayTrigger(ezString sName);

  /// \brief Gets the name of the current play trigger.
  [[nodiscard]] const ezString& GetPlayTrigger() const;

  /// \brief Sets the name of the stop trigger. If the provided name is the same than the
  /// current name, nothing will happen.
  ///
  /// \param sName The name of the stop trigger.
  void SetStopTrigger(ezString sName);

  /// \brief Gets the name of the current stop trigger.
  [[nodiscard]] const ezString& GetStopTrigger() const;

  /// \brief Gets the internal state of the play trigger.
  /// \returns An ezEnum with the value of the play trigger state.
  [[nodiscard]] const ezEnum<ezAudioSystemTriggerState>& GetState() const;

  /// \brief Returns whether the play trigger is currently being loaded.
  /// \returns True if the play trigger is currently being loaded, false otherwise.
  [[nodiscard]] bool IsLoading() const;

  /// \brief Returns whether the play trigger is ready to be activated.
  /// \returns True if the play trigger is ready to be activated, false otherwise.
  [[nodiscard]] bool IsReady() const;

  /// \brief Returns whether the play trigger is being activated.
  /// \returns True if the play trigger is being activated, false otherwise.
  [[nodiscard]] bool IsStarting() const;

  /// \brief Returns whether the play trigger has been activated and is currently playing.
  /// \returns True if the play trigger is currently playing, false otherwise.
  [[nodiscard]] bool IsPlaying() const;

  /// \brief Returns whether the event is being stopped, either by activating the stop trigger
  /// if defined, or by stopping the event directly.
  /// \returns True if the event is being stopped, false otherwise.
  [[nodiscard]] bool IsStopping() const;

  /// \brief Returns whether the play trigger has been activated and is currently stopped.
  /// \returns True if the play trigger is currently stopped, false otherwise.
  [[nodiscard]] bool IsStopped() const;

  /// \brief Returns whether the play trigger is being unloaded.
  /// \returns True if the play trigger is currently being unloaded, false otherwise.
  [[nodiscard]] bool IsUnloading() const;

  /// \brief Activates the play trigger. If the play trigger was not loaded on initialization, this will
  /// load the play trigger the first time it's called.
  ///
  /// \param bSync Whether the request should be executed synchronously or asynchronously.
  void Play(bool bSync = false);

  /// \brief If a stop trigger is defined, this will activate it. Otherwise, the triggered event will be stopped.
  ///
  /// \param bSync Whether the request should be executed synchronously or asynchronously.
  void Stop(bool bSync = false);

protected:
  void Initialize() override;
  void OnActivated() override;
  void OnSimulationStarted() override;
  void OnDeactivated() override;
  void Deinitialize() override;

  void Update();

private:
  void LoadPlayTrigger(bool bSync);
  void LoadStopTrigger(bool bSync);
  void UnloadPlayTrigger();
  void UnloadStopTrigger();

  ezEnum<ezAudioSystemTriggerState> m_eState;

  ezAudioSystemDataID m_uiPlayEventId = 0;
  ezAudioSystemDataID m_uiStopEventId = 0;

  ezString m_sPlayTrigger;
  ezString m_sStopTrigger;

  ezEnum<ezAudioSystemSoundObstructionType> m_eObstructionType;

  bool m_bLoadOnInit;
  bool m_bPlayOnActivate;

  bool m_bPlayTriggerLoaded = false;
  bool m_bStopTriggerLoaded = false;

  bool m_bHasPlayedOnActivate = false;
};
