#pragma once

#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/Strings/String.h>

/// \brief Stores project specific preferences for the current user
class EZ_EDITORFRAMEWORK_DLL ezProjectPreferencesUser : public ezPreferences
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProjectPreferencesUser, ezPreferences);

public:
  ezProjectPreferencesUser();

  // which apps to launch as external 'Players' (other than ezPlayer.exe)
  ezDynamicArray<ezString> m_PlayerApps;

  // the directory where the project should be exported to
  ezString m_sExportFolder;

  // path to a folder where shared materials should be stored
  ezString m_sSharedMaterialFolder;
};
