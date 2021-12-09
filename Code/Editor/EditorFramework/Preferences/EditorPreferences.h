#pragma once

#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/Strings/String.h>

class ezEngineViewLightSettings;

/// \brief Stores editor specific preferences for the current user
class EZ_EDITORFRAMEWORK_DLL ezEditorPreferencesUser : public ezPreferences
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorPreferencesUser, ezPreferences);

public:
  ezEditorPreferencesUser();
  ~ezEditorPreferencesUser();

  void ApplyDefaultValues(ezEngineViewLightSettings& settings);
  void SetAsDefaultValues(const ezEngineViewLightSettings& settings);

  float m_fPerspectiveFieldOfView = 70.0f;
  float m_fGizmoScale = 1.0f;
  bool m_bUsePrecompiledTools = true;
  bool m_bLoadLastProjectAtStartup = true;
  bool m_bShowSplashscreen = true;
  bool m_bExpandSceneTreeOnSelection = true;
  bool m_bBackgroundAssetProcessing = false;
  bool m_bAssetFilterCombobox = true;

  bool m_bSkyBox = true;
  bool m_bSkyLight = true;
  ezString m_sSkyLightCubeMap = "{ 0b202e08-a64f-465d-b38e-15b81d161822 }";
  float m_fSkyLightIntensity = 1.0f;
  bool m_bDirectionalLight = true;
  ezAngle m_DirectionalLightAngle = ezAngle::Degree(30.0f);
  bool m_bDirectionalLightShadows = false;
  float m_fDirectionalLightIntensity = 10.0f;
  bool m_bFog = false;
};
