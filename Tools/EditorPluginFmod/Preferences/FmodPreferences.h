#pragma once

#include <EditorPluginFmod/Plugin.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/Strings/String.h>

class EZ_EDITORPLUGINFMOD_DLL ezFmodProjectPreferences : public ezPreferences
{
  EZ_ADD_DYNAMIC_REFLECTION(ezFmodProjectPreferences, ezPreferences);

public:
  ezFmodProjectPreferences();

  void SetMute(bool mute);
  bool GetMute() const { return m_bMute; }

  void SetVolume(float fVolume);
  float GetVolume() const { return m_fMasterVolume; }

  void SyncCVars();

private:
  bool m_bMute = false;
  float m_fMasterVolume = 1.0f;
};