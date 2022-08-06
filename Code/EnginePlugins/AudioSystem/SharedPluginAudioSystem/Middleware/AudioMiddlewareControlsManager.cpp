#include <SharedPluginAudioSystem/SharedPluginAudioSystemPCH.h>

#include <SharedPluginAudioSystem/Middleware/AudioMiddlewareControlsManager.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

ezResult ezAudioMiddlewareControlsManager::CreateTriggerControl(const char* szTriggerName, const ezAudioSystemTriggerData* triggerData)
{
  ezStringBuilder sbOutputFile;
  sbOutputFile.Format(":atl/Triggers/{0}.ezAudioSystemControl", szTriggerName);

  ezStringBuilder sbAssetPath;
  if (ezFileSystem::ResolvePath(sbOutputFile, &sbAssetPath, nullptr).Failed())
    return EZ_FAILURE;

  ezFileWriter file;
  if (file.Open(sbAssetPath, 256).Failed())
    return EZ_FAILURE;

  // Set the control type
  file << ezAudioSystemControlType::Trigger;

  // Serialize the trigger data. This method is implemented by the audio middleware.
  if (SerializeTriggerControl(&file, triggerData).Succeeded())
  {
    file.Close();
    return EZ_SUCCESS;
  }

  file.Close();
  return EZ_FAILURE;
}

EZ_STATICLINK_FILE(SharedPluginAudioSystem, SharedPluginAudioSystem_Middleware_AudioMiddlewareControlsManager);
