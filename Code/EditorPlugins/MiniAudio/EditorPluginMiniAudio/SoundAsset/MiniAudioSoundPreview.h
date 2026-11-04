#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Time.h>
#include <MiniAudio/miniaudio.h>

/// Plays a single sound file in the editor process, for the preview in the sound asset window.
///
/// ezMiniAudioSingleton can't be used for this, it is tied to the game application and only runs in the engine process.
/// The audio device is opened with the first call to Play() and is held until this object is destroyed.
class ezMiniAudioSoundPreview
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezMiniAudioSoundPreview);

public:
  struct FileInfo
  {
    ezTime m_Length;
    ezUInt32 m_uiSampleRate = 0;
    ezUInt32 m_uiChannels = 0;
  };

  ezMiniAudioSoundPreview();
  ~ezMiniAudioSoundPreview();

  /// Reads the stats of a sound file, without needing an audio device.
  ///
  /// Decoders have to scan the entire file to determine the length for some formats (mp3), so this shouldn't be called frequently.
  static ezResult QueryFileInfo(ezStringView sAbsolutePath, FileInfo& out_info);

  /// Stops any previous sound and starts the given file. The file is always played non-positional.
  ///
  /// The length is only stored for GetLength(), it has to come from QueryFileInfo().
  /// It can't be determined from the playing sound. For mp3 files that seeks through the decoder that the audio thread is reading from,
  /// which makes playback stutter and crashes the decoder.
  ///
  /// Failures are logged.
  ezResult Play(ezStringView sAbsolutePath, ezTime length, bool bLoop, float fVolume, float fPitch);

  void Stop();

  /// Returns false once the sound has ended by itself.
  bool IsPlaying() const;

  ezTime GetPosition() const;
  ezTime GetLength() const { return m_Length; }

private:
  ezDynamicArray<ezUInt8> m_AudioData;
  ezTime m_Length;

  ma_engine m_Engine;
  ma_decoder m_Decoder;
  ma_sound m_Sound;

  bool m_bEngineInitialized = false;
  bool m_bSoundInitialized = false;
};
