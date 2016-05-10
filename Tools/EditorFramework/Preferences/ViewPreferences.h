#pragma once

#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/Strings/String.h>

class EZ_EDITORFRAMEWORK_DLL ezViewUserPreferences : public ezPreferences
{
  EZ_ADD_DYNAMIC_REFLECTION(ezViewUserPreferences, ezPreferences);

public:
  ezViewUserPreferences()
    : ezPreferences(Domain::Project, ezPreferences::Visibility::User, "EditorView")
  {
  }

  ezString m_sRenderPipelines;
};
