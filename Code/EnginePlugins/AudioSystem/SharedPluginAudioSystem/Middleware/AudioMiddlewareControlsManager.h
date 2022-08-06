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
  virtual ezResult SerializeTriggerControl(ezStreamWriter* stream, const ezAudioSystemTriggerData* triggerData) = 0;

  virtual ezResult CreateTriggerControl(const char* szTriggerName, const ezAudioSystemTriggerData* triggerData);
};
