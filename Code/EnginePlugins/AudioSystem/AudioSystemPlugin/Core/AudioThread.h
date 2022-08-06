#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <Foundation/Threading/Thread.h>

/// \brief The audio thread. Responsible to process audio requests.
class EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioThread : public ezThread
{
public:
  ezAudioThread();

private:
  friend class ezAudioSystem;

  ezUInt32 Run() override;

  class ezAudioSystem* m_pAudioSystem = nullptr;
  volatile bool m_bKeepRunning = true;
};
