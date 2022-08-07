#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <AudioSystemPlugin/ATL/AudioTranslationLayer.h>
#include <AudioSystemPlugin/Core/AudioSystemData.h>
#include <AudioSystemPlugin/Core/AudioThread.h>
#include <AudioSystemPlugin/Resources/AudioControlCollectionResource.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/Interfaces/SoundInterface.h>

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Threading/Semaphore.h>

typedef ezDeque<ezVariant> ezAudioSystemRequestsQueue;

class EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioSystem : public ezSoundInterface
{
  EZ_DECLARE_SINGLETON_OF_INTERFACE(ezAudioSystem, ezSoundInterface);

  // ----- ezSoundInterface

public:
  /// \brief This should be called in the audio middleware implementation startup to
  /// load the AudioSystem with the correct configuration.
  void LoadConfiguration(const char* szFile) override;

  /// \brief By default, the AudioSystem will auto-detect the platform (and thus the config) to use.
  /// Calling this before startup allows to override which configuration is used.
  void SetOverridePlatform(const char* szPlatform) override;

  /// \brief Called once per frame to update all sounds.
  void UpdateSound() override;

  /// \brief Asks the audio middleware to adjust its master volume.
  void SetMasterChannelVolume(float volume) override;

  /// \brief Gets the master volume of the audio middleware.
  float GetMasterChannelVolume() const override;

  /// \brief Asks the audio middleware to mute its master channel.
  void SetMasterChannelMute(bool mute) override;

  /// \brief Gets the muted state of the audio middleware master channel.
  bool GetMasterChannelMute() const override;

  /// \brief Asks the audio middleware to pause every playbacks.
  void SetMasterChannelPaused(bool paused) override;

  /// \brief Gets the paused state of the audio middleware.
  bool GetMasterChannelPaused() const override;

  /// \brief Asks the audio middleware to adjust the volume of a sound group.
  void SetSoundGroupVolume(const char* szVcaGroupGuid, float volume) override;

  /// \brief Gets a sound group volume from the audio middleware.
  float GetSoundGroupVolume(const char* szVcaGroupGuid) const override;

  /// \brief Asks the audio middleware to set the required number of listeners.
  void SetNumListeners(ezUInt8 uiNumListeners) override {}

  /// \brief Gets the number of listeners from the audio middleware.
  ezUInt8 GetNumListeners() override;

  /// \brief Overrides the active audio middleware listener by the editor camera. Transformation
  /// data will be provided by the editor camera.
  void SetListenerOverrideMode(bool enabled) override;

  /// \brief Sets the transformation of the listener with the given ID.
  /// ID -1 is used for the override mode listener (editor camera).
  void SetListener(ezInt32 iIndex, const ezVec3& vPosition, const ezVec3& vForward, const ezVec3& vUp, const ezVec3& vVelocity) override;

  // ----- ezAudioSystem

public:
  ezAudioSystem();
  ~ezAudioSystem();

  bool Startup();
  void Shutdown();

  void SendRequest(ezVariant&& request);
  void SendRequests(ezAudioSystemRequestsQueue& requests);

  void SendRequestSync(ezVariant&& request);

  ezAudioSystemDataID GetTriggerId(const char* szTriggerName) const;

  ezAudioSystemDataID GetRtpcId(const char* szRtpcName) const;

  ezAudioSystemDataID GetSwitchId(const char* szSwitchName) const;

  ezAudioSystemDataID GetSwitchStateId(ezAudioSystemDataID uiSwitchId, const char* szSwitchStateName) const;

  ezAudioSystemDataID GetEnvironmentId(const char* szEnvironmentName) const;

  ezAudioSystemDataID GetBankId(const char* szBankName) const;

  const char* GetControlsPath() const;

  void UpdateControlsPath();

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(AudioSystem, AudioSystemPlugin);

  friend class ezAudioThread;
  friend class ezAudioTranslationLayer;
  friend class ezAudioControlCollectionResource;

  static void GameApplicationEventHandler(const ezGameApplicationExecutionEvent& e);

  void UpdateInternal();

  void StartAudioThread();
  void StopAudioThread();

  void QueueRequestCallback(ezVariant&& request);

  void RegisterTrigger(const char* szTriggerName, const char* szControlFile);
  void RegisterTrigger(const char* szTriggerName, ezStreamReader* pStreamReader);
  void RegisterRtpc(const char* szTriggerName, const char* szControlFile);
  void RegisterRtpc(const char* szRtpcName, ezStreamReader* pStreamReader);

  ezAudioThread* m_pAudioThread = nullptr;
  ezAudioTranslationLayer m_AudioTranslationLayer;

  ezAudioControlCollectionResourceHandle m_hAudioControlsCollection;

  ezAudioSystemRequestsQueue m_RequestsQueue;
  ezAudioSystemRequestsQueue m_PendingRequestsQueue;
  ezAudioSystemRequestsQueue m_BlockingRequestsQueue;
  ezAudioSystemRequestsQueue m_PendingRequestCallbacksQueue;

  mutable ezMutex m_RequestsMutex;
  mutable ezMutex m_PendingRequestsMutex;
  mutable ezMutex m_BlockingRequestsMutex;
  mutable ezMutex m_PendingRequestCallbacksMutex;

  ezSemaphore m_MainEvent;
  ezSemaphore m_ProcessingEvent;

  bool m_bInitialized;
};
