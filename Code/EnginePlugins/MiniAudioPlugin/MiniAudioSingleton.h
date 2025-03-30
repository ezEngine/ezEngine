#pragma once

#include <Core/Interfaces/SoundInterface.h>
#include <Core/World/Declarations.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>
#include <MiniAudio/miniaudio.h>
#include <MiniAudioPlugin/MiniAudioPluginDLL.h>

// TODO MiniAudio: Future Work
//
// * in MiniAudioResource Load sounds through the MA resource manager (redirect file hooks to our resource manager)
// * then decode one sound right away and use that as a template to copy from for future sound playback
// * Add support for sound buses (groups / VCAs), ie each sound resource should be in a group and the volume of groups should be configurable at runtime
// * Add max sound size, check whether MA adds FMOD-like attenuation models
// * skip sounds that are too far away
// * potentially virtualize sounds (probably out of scope for the MA plugin)
// * Add preview playback to sound asset
// * For the PlaySound function, take an ezWorld arg and then create a game object in that world -> works for randomization etc

struct ezGameApplicationExecutionEvent;

struct ezMiniAudioSoundInstance
{
  ma_sound m_Sound;
  ma_decoder m_Decoder;
  ezWorld* pWorld = nullptr;
  ezComponentHandle m_hComponent;
  ezUInt16 m_uiOwnIndex;
  bool m_bInUse = false;
};


class EZ_MINIAUDIOPLUGIN_DLL ezMiniAudioSingleton : public ezSoundInterface
{
  EZ_DECLARE_SINGLETON_OF_INTERFACE(ezMiniAudioSingleton, ezSoundInterface);

public:
  ezMiniAudioSingleton();
  ~ezMiniAudioSingleton();

  void Startup();
  void Shutdown();

  ma_engine* GetEngine() { return &m_pData->m_Engine; }

  /// \brief Can be called before startup to load the configuration from a different file.
  /// Otherwise will automatically be loaded at startup with the default path.
  virtual void LoadConfiguration(ezStringView sFile) override;

  /// \brief By default the integration will auto-detect the platform (and thus the config) to use.
  /// Calling this before startup allows to override which configuration is used.
  virtual void SetOverridePlatform(ezStringView sPlatform) override;

  /// \brief Automatically called by the plugin every time ezGameApplicationExecutionEvent::BeforeUpdatePlugins is fired.
  virtual void UpdateSound() override;

  /// \brief Adjusts the master volume. This affects all sounds, with no exception. Value must be between 0.0f and 1.0f.
  virtual void SetMasterChannelVolume(float fVolume) override;
  virtual float GetMasterChannelVolume() const override;

  /// \brief Allows to mute all sounds. Useful for when the application goes to a background state.
  virtual void SetMasterChannelMute(bool bMute) override;
  virtual bool GetMasterChannelMute() const override;

  /// \brief Allows to pause all sounds. Useful for when the application goes to a background state and you want to pause all sounds,
  /// instead of mute them.
  virtual void SetMasterChannelPaused(bool bPaused) override;
  virtual bool GetMasterChannelPaused() const override;

  /// \brief Specifies the volume for a VCA ('Voltage Control Amplifier').
  ///
  /// This is used to control the volume of high level sound groups, such as 'Effects', 'Music', 'Ambiance or 'Speech'.
  virtual void SetSoundGroupVolume(ezStringView sVcaGroupGuid, float fVolume) override;
  virtual float GetSoundGroupVolume(ezStringView sVcaGroupGuid) const override;
  void UpdateSoundGroupVolumes();

  /// \brief Default is 1. Allows to set how many virtual listeners the sound is mixed for (split screen game play).
  virtual void SetNumListeners(ezUInt8 uiNumListeners) override;
  virtual ezUInt8 GetNumListeners() override;

  static void GameApplicationEventHandler(const ezGameApplicationExecutionEvent& e);

  virtual void SetListenerOverrideMode(bool bEnabled) override;
  virtual void SetListener(ezInt32 iIndex, const ezVec3& vPosition, const ezVec3& vForward, const ezVec3& vUp, const ezVec3& vVelocity) override;
  ezVec3 GetListenerPosition() { return m_vListenerPosition; }

  virtual ezResult OneShotSound(ezStringView sResourceID, const ezTransform& globalPosition, float fPitch = 1.0f, float fVolume = 1.0f, bool bBlockIfNotLoaded = true) override;

  ezMiniAudioSoundInstance* AllocateSoundInstance(const ezDataBuffer& audioData, ezWorld* pWorld, ezComponentHandle hComponent);
  void FreeSoundInstance(ezMiniAudioSoundInstance*& ref_pInstance);
  void DetachSoundInstance(ezMiniAudioSoundInstance*& ref_pInstance);

  void DetachAndFadeOutSoundInstance(ezMiniAudioSoundInstance*& ref_pInstance, ezTime fadeDuration);

  void SoundEnded(ezMiniAudioSoundInstance* pInstance);

  void StopWorldSounds(ezWorld* pWorld);

private:
  bool m_bInitialized = false;
  bool m_bListenerOverrideMode = false;
  ezVec3 m_vListenerPosition;

  struct Data
  {
    ezMutex m_Mutex;

    ma_engine m_Engine;
    ezDeque<ezMiniAudioSoundInstance> m_SoundInstancesStorage;
    ezDeque<ezUInt32> m_SoundInstanceFreeList;

    ezDeque<ezUInt32> m_FadingInstances;
    ezDeque<ezUInt32> m_FinishedInstances;
  };

  ezUniquePtr<Data> m_pData;
};
