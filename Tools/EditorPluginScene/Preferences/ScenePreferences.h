#pragma once

#include <EditorFramework/Preferences/Preferences.h>

struct ezSceneViewPreferences
{
  ezVec3 m_vCamPos;
  ezVec3 m_vCamDir;
  ezVec3 m_vCamUp;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezSceneViewPreferences);

class ezScenePreferencesUser : public ezPreferences
{
  EZ_ADD_DYNAMIC_REFLECTION(ezScenePreferencesUser, ezPreferences);

public:
  ezScenePreferencesUser();

  int m_iCameraSpeed;
  ezSceneViewPreferences m_ViewSingle;
  ezSceneViewPreferences m_ViewQuad0;
  ezSceneViewPreferences m_ViewQuad1;
  ezSceneViewPreferences m_ViewQuad2;
  ezSceneViewPreferences m_ViewQuad3;
};
