#pragma once

#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/Strings/String.h>

class EZ_EDITORFRAMEWORK_DLL ezProjectPreferencesShared : public ezPreferences
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProjectPreferencesShared, ezPreferences);

public:
  ezProjectPreferencesShared();

};


class EZ_EDITORFRAMEWORK_DLL ezProjectPreferencesUser : public ezPreferences
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProjectPreferencesUser, ezPreferences);

public:
  ezProjectPreferencesUser();

  ezString m_sRenderPipelines;
};

