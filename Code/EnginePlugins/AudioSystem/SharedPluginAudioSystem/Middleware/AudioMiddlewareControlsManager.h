#pragma once

#include <SharedPluginAudioSystem/SharedPluginAudioSystemDLL.h>

#include <AudioSystemPlugin/Core/AudioSystemData.h>

#include <Foundation/Basics.h>

class EZ_SHAREDPLUGINAUDIOSYSTEM_DLL ezAudioMiddlewareControlsManager
{
public:
  virtual ~ezAudioMiddlewareControlsManager() = default;

  /// \brief Create controls assets needed for the audio middleware.
  /// \return EZ_SUCCESS on success, otherwise EZ_FAILURE.
  virtual ezResult ReloadControls() = 0;

protected:
  virtual ezResult SerializeTriggerControl(ezStreamWriter* pStream, const ezAudioSystemTriggerData* pTriggerData) = 0;
  virtual ezResult SerializeRtpcControl(ezStreamWriter* pStream, const ezAudioSystemRtpcData* pRtpcData) = 0;

  virtual ezResult CreateTriggerControl(const char* szTriggerName, const ezAudioSystemTriggerData* pTriggerData);
  virtual ezResult CreateRtpcControl(const char* szRtpcName, const ezAudioSystemRtpcData* prtRtpcData);
};
