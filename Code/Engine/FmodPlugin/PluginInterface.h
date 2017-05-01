#pragma once

class ezFmodInterface
{
public:

  /// \brief Has to be called once per frame to update fmod
  virtual void UpdateFmod() = 0;

  /// \brief Adjusts the master volume. This affects all sounds, with no exception. Value must be between 0.0f and 1.0f.
  virtual void SetMasterChannelVolume(float volume) = 0;
  virtual float GetMasterChannelVolume() const = 0;

  /// \brief Allows to mute all sounds. Useful for when the application goes to a background state.
  virtual void SetMasterChannelMute(bool mute) = 0;
  virtual bool GetMasterChannelMute() const = 0;

  /// \brief Allows to pause all sounds. Useful for when the application goes to a background state and you want to pause all sounds, instead of mute them.
  virtual void SetMasterChannelPaused(bool paused) = 0;
  virtual bool GetMasterChannelPaused() const = 0;

  /// \brief Specifies the volume for a VCA ('Voltage Control Amplifier').
  ///
  /// This is used to control the volume of high level sound groups, such as 'Effects', 'Music', 'Ambience' or 'Speech'.
  /// Note that the fmod strings banks are never loaded, so the given string must be a GUID (fmod Studio -> Copy GUID).
  virtual void SetSoundGroupVolume(const char* szVcaGroupGuid, float volume) = 0;
  virtual float GetSoundGroupVolume(const char* szVcaGroupGuid) const = 0;

  /// \brief Default is 1. Allows to set how many virtual listeners the sound is mixed for (split screen game play).
  virtual void SetNumListeners(ezUInt8 uiNumListeners) = 0;
  virtual ezUInt8 GetNumListeners() = 0;
};

