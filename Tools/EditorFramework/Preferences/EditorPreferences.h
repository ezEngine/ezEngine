#pragma once

#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/Strings/String.h>

/// \brief Stores editor specific preferences for the current user
class EZ_EDITORFRAMEWORK_DLL ezEditorPreferencesUser : public ezPreferences
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorPreferencesUser, ezPreferences);

public:
  ezEditorPreferencesUser();

  float m_fPerspectiveFieldOfView = 70.0f;
  float m_fGizmoScale = 1.0f;
  bool m_bUsePrecompiledTools = true;
  bool m_bLoadLastProjectAtStartup = true;
  bool m_bExpandSceneTreeOnSelection = true;
};

