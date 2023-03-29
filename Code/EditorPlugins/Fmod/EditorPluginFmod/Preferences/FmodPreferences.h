#pragma once

#include <EditorFramework/Preferences/Preferences.h>
#include <EditorPluginFmod/EditorPluginFmodDLL.h>
#include <Foundation/Strings/String.h>

class EZ_EDITORPLUGINFMOD_DLL ezFmodProjectPreferences : public ezPreferences
{
  EZ_ADD_DYNAMIC_REFLECTION(ezFmodProjectPreferences, ezPreferences);

public:
  ezFmodProjectPreferences();
  ~ezFmodProjectPreferences();

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
