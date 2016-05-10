#pragma once

#include <EditorFramework/Preferences/Preferences.h>

class ezSceneUserPreferences : public ezPreferences
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneUserPreferences, ezPreferences);

public:
  ezSceneUserPreferences()
    : ezPreferences(Domain::Document, ezPreferences::Visibility::User, "Scene")
  {
    m_iCameraSpeed = 15;
  }

  int m_iCameraSpeed;
};
