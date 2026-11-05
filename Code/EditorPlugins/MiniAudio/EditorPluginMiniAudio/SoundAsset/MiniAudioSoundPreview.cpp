#include <EditorPluginMiniAudio/EditorPluginMiniAudioPCH.h>

#include <EditorPluginMiniAudio/SoundAsset/MiniAudioSoundPreview.h>
#include <Foundation/IO/FileSystem/FileReader.h>

namespace
{
  // goes through ezFileReader instead of ma_decoder_init_file, so that paths are always treated as UTF-8
  ezResult ReadFile(ezStringView sAbsolutePath, ezDynamicArray<ezUInt8>& out_data)
  {
    ezFileReader file;
    EZ_SUCCEED_OR_RETURN(file.Open(sAbsolutePath));

    out_data.SetCountUninitialized(static_cast<ezUInt32>(file.GetFileSize()));

    if (!out_data.IsEmpty())
    {
      out_data.SetCountUninitialized(static_cast<ezUInt32>(file.ReadBytes(out_data.GetData(), out_data.GetCount())));
    }

    return EZ_SUCCESS;
  }
} // namespace

ezMiniAudioSoundPreview::ezMiniAudioSoundPreview() = default;

ezMiniAudioSoundPreview::~ezMiniAudioSoundPreview()
{
  Stop();

  if (m_bEngineInitialized)
  {
    ma_engine_uninit(&m_Engine);
  }
}

ezResult ezMiniAudioSoundPreview::QueryFileInfo(ezStringView sAbsolutePath, FileInfo& out_info)
{
  ezDynamicArray<ezUInt8> data;
  EZ_SUCCEED_OR_RETURN(ReadFile(sAbsolutePath, data));

  ma_decoder decoder;
  if (ma_decoder_init_memory(data.GetData(), data.GetCount(), nullptr, &decoder) != MA_SUCCESS)
    return EZ_FAILURE;

  EZ_SCOPE_EXIT(ma_decoder_uninit(&decoder));

  ma_format format = ma_format_unknown;
  ma_uint32 uiChannels = 0;
  ma_uint32 uiSampleRate = 0;
  if (ma_decoder_get_data_format(&decoder, &format, &uiChannels, &uiSampleRate, nullptr, 0) != MA_SUCCESS)
    return EZ_FAILURE;

  ma_uint64 uiFrames = 0;
  if (ma_decoder_get_length_in_pcm_frames(&decoder, &uiFrames) != MA_SUCCESS)
    return EZ_FAILURE;

  out_info.m_uiChannels = uiChannels;
  out_info.m_uiSampleRate = uiSampleRate;
  out_info.m_Length = uiSampleRate > 0 ? ezTime::MakeFromSeconds(static_cast<double>(uiFrames) / uiSampleRate) : ezTime::MakeZero();

  return EZ_SUCCESS;
}

ezResult ezMiniAudioSoundPreview::Play(ezStringView sAbsolutePath, ezTime length, bool bLoop, float fVolume, float fPitch)
{
  Stop();

  if (!m_bEngineInitialized)
  {
    ma_engine_config cfg = ma_engine_config_init();
    cfg.listenerCount = 1;

    const ma_result res = ma_engine_init(&cfg, &m_Engine);
    if (res != MA_SUCCESS)
    {
      ezLog::Error("Failed to initialize MiniAudio for the sound preview: {}", ma_result_description(res));
      return EZ_FAILURE;
    }

    m_bEngineInitialized = true;
  }

  if (ReadFile(sAbsolutePath, m_AudioData).Failed())
  {
    ezLog::Error("Failed to read sound file '{}'", sAbsolutePath);
    return EZ_FAILURE;
  }

  ma_result res = ma_decoder_init_memory(m_AudioData.GetData(), m_AudioData.GetCount(), nullptr, &m_Decoder);
  if (res != MA_SUCCESS)
  {
    ezLog::Error("Failed to decode sound file '{}': {}", sAbsolutePath, ma_result_description(res));
    return EZ_FAILURE;
  }

  res = ma_sound_init_from_data_source(&m_Engine, &m_Decoder, MA_SOUND_FLAG_NO_SPATIALIZATION, nullptr, &m_Sound);
  if (res != MA_SUCCESS)
  {
    ma_decoder_uninit(&m_Decoder);

    ezLog::Error("Failed to create a sound from file '{}': {}", sAbsolutePath, ma_result_description(res));
    return EZ_FAILURE;
  }

  m_bSoundInitialized = true;
  m_Length = length;

  ma_sound_set_looping(&m_Sound, bLoop ? MA_TRUE : MA_FALSE);
  ma_sound_set_volume(&m_Sound, fVolume);
  ma_sound_set_pitch(&m_Sound, fPitch);

  res = ma_sound_start(&m_Sound);
  if (res != MA_SUCCESS)
  {
    Stop();

    ezLog::Error("Failed to start sound file '{}': {}", sAbsolutePath, ma_result_description(res));
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

void ezMiniAudioSoundPreview::Stop()
{
  if (!m_bSoundInitialized)
    return;

  m_bSoundInitialized = false;

  // the sound reads from the decoder on the audio thread, so it has to go first
  ma_sound_uninit(&m_Sound);
  ma_decoder_uninit(&m_Decoder);

  m_AudioData.Clear();
  m_Length = ezTime::MakeZero();
}

bool ezMiniAudioSoundPreview::IsPlaying() const
{
  return m_bSoundInitialized && ma_sound_is_playing(&m_Sound) && !ma_sound_at_end(&m_Sound);
}

ezTime ezMiniAudioSoundPreview::GetPosition() const
{
  if (!m_bSoundInitialized)
    return ezTime::MakeZero();

  float fSeconds = 0.0f;
  if (ma_sound_get_cursor_in_seconds(&m_Sound, &fSeconds) != MA_SUCCESS)
    return ezTime::MakeZero();

  return ezTime::MakeFromSeconds(fSeconds);
}
