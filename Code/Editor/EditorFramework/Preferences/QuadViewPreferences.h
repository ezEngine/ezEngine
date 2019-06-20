#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/Preferences/Preferences.h>

struct EZ_EDITORFRAMEWORK_DLL ezEngineViewPreferences
{
  ezEngineViewPreferences()
  {
    m_vCamPos.SetZero();
    m_vCamDir.Set(1, 0, 0);
    m_vCamUp.Set(0, 0, 1);
    m_uiPerspectiveMode = ezSceneViewPerspective::Perspective;
    m_uiRenderMode = ezViewRenderMode::Default;
    m_fFov = 70.0f;
  }

  ezVec3 m_vCamPos;
  ezVec3 m_vCamDir;
  ezVec3 m_vCamUp;
  ezSceneViewPerspective::Enum m_uiPerspectiveMode;
  ezViewRenderMode::Enum m_uiRenderMode;
  float m_fFov;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_EDITORFRAMEWORK_DLL, ezEngineViewPreferences);

class EZ_EDITORFRAMEWORK_DLL ezQuadViewPreferencesUser : public ezPreferences
{
  EZ_ADD_DYNAMIC_REFLECTION(ezQuadViewPreferencesUser, ezPreferences);

public:
  ezQuadViewPreferencesUser();

  bool m_bQuadView;
  ezEngineViewPreferences m_ViewSingle;
  ezEngineViewPreferences m_ViewQuad0;
  ezEngineViewPreferences m_ViewQuad1;
  ezEngineViewPreferences m_ViewQuad2;
  ezEngineViewPreferences m_ViewQuad3;

  ezUInt32 FavCams_GetCount() const { return 10; }
  ezEngineViewPreferences FavCams_GetCam(ezUInt32 i) const { return m_FavouriteCamera[i]; }
  void FavCams_SetCam(ezUInt32 i, ezEngineViewPreferences cam) { m_FavouriteCamera[i] = cam; }
  void FavCams_Insert(ezUInt32 uiIndex, ezEngineViewPreferences cam) {}
  void FavCams_Remove(ezUInt32 uiIndex) {}

  ezEngineViewPreferences m_FavouriteCamera[10];
};
