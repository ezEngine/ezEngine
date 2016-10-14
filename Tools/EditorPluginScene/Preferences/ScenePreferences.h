#pragma once

#include <EditorFramework/Preferences/Preferences.h>

struct ezSceneViewPreferences
{
  ezVec3 m_vCamPos;
  ezVec3 m_vCamDir;
  ezVec3 m_vCamUp;
  ezUInt8 m_uiPerspectiveMode;
  ezUInt8 m_uiRenderMode;
  float m_fFov;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezSceneViewPreferences);

class ezScenePreferencesUser : public ezPreferences
{
  EZ_ADD_DYNAMIC_REFLECTION(ezScenePreferencesUser, ezPreferences);

public:
  ezScenePreferencesUser();

  void SetCameraSpeed(ezInt32 value);
  ezInt32 GetCameraSpeed() const { return m_iCameraSpeed; }

  void SetShowGrid(bool show);
  bool GetShowGrid() const { return m_bShowGrid; }

  bool m_bQuadView;
  ezSceneViewPreferences m_ViewSingle;
  ezSceneViewPreferences m_ViewQuad0;
  ezSceneViewPreferences m_ViewQuad1;
  ezSceneViewPreferences m_ViewQuad2;
  ezSceneViewPreferences m_ViewQuad3;

protected:
  bool m_bShowGrid;
  int m_iCameraSpeed;
};
