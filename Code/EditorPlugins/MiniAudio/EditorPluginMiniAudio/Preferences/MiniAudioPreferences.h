#pragma once

#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorPluginMiniAudio/EditorPluginMiniAudioDLL.h>
#include <Foundation/Strings/String.h>

class EZ_EDITORPLUGINMINIAUDIO_DLL ezMiniAudioProjectPreferences : public ezPreferences
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMiniAudioProjectPreferences, ezPreferences);

public:
  ezMiniAudioProjectPreferences();
  ~ezMiniAudioProjectPreferences();

  void SetMute(bool bMute);
  bool GetMute() const { return m_bMute; }

  void SetVolume(float fVolume);
  float GetVolume() const { return m_fMasterVolume; }

  void SyncCVars();

private:
  void ProcessEventHandler(const ezEditorEngineProcessConnection::Event& e);

  bool m_bMute = false;
  float m_fMasterVolume = 1.0f;
};
