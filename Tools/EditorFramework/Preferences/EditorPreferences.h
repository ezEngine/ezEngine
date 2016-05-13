#pragma once

#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/Strings/String.h>

class EZ_EDITORFRAMEWORK_DLL ezEditorPreferencesShared : public ezPreferences
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorPreferencesShared, ezPreferences);

public:
  ezEditorPreferencesShared();

};


class EZ_EDITORFRAMEWORK_DLL ezEditorPreferencesUser : public ezPreferences
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorPreferencesUser, ezPreferences);

public:
  ezEditorPreferencesUser();

  float m_fGizmoScale;
};

