#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/Core/AudioSystem.h>
#include <AudioSystemPlugin/Core/AudioThread.h>

ezAudioThread::ezAudioThread()
  : ezThread("AudioThread")
{
}

ezUInt32 ezAudioThread::Run()
{
  EZ_ASSERT_DEBUG(m_pAudioSystem, "AudioThread has no AudioSystem!");

  while (m_bKeepRunning)
  {
    m_pAudioSystem->UpdateInternal();
  }

  return 0;
}

EZ_STATICLINK_FILE(AudioSystemPlugin, AudioSystemPlugin_Implementation_AudioThread);
