#pragma once

#include <EditorFramework/Preferences/Preferences.h>

class ezScenePreferencesUser : public ezPreferences
{
  EZ_ADD_DYNAMIC_REFLECTION(ezScenePreferencesUser, ezPreferences);

public:
  ezScenePreferencesUser()
    : ezPreferences(Domain::Document, ezPreferences::Visibility::User, "Scene")
  {
    m_iCameraSpeed = 15;
  }

  int m_iCameraSpeed;
};
