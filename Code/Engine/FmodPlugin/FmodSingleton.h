#pragma once

#include <FmodPlugin/Basics.h>
#include <GameEngine/Interfaces/SoundInterface.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Singleton.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Types/UniquePtr.h>

struct ezGameApplicationEvent;
class ezOpenDdlWriter;
class ezOpenDdlReaderElement;
typedef ezDynamicArray<ezUInt8> ezDataBuffer;

typedef ezTypedResourceHandle<class ezFmodSoundBankResource> ezFmodSoundBankResourceHandle;

/// \brief Abstraction of FMOD_SPEAKERMODE
enum class ezFmodSpeakerMode : ezUInt8
{
  ModeStereo,
  Mode5Point1,
  Mode7Point1,
};

/// \brief The fmod configuration to be used on a specific platform
struct EZ_FMODPLUGIN_DLL ezFmodConfiguration
{
  ezString m_sMasterSoundBank;
  ezFmodSpeakerMode m_SpeakerMode = ezFmodSpeakerMode::Mode5Point1; ///< This must be set to what is configured in Fmod Studio for the target platform. Using anything else is incorrect.
  ezUInt16 m_uiVirtualChannels = 32; ///< See FMOD::Studio::System::initialize
  ezUInt32 m_uiSamplerRate = 0; ///< See FMOD::System::setSoftwareFormat

  void Save(ezOpenDdlWriter& ddl) const;
  void Load(const ezOpenDdlReaderElement& ddl);

  bool operator==(const ezFmodConfiguration& rhs) const;
  bool operator!=(const ezFmodConfiguration& rhs) const { return !operator==(rhs); }
};

/// \brief All available fmod platform configurations 
struct EZ_FMODPLUGIN_DLL ezFmodPlatformConfigs
{
  ezResult Save(const char* szFile) const;
  ezResult Load(const char* szFile);

  ezMap<ezString, ezFmodConfiguration> m_PlatformConfigs;
};

class EZ_FMODPLUGIN_DLL ezFmod : public ezSoundInterface
{
  EZ_DECLARE_SINGLETON_OF_INTERFACE(ezFmod, ezSoundInterface);

public:
  ezFmod();

  void Startup();
  void Shutdown();

  FMOD::Studio::System* GetStudioSystem() const { return m_pStudioSystem; }
  FMOD::System* GetLowLevelSystem() const { return m_pLowLevelSystem; }

  /// \brief Can be called before startup to load the fmod configs from a different file.
  /// Otherwise will automatically be loaded by fmod startup with the default path ":project/FmodConfig.ddl"
  virtual void LoadConfiguration(const char* szFile) override;

  /// \brief By default the fmod integration will auto-detect the platform (and thus the config) to use.
  /// Calling this before startup allows to override which configuration is used.
  virtual void SetOverridePlatform(const char* szPlatform) override;

  /// \brief Automatically called by the plugin every time ezGameApplicationEvent::BeforeUpdatePlugins is fired.
  virtual void UpdateSound() override;

  /// \brief Adjusts the master volume. This affects all sounds, with no exception. Value must be between 0.0f and 1.0f.
  virtual void SetMasterChannelVolume(float volume) override;
  virtual float GetMasterChannelVolume() const override;

  /// \brief Allows to mute all sounds. Useful for when the application goes to a background state.
  virtual void SetMasterChannelMute(bool mute) override;
  virtual bool GetMasterChannelMute() const override;

  /// \brief Allows to pause all sounds. Useful for when the application goes to a background state and you want to pause all sounds, instead of mute them.
  virtual void SetMasterChannelPaused(bool paused) override;
  virtual bool GetMasterChannelPaused() const override;

  /// \brief Specifies the volume for a VCA ('Voltage Control Amplifier').
  ///
  /// This is used to control the volume of high level sound groups, such as 'Effects', 'Music', 'Ambiance or 'Speech'.
  /// Note that the fmod strings banks are never loaded, so the given string must be a GUID (fmod Studio -> Copy GUID).
  virtual void SetSoundGroupVolume(const char* szVcaGroupGuid, float volume) override;
  virtual float GetSoundGroupVolume(const char* szVcaGroupGuid) const override;
  void UpdateSoundGroupVolumes();

  /// \brief Default is 1. Allows to set how many virtual listeners the sound is mixed for (split screen game play).
  virtual void SetNumListeners(ezUInt8 uiNumListeners) override;
  virtual ezUInt8 GetNumListeners() override;

  static void GameApplicationEventHandler(const ezGameApplicationEvent& e);

  /// \brief Configures how many reverb ('EAX') volumes are being blended/mixed for a sound.
  ///
  /// The number is clamped between 0 and 4. 0 Means all environmental effects are disabled for all sound sources.
  /// 1 means only the most important reverb is applied. 2, 3 and 4 allow to add more fidelity, but will cost more CPU resources.
  ///
  /// The default is currently 4.
  void SetNumBlendedReverbVolumes(ezUInt8 uiNumBlendedVolumes);

  /// \brief See SetNumBlendedReverbVolumes()
  ezUInt8 GetNumBlendedReverbVolumes() const { return m_uiNumBlendedVolumes; }


  virtual void SetListenerOverrideMode(bool enabled) override;
  virtual void SetListener(ezInt32 iIndex, const ezVec3& vPosition, const ezVec3& vForward, const ezVec3& vUp, const ezVec3& vVelocity) override;

private:
  friend class ezFmodSoundBankResource;
  void QueueSoundBankDataForDeletion(ezDataBuffer* pData);
  void ClearSoundBankDataDeletionQueue();
  mutable ezMutex m_DeletionQueueMutex;

private:

  void DetectPlatform();
  ezResult LoadMasterSoundBank(const char* szMasterBankResourceID);

  bool m_bInitialized = false;
  bool m_bListenerOverrideMode = false;
  ezUInt8 m_uiNumBlendedVolumes = 4;

  FMOD::Studio::System* m_pStudioSystem;
  FMOD::System* m_pLowLevelSystem;

  struct Data
  {
    ezMap<ezString, float> m_VcaVolumes;
    ezFmodPlatformConfigs m_Configs;
    ezString m_sPlatform;
    ezFmodSoundBankResourceHandle m_hMasterBank;
    ezHybridArray<ezDataBuffer*, 4> m_SbDeletionQueue;
  };

  ezUniquePtr<Data> m_pData;
};

EZ_DYNAMIC_PLUGIN_DECLARATION(EZ_FMODPLUGIN_DLL, ezFmodPlugin);
