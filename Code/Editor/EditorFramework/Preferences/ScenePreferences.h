#pragma once

#include <EditorFramework/Preferences/Preferences.h>

class EZ_EDITORFRAMEWORK_DLL ezScenePreferencesUser : public ezPreferences
{
  EZ_ADD_DYNAMIC_REFLECTION(ezScenePreferencesUser, ezPreferences);

public:
  ezScenePreferencesUser();

  void SetCameraSpeed(ezInt32 value);
  ezInt32 GetCameraSpeed() const { return m_iCameraSpeed; }

  void SetShowGrid(bool show);
  bool GetShowGrid() const { return m_bShowGrid; }

protected:
  bool m_bShowGrid;
  int m_iCameraSpeed;
};
