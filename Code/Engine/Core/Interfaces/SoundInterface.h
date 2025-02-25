#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>

class EZ_CORE_DLL ezSoundInterface
{
public:
  /// \brief Can be called before startup to load the configs from a different file.
  /// Otherwise will automatically be loaded by the sound system startup with the default path.
  virtual void LoadConfiguration(ezStringView sFile) = 0;

  /// \brief By default the integration should auto-detect the platform (and thus the config) to use.
  /// Calling this before startup allows to override which configuration is used.
  virtual void SetOverridePlatform(ezStringView sPlatform) = 0;

  /// \brief Has to be called once per frame to update all sounds
  virtual void UpdateSound() = 0;

  /// \brief Adjusts the master volume. This affects all sounds, with no exception. Value must be between 0.0f and 1.0f.
  virtual void SetMasterChannelVolume(float fVolume) = 0;
  virtual float GetMasterChannelVolume() const = 0;

  /// \brief Allows to mute all sounds. Useful for when the application goes to a background state.
  virtual void SetMasterChannelMute(bool bMute) = 0;
  virtual bool GetMasterChannelMute() const = 0;

  /// \brief Allows to pause all sounds. Useful for when the application goes to a background state and you want to pause all sounds, instead of mute
  /// them.
  virtual void SetMasterChannelPaused(bool bPaused) = 0;
  virtual bool GetMasterChannelPaused() const = 0;

  /// \brief Specifies the volume for a VCA ('Voltage Control Amplifier').
  ///
  /// This is used to control the volume of high level sound groups, such as 'Effects', 'Music', 'Ambiance' or 'Speech'.
  /// Note that the FMOD strings banks are never loaded, so the given string must be a GUID (FMOD Studio -> Copy GUID).
  virtual void SetSoundGroupVolume(ezStringView sVcaGroupGuid, float fVolume) = 0;
  virtual float GetSoundGroupVolume(ezStringView sVcaGroupGuid) const = 0;

  /// \brief Default is 1. Allows to set how many virtual listeners the sound is mixed for (split screen game play).
  virtual void SetNumListeners(ezUInt8 uiNumListeners) = 0;
  virtual ezUInt8 GetNumListeners() = 0;

  /// \brief The editor activates this to ignore the listener positions from the listener components, and instead use the editor camera as the
  /// listener position.
  virtual void SetListenerOverrideMode(bool bEnabled) = 0;

  /// \brief Sets the position for listener N. Index -1 is used for the override mode listener.
  virtual void SetListener(ezInt32 iIndex, const ezVec3& vPosition, const ezVec3& vForward, const ezVec3& vUp, const ezVec3& vVelocity) = 0;

  /// \brief Plays a sound once. Callced by ezSoundInterface::PlaySound().
  virtual ezResult OneShotSound(ezStringView sResourceID, const ezTransform& globalPosition, float fPitch = 1.0f, float fVolume = 1.0f, bool bBlockIfNotLoaded = true) = 0;

  /// \brief Plays a sound once.
  ///
  /// Convenience function to call OneShotSound() without having to retrieve the ezSoundInterface first.
  ///
  /// Which sound to play is specified through a resource ID ('Asset GUID').
  /// This is not the most efficient way to load a sound, as there is no way to preload the resource.
  /// If preloading is desired, you need to access the implementation-specific resource type directly (e.g. ezFmodSoundEventResource).
  /// Also see ezFmodSoundEventResource::PlayOnce().
  /// In practice, though, sounds are typically loaded in bulk from sound-banks, and preloading is not necessary.
  ///
  /// Be aware that this does not allow to adjust volume, pitch or position after creation. Stopping is also not possible.
  /// Use a sound component, if that is necessary.
  ///
  /// Also by default a pitch of 1 is always used. If the game speed is not 1 (ezWorld clock), a custom pitch would need to be provided,
  /// if the sound should play at the same speed.
  static ezResult PlaySound(ezStringView sResourceID, const ezTransform& globalPosition, float fPitch = 1.0f, float fVolume = 1.0f, bool bBlockIfNotLoaded = true);
};

class EZ_CORE_DLL ezScriptExtensionClass_Sound
{
public:
  static void PlaySound(ezStringView sResourceID, const ezVec3& vGlobalPos, const ezQuat& qGlobalRot, float fPitch = 1.0f, float fVolume = 1.0f, bool bBlockIfNotLoaded = true);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezScriptExtensionClass_Sound);
