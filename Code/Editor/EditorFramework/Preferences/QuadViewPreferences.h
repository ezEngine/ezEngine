#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/Preferences/Preferences.h>

struct EZ_EDITORFRAMEWORK_DLL ezEngineViewPreferences
{
  /// Replaces invalid values (NaN, infinity, out of range) with usable defaults.
  ///
  /// The preferences are read from a file on disk, which may contain garbage (e.g. a NaN camera position),
  /// which would otherwise result in an invalid camera and frustum. Call this before using loaded values.
  void Sanitize();

  ezVec3 m_vCamPos = ezVec3::MakeZero();
  ezVec3 m_vCamDir = ezVec3::MakeAxisX();
  ezVec3 m_vCamUp = ezVec3::MakeAxisZ();
  ezSceneViewPerspective::Enum m_PerspectiveMode = ezSceneViewPerspective::Perspective;
  ezViewRenderMode::Enum m_RenderMode = ezViewRenderMode::Default;
  float m_fFov = 70.0f;
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
  ezEngineViewPreferences FavCams_GetCam(ezUInt32 i) const { return m_FavoriteCamera[i]; }
  void FavCams_SetCam(ezUInt32 i, ezEngineViewPreferences cam) { m_FavoriteCamera[i] = cam; }
  void FavCams_Insert(ezUInt32 uiIndex, ezEngineViewPreferences cam) {}
  void FavCams_Remove(ezUInt32 uiIndex) {}

  ezEngineViewPreferences m_FavoriteCamera[10];
};
