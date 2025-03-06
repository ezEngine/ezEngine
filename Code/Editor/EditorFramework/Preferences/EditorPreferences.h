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

  void ApplyDefaultValues(ezEngineViewLightSettings& ref_settings);
  void SetAsDefaultValues(const ezEngineViewLightSettings& settings);

  float m_fPerspectiveFieldOfView = 70.0f;
  ezAngle m_RotationSnapValue = ezAngle::MakeFromDegree(15.0f);
  float m_fScaleSnapValue = 0.125f;
  float m_fTranslationSnapValue = 0.25f;
  bool m_bUsePrecompiledTools = true;
  ezString m_sCustomPrecompiledToolsFolder;
  bool m_bLoadLastProjectAtStartup = true;
  bool m_bShowSplashscreen = true;
  bool m_bExpandSceneTreeOnSelection = true;
  bool m_bBackgroundAssetProcessing = true;
  ezUInt8 m_uiMaxAssetProcessors = 8;
  bool m_bHighlightUntranslatedUI = false;
  bool m_bAssetBrowserShowItemsInSubFolders = true;

  bool m_bSkyBox = true;
  bool m_bSkyLight = true;
  ezString m_sSkyLightCubeMap = "{ 0b202e08-a64f-465d-b38e-15b81d161822 }";
  float m_fSkyLightIntensity = 1.0f;
  bool m_bDirectionalLight = true;
  ezAngle m_DirectionalLightAngle = ezAngle::MakeFromDegree(70.0f);
  bool m_bDirectionalLightShadows = false;
  float m_fDirectionalLightIntensity = 10.0f;
  bool m_bFog = false;
  bool m_bClearEditorLogsOnPlay = true;
  bool m_bCombinedEditorAndEngineLogs = true;

  void SetShowInDevelopmentFeatures(bool b);
  bool GetShowInDevelopmentFeatures() const
  {
    return m_bShowInDevelopmentFeatures;
  }

  void SetHighlightUntranslatedUI(bool b);
  bool GetHighlightUntranslatedUI() const
  {
    return m_bHighlightUntranslatedUI;
  }

  void SetGizmoSize(float f);
  float GetGizmoSize() const { return m_fGizmoSize; }

  void SetMaxFramerate(ezUInt16 uiFPS);
  ezUInt16 GetMaxFramerate() const { return m_uiMaxFramerate; }

  ezHybridArray<ezString, 8> m_RecentlyCreatedTypes;

private:
  void SyncGlobalSettings();

  float m_fGizmoSize = 1.5f;
  bool m_bShowInDevelopmentFeatures = false;
  ezUInt16 m_uiMaxFramerate = 60;
};
