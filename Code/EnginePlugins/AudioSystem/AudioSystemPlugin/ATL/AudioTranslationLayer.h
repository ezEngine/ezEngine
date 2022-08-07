#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <AudioSystemPlugin/ATL/AudioTranslationLayerData.h>
#include <AudioSystemPlugin/Core/AudioSystemRequests.h>
#include <AudioSystemPlugin/Core/AudioThread.h>

class EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioTranslationLayer
{
public:
  ezAudioTranslationLayer();
  ~ezAudioTranslationLayer();

  /// \brief Initializes the audio translation layer.
  [[nodiscard]] ezResult Startup();

  /// \brief Shuts down the audio translation layer.
  void Shutdown();

  /// \brief Updates the audio translation layer.
  /// This will also trigger an update of the audio middleware.
  void Update();

  ezAudioSystemDataID GetTriggerId(const char* szTriggerName) const;

  ezAudioSystemDataID GetRtpcId(const char* szRtpcName) const;

private:
  friend class ezAudioSystem;

  void ProcessRequest(ezVariant&& request);

  void RegisterTrigger(ezAudioSystemDataID uiId, ezAudioSystemTriggerData* pTriggerData);
  void RegisterRtpc(ezAudioSystemDataID uiId, ezAudioSystemRtpcData* pTriggerData);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  void DebugRender();
#endif

  // ATLObject containers
  ezATLEntityLookup m_mEntities;
  ezATLTriggerLookup m_mTriggers;
  ezATLRtpcLookup m_mRtpcs;
  // ezATLSwitchLookup m_mSwitches;
  // ezATLBanksLookup m_mBanks;
  // ezATLEnvironmentLookup m_mEnvironments;

  ezTime m_LastUpdateTime;
  ezTime m_LastFrameTime;
};
