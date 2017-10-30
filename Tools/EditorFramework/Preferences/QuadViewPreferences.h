#pragma once

#include <EditorFramework/Preferences/Preferences.h>
#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>

struct EZ_EDITORFRAMEWORK_DLL ezEngineViewPreferences
{
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
};
